/*
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "checkasm.h"

#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cpu.h"

#ifdef _WIN32
#ifndef SIGBUS
/* non-standard, use the same value as mingw-w64 */
#define SIGBUS 10
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x04
#endif
#else
#include <time.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_SETAFFINITY_NP
#include <pthread.h>
#if HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
#endif
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif
#endif

#if CONFIG_LINUX_PERF
    #include <sys/syscall.h>
#elif CONFIG_MACOS_KPERF
    #include <dlfcn.h>
#endif

#define COLOR_RED    31
#define COLOR_GREEN  32
#define COLOR_YELLOW 33

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#if ARCH_AARCH64 && HAVE_SVE
int checkasm_sve_length(void);
#endif

typedef struct CheckasmFuncVersion {
    struct CheckasmFuncVersion *next;
    void *func;
    int ok;
    unsigned cpu;
    int iterations;
    uint64_t cycles;
} CheckasmFuncVersion;

/* Binary search tree node */
typedef struct CheckasmFunc {
    struct CheckasmFunc *child[2];
    CheckasmFuncVersion versions;
    uint8_t color; /* 0 = red, 1 = black */
    char name[];
} CheckasmFunc;

/* Internal state */
static CheckasmConfig cfg;
static struct {
    CheckasmFunc *funcs;
    CheckasmFunc *current_func;
    CheckasmFuncVersion *current_func_ver;
    const char *current_test_name;
    int num_checked;
    int num_failed;
    double nop_time;
    unsigned cpu_flag;
    const char *cpu_flag_name;
    volatile sig_atomic_t sig; // SIG_ATOMIC_MAX = signal handling enabled
    int suffix_length;
    int max_function_name_length;
#if ARCH_X86_64
    void (*simd_warmup)(void);
#endif
#if CONFIG_LINUX_PERF
    int perf_sysfd;
#endif
} state;

static uint32_t xs_state[4];

static void xor128_srand(unsigned seed) {
    xs_state[0] = seed;
    xs_state[1] = ( seed & 0xffff0000) | (~seed & 0x0000ffff);
    xs_state[2] = (~seed & 0xffff0000) | ( seed & 0x0000ffff);
    xs_state[3] = ~seed;
}

// xor128 from Marsaglia, George (July 2003). "Xorshift RNGs".
//             Journal of Statistical Software. 8 (14).
//             doi:10.18637/jss.v008.i14.
int xor128_rand(void) {
    const uint32_t x = xs_state[0];
    const uint32_t t = x ^ (x << 11);

    xs_state[0] = xs_state[1];
    xs_state[1] = xs_state[2];
    xs_state[2] = xs_state[3];
    uint32_t w = xs_state[3];

    w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
    xs_state[3] = w;

    return w >> 1;
}

#if CONFIG_LINUX_PERF

static int linux_perf_init(void)
{
    struct perf_event_attr attr = {
        .type           = PERF_TYPE_HARDWARE,
        .size           = sizeof(struct perf_event_attr),
        .config         = PERF_COUNT_HW_CPU_CYCLES,
        .disabled       = 1, // start counting only on demand
        .exclude_kernel = 1,
        .exclude_hv     = 1,
#if !ARCH_X86
        .exclude_guest  = 1,
#endif
    };

    fprintf(stderr, "benchmarking with Linux Perf Monitoring API\n");
    state.perf_sysfd = syscall(SYS_perf_event_open, &attr, 0, -1, -1, 0);
    if (state.perf_sysfd == -1) {
        perror("perf_event_open");
        return 1;
    }
    return 0;
}

int checkasm_get_perf_sysfd(void)
{
    return state.perf_sysfd;
}

#elif CONFIG_MACOS_KPERF

static int (*kpc_get_thread_counters)(int, unsigned int, void *);

#define CFGWORD_EL0A64EN_MASK (0x20000)

#define CPMU_CORE_CYCLE 0x02

#define KPC_CLASS_FIXED_MASK        (1 << 0)
#define KPC_CLASS_CONFIGURABLE_MASK (1 << 1)

#define COUNTERS_COUNT 10
#define CONFIG_COUNT 8
#define KPC_MASK (KPC_CLASS_CONFIGURABLE_MASK | KPC_CLASS_FIXED_MASK)

static int kperf_init(void)
{
    uint64_t config[COUNTERS_COUNT] = { 0 };

    void *kperf = dlopen("/System/Library/PrivateFrameworks/kperf.framework/kperf", RTLD_LAZY);
    if (!kperf) {
        fprintf(stderr, "checkasm: Unable to load kperf: %s\n", dlerror());
        return 1;
    }

    int (*kpc_force_all_ctrs_set)(int) = dlsym(kperf, "kpc_force_all_ctrs_set");
    int (*kpc_set_counting)(uint32_t) = dlsym(kperf, "kpc_set_counting");
    int (*kpc_set_thread_counting)(uint32_t) = dlsym(kperf, "kpc_set_thread_counting");
    int (*kpc_set_config)(uint32_t, void *) = dlsym(kperf, "kpc_set_config");
    uint32_t (*kpc_get_counter_count)(uint32_t) = dlsym(kperf, "kpc_get_counter_count");
    uint32_t (*kpc_get_config_count)(uint32_t) = dlsym(kperf, "kpc_get_config_count");
    kpc_get_thread_counters = dlsym(kperf, "kpc_get_thread_counters");

    if (!kpc_get_thread_counters) {
        fprintf(stderr, "checkasm: Unable to load kpc_get_thread_counters\n");
        return 1;
    }

    if (!kpc_get_counter_count || kpc_get_counter_count(KPC_MASK) != COUNTERS_COUNT) {
        fprintf(stderr, "checkasm: Unxpected kpc_get_counter_count\n");
        return 1;
    }
    if (!kpc_get_config_count || kpc_get_config_count(KPC_MASK) != CONFIG_COUNT) {
        fprintf(stderr, "checkasm: Unxpected kpc_get_config_count\n");
        return 1;
    }

    config[0] = CPMU_CORE_CYCLE | CFGWORD_EL0A64EN_MASK;

    if (!kpc_set_config || kpc_set_config(KPC_MASK, config)) {
        fprintf(stderr, "checkasm: The kperf API needs to be run as root\n");
        return 1;
    }
    if (!kpc_force_all_ctrs_set || kpc_force_all_ctrs_set(1)) {
        fprintf(stderr, "checkasm: kpc_force_all_ctrs_set failed\n");
        return 1;
    }
    if (!kpc_set_counting || kpc_set_counting(KPC_MASK)) {
        fprintf(stderr, "checkasm: kpc_set_counting failed\n");
        return 1;
    }
    if (!kpc_set_counting || kpc_set_thread_counting(KPC_MASK)) {
        fprintf(stderr, "checkasm: kpc_set_thread_counting failed\n");
        return 1;
    }
    return 0;
}

uint64_t checkasm_kperf_cycles(void) {
    uint64_t counters[COUNTERS_COUNT];
    if (kpc_get_thread_counters(0, COUNTERS_COUNT, counters))
        return -1;

    return counters[0];
}
#endif

static int imax(const int a, const int b)
{
    return a > b ? a : b;
}

/* float compare support code */
typedef union {
    float f;
    uint32_t i;
} intfloat;

static int is_negative(const intfloat u)
{
    return u.i >> 31;
}

int float_near_ulp(const float a, const float b, const unsigned max_ulp)
{
    intfloat x, y;

    x.f = a;
    y.f = b;

    if (is_negative(x) != is_negative(y)) {
        // handle -0.0 == +0.0
        return a == b;
    }

    if (llabs((int64_t)x.i - y.i) <= max_ulp)
        return 1;

    return 0;
}

int float_near_ulp_array(const float *const a, const float *const b,
                         const unsigned max_ulp, const int len)
{
    for (int i = 0; i < len; i++)
        if (!float_near_ulp(a[i], b[i], max_ulp))
            return 0;

    return 1;
}

int float_near_abs_eps(const float a, const float b, const float eps)
{
    return fabsf(a - b) < eps;
}

int float_near_abs_eps_array(const float *const a, const float *const b,
                             const float eps, const int len)
{
    for (int i = 0; i < len; i++)
        if (!float_near_abs_eps(a[i], b[i], eps))
            return 0;

    return 1;
}

int float_near_abs_eps_ulp(const float a, const float b, const float eps,
                           const unsigned max_ulp)
{
    return float_near_ulp(a, b, max_ulp) || float_near_abs_eps(a, b, eps);
}

int float_near_abs_eps_array_ulp(const float *const a, const float *const b,
                                 const float eps, const unsigned max_ulp,
                                 const int len)
{
    for (int i = 0; i < len; i++)
        if (!float_near_abs_eps_ulp(a[i], b[i], eps, max_ulp))
            return 0;

    return 1;
}

int double_near_abs_eps(const double a, const double b, const double eps)
{
    return fabs(a - b) < eps;
}

int double_near_abs_eps_array(const double *const a, const double *const b,
                              const double eps, const unsigned len)
{
    for (unsigned i = 0; i < len; i++)
        if (!double_near_abs_eps(a[i], b[i], eps))
            return 0;

    return 1;
}

/* Print colored text to stderr if the terminal supports it */
static int use_printf_color;
static void color_fprintf(FILE *const f, const int color, const char *const fmt, ...)
{
    va_list arg;

    if (use_printf_color)
        fprintf(f, "\x1b[0;%dm", color);

    va_start(arg, fmt);
    vfprintf(f, fmt, arg);
    va_end(arg);

    if (use_printf_color)
        fprintf(f, "\x1b[0m");
}

static void setup_printf(FILE *const f)
{
#ifdef _WIN32
  #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HANDLE con = GetStdHandle(f == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    DWORD con_mode = 0;
    use_printf_color = con && con != INVALID_HANDLE_VALUE &&
                       GetConsoleMode(con, &con_mode) &&
                       SetConsoleMode(con, con_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  #endif
#else
    if (isatty(f == stderr ? 2 : 1)) {
        const char *const term = getenv("TERM");
        use_printf_color = term && strcmp(term, "dumb");
    }
#endif
}

/* Deallocate a tree */
static void destroy_func_tree(CheckasmFunc *const f)
{
    if (f) {
        CheckasmFuncVersion *v = f->versions.next;
        while (v) {
            CheckasmFuncVersion *next = v->next;
            free(v);
            v = next;
        }

        destroy_func_tree(f->child[0]);
        destroy_func_tree(f->child[1]);
        free(f);
    }
}

/* Allocate a zero-initialized block, clean up and exit on failure */
static void *checkasm_malloc(const size_t size)
{
    void *const ptr = calloc(1, size);
    if (!ptr) {
        fprintf(stderr, "checkasm: malloc failed\n");
        destroy_func_tree(state.funcs);
        exit(1);
    }
    return ptr;
}

/* Get the suffix of the specified cpu flag */
static const char *cpu_suffix(const unsigned cpu)
{
    for (int i = cfg.nb_cpu_flags - 1; i >= 0; i--)
        if (cpu & cfg.cpu_flags[i].flag)
            return cfg.cpu_flags[i].suffix;

    return "c";
}

#ifdef PERF_START
static int cmp_nop(const void *a, const void *b)
{
    return *(const uint16_t*)a - *(const uint16_t*)b;
}

/* Measure the overhead of the timing code (in decicycles) */
static double measure_nop_time(void)
{
    uint16_t nops[10000];
    int nop_sum = 0;

    PERF_SETUP();
    for (int i = 0; i < 10000; i++) {
        uint64_t t;
        PERF_START(t);
        PERF_STOP(t);
        nops[i] = (uint16_t) t;
    }

    qsort(nops, 10000, sizeof(uint16_t), cmp_nop);
    for (int i = 2500; i < 7500; i++)
        nop_sum += nops[i];

    return nop_sum / 5000.0;
}

static double avg_cycles_per_call(const CheckasmFuncVersion *const v)
{
    if (v->iterations) {
        const double cycles = (double)v->cycles / v->iterations - state.nop_time;
        if (cycles > 0.0)
            return cycles / 32.0; /* 32 calls per iteration */
    }
    return 0.0;
}

/* Print benchmark results */
static void print_benchs(const CheckasmFunc *const f)
{
    if (f) {
        print_benchs(f->child[0]);

        const CheckasmFuncVersion *v = &f->versions;
        if (v->iterations) {
            const double baseline = avg_cycles_per_call(v);
            do {
                const int pad_length = 10 + state.max_function_name_length -
                    printf("%s_%s:", f->name, cpu_suffix(v->cpu));
                const double cycles = avg_cycles_per_call(v);
                const double ratio = cycles ? baseline / cycles : 0.0;
                printf("%*.1f (%5.2fx)\n", imax(pad_length, 0), cycles, ratio);
            } while ((v = v->next));
        }

        print_benchs(f->child[1]);
    }
}
#endif

static void print_functions(const CheckasmFunc *const f)
{
    if (f) {
        print_functions(f->child[0]);
        const CheckasmFuncVersion *v = &f->versions;
        printf("%s (%s", f->name, cpu_suffix(v->cpu));
        while ((v = v->next))
            printf(", %s", cpu_suffix(v->cpu));
        printf(")\n");
        print_functions(f->child[1]);
    }
}

#define is_digit(x) ((x) >= '0' && (x) <= '9')

/* ASCIIbetical sort except preserving natural order for numbers */
static int cmp_func_names(const char *a, const char *b)
{
    const char *const start = a;
    int ascii_diff, digit_diff;

    for (; !(ascii_diff = *(const unsigned char*)a -
                          *(const unsigned char*)b) && *a; a++, b++);
    for (; is_digit(*a) && is_digit(*b); a++, b++);

    if (a > start && is_digit(a[-1]) &&
        (digit_diff = is_digit(*a) - is_digit(*b)))
    {
        return digit_diff;
    }

    return ascii_diff;
}

/* Perform a tree rotation in the specified direction and return the new root */
static CheckasmFunc *rotate_tree(CheckasmFunc *const f, const int dir)
{
    CheckasmFunc *const r = f->child[dir^1];
    f->child[dir^1] = r->child[dir];
    r->child[dir] = f;
    r->color = f->color;
    f->color = 0;
    return r;
}

#define is_red(f) ((f) && !(f)->color)

/* Balance a left-leaning red-black tree at the specified node */
static void balance_tree(CheckasmFunc **const root)
{
    CheckasmFunc *const f = *root;

    if (is_red(f->child[0]) && is_red(f->child[1])) {
        f->color ^= 1;
        f->child[0]->color = f->child[1]->color = 1;
    }
    else if (!is_red(f->child[0]) && is_red(f->child[1]))
        *root = rotate_tree(f, 0); /* Rotate left */
    else if (is_red(f->child[0]) && is_red(f->child[0]->child[0]))
        *root = rotate_tree(f, 1); /* Rotate right */
}

/* Get a node with the specified name, creating it if it doesn't exist */
static CheckasmFunc *get_func(CheckasmFunc **const root, const char *const name)
{
    CheckasmFunc *f = *root;

    if (f) {
        /* Search the tree for a matching node */
        const int cmp = cmp_func_names(name, f->name);
        if (cmp) {
            f = get_func(&f->child[cmp > 0], name);

            /* Rebalance the tree on the way up if a new node was inserted */
            if (!f->versions.func)
                balance_tree(root);
        }
    } else {
        /* Allocate and insert a new node into the tree */
        const size_t name_length = strlen(name) + 1;
        f = *root = checkasm_malloc(offsetof(CheckasmFunc, name) + name_length);
        memcpy(f->name, name, name_length);
    }

    return f;
}

checkasm_context checkasm_context_buf;

/* Crash handling: attempt to catch crashes and handle them
 * gracefully instead of just aborting abruptly. */
#ifdef _WIN32
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
static LONG NTAPI signal_handler(EXCEPTION_POINTERS *const e)
{
    if (state.sig == SIG_ATOMIC_MAX) {
        int s;
        switch (e->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            s = SIGFPE;
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
            s = SIGILL;
            break;
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_STACK_OVERFLOW:
            s = SIGSEGV;
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            s = SIGBUS;
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
        }
        state.sig = s;
        checkasm_load_context();
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif
#else
static void signal_handler(int s);

static const struct sigaction signal_handler_act = {
    .sa_handler = signal_handler,
    .sa_flags = SA_RESETHAND,
};

static void signal_handler(const int s)
{
    if (state.sig == SIG_ATOMIC_MAX) {
        state.sig = s;
        sigaction(s, &signal_handler_act, NULL);
        checkasm_load_context();
    }
}
#endif

/* Compares a string with a wildcard pattern. */
static int wildstrcmp(const char *str, const char *pattern)
{
    const char *wild = strchr(pattern, '*');
    if (wild) {
        const size_t len = wild - pattern;
        if (strncmp(str, pattern, len)) return 1;
        while (*++wild == '*');
        if (!*wild) return 0;
        str += len;
        while (*str && wildstrcmp(str, wild)) str++;
        return !*str;
    }
    return strcmp(str, pattern);
}

/* Perform tests and benchmarks for the specified
 * cpu flag if supported by the host */
static void check_cpu_flag(const char *const name, unsigned flag)
{
    const unsigned old_cpu_flag = state.cpu_flag;

    flag |= old_cpu_flag;
    cfg.set_cpu_flags(flag);
    state.cpu_flag = cfg.get_cpu_flags();

    if (!flag || state.cpu_flag != old_cpu_flag) {
        state.cpu_flag_name = name;
        state.suffix_length = (int) strlen(cpu_suffix(flag)) + 1;
        for (int i = 0; i < cfg.nb_tests; i++) {
            if (cfg.test_pattern && wildstrcmp(cfg.tests[i].name, cfg.test_pattern))
                continue;
            xor128_srand(cfg.seed);
            state.current_test_name = cfg.tests[i].name;
            cfg.tests[i].func();
        }
    }
}

/* Print the name of the current CPU flag, but only do it once */
static void print_cpu_name(void)
{
    if (state.cpu_flag_name) {
        color_fprintf(stderr, COLOR_YELLOW, "%s:\n", state.cpu_flag_name);
        state.cpu_flag_name = NULL;
    }
}

static unsigned get_seed(void)
{
#ifdef _WIN32
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.LowPart;
#elif defined(__APPLE__)
    return (unsigned) mach_absolute_time();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned) (1000000000ULL * ts.tv_sec + ts.tv_nsec);
#endif
}

static void set_signal_handlers(void)
{
    static int handlers_set;
    if (handlers_set)
        return;

#ifdef _WIN32
  #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    AddVectoredExceptionHandler(0, signal_handler);
  #endif
#else
    sigaction(SIGBUS,  &signal_handler_act, NULL);
    sigaction(SIGFPE,  &signal_handler_act, NULL);
    sigaction(SIGILL,  &signal_handler_act, NULL);
    sigaction(SIGSEGV, &signal_handler_act, NULL);
#endif

    handlers_set = 1;
}

static int set_cpu_affinity(const uint64_t affinity)
{
    int affinity_err;
    if (!affinity)
        return 0;

#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
  #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    BOOL (WINAPI *spdcs)(HANDLE, const ULONG*, ULONG) =
        (void*)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetProcessDefaultCpuSets");
    if (spdcs)
        affinity_err = !spdcs(process, (ULONG[]){ affinity + 256 }, 1);
    else
  #endif
    {
        if (affinity < sizeof(DWORD_PTR) * 8)
            affinity_err = !SetProcessAffinityMask(process, (DWORD_PTR)1 << affinity);
        else
            affinity_err = 1;
    }
#elif HAVE_PTHREAD_SETAFFINITY_NP && defined(CPU_SET)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(affinity, &set);
    affinity_err = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
#else
    (void) affinity;
    (void) affinity_err;
    fprintf(stderr,
            "checkasm: --affinity is not supported on your system\n");
    return 1;
#endif

    if (affinity_err) {
        fprintf(stderr, "checkasm: invalid cpu affinity (%"PRIx64")\n", affinity);
        return 1;
    } else {
        fprintf(stderr, "checkasm: running on cpu %"PRIx64"\n", affinity);
        return 0;
    }
}

void checkasm_list_cpu_flags(const CheckasmConfig *cfg)
{
    const unsigned cpu_flags = cfg->get_cpu_flags();
    const int last_flag = cfg->nb_cpu_flags - 1;
    setup_printf(stdout);

    for (int i = 0; i < cfg->nb_cpu_flags; i++) {
        if (cfg->cpu_flags[i].flag & cpu_flags)
            color_fprintf(stdout, COLOR_GREEN, "%s", cfg->cpu_flags[i].suffix);
        else
            color_fprintf(stdout, COLOR_RED, "~%s", cfg->cpu_flags[i].suffix);
        printf(i == last_flag ? "\n" : ", ");
    }
}

void checkasm_list_tests(const CheckasmConfig *config)
{
    for (int i = 0; i < config->nb_tests; i++)
        printf("%s\n", config->tests[i].name);
}

int checkasm_run(const CheckasmConfig *config)
{
    assert(config->get_cpu_flags);
    assert(config->set_cpu_flags);
    memset(&state, 0, sizeof(state));
    cfg = *config;

    set_signal_handlers();
    set_cpu_affinity(cfg.cpu_affinity);
    setup_printf(cfg.list_functions ? stdout : stderr);

    if (!cfg.seed)
        cfg.seed = get_seed();

    if (cfg.bench) {
    #if CONFIG_LINUX_PERF
        if (linux_perf_init())
            return 1;
    #elif CONFIG_MACOS_KPERF
        if (kperf_init())
            return 1;
    #elif defined(readtime)
        if (!checkasm_save_context()) {
            checkasm_set_signal_handler_state(1);
            readtime();
            checkasm_set_signal_handler_state(0);
        } else {
            fprintf(stderr, "checkasm: unable to access cycle counter\n");
            return 1;
        }
    #else
        fprintf(stderr, "checkasm: benchmarking not supported on this platform\n");
        return 1;
    #endif
    }

#if ARCH_X86_64
    state.simd_warmup = checkasm_get_simd_warmup_x86();
    checkasm_simd_warmup();
#endif

#if ARCH_ARM
    const unsigned cpu_flags = checkasm_get_cpu_flags_arm();
    void checkasm_checked_call_vfp(void *func, int dummy, ...);
    void checkasm_checked_call_novfp(void *func, int dummy, ...);
    if (cpu_flags & CHECKASM_ARM_CPU_FLAG_NEON)
        checkasm_checked_call_ptr = checkasm_checked_call_vfp;
    else
        checkasm_checked_call_ptr = checkasm_checked_call_novfp;
#endif

#if ARCH_X86
    unsigned checkasm_init_x86(char *name);
    char name[48];
    const unsigned cpuid = checkasm_init_x86(name);
    for (size_t len = strlen(name); len && name[len-1] == ' '; len--)
        name[len-1] = '\0'; /* trim trailing whitespace */
    fprintf(stderr, "checkasm: %s (%08X) using random seed %u\n", name, cpuid, cfg.seed);
#elif ARCH_RISCV
    const unsigned vlen = checkasm_get_vlen();
    char buf[32] = "";
    if (vlen)
        snprintf(buf, sizeof(buf), "VLEN=%i bits, ", vlen);
    fprintf(stderr, "checkasm: %susing random seed %u\n", buf, cfg.seed);
#elif ARCH_AARCH64 && HAVE_SVE
    char buf[48] = "";
    const unsigned sve_len = checkasm_sve_length();
    if (sve_len)
        snprintf(buf, sizeof(buf), "SVE %d bits, ", sve_len);
    fprintf(stderr, "checkasm: %susing random seed %u\n", buf, cfg.seed);
#else
    fprintf(stderr, "checkasm: using random seed %u\n", cfg.seed);
#endif

    check_cpu_flag(NULL, 0);
    for (int i = 0; i < cfg.nb_cpu_flags; i++)
        check_cpu_flag(cfg.cpu_flags[i].name, cfg.cpu_flags[i].flag);

    int ret = 0;
    if (cfg.list_functions) {
        print_functions(state.funcs);
    } else if (state.num_failed) {
        fprintf(stderr, "checkasm: %d of %d tests failed\n",
                state.num_failed, state.num_checked);
        ret = 1;
    } else {
        if (state.num_checked)
            fprintf(stderr, "checkasm: all %d tests passed\n", state.num_checked);
        else
            fprintf(stderr, "checkasm: no tests to perform\n");
#ifdef PERF_START
        if (cfg.bench && state.max_function_name_length) {
            state.nop_time = measure_nop_time();
            if (cfg.verbose)
                printf("nop:%*.1f\n", state.max_function_name_length + 6, state.nop_time);
            print_benchs(state.funcs);
        }
#endif
    }

    destroy_func_tree(state.funcs);
    return ret;
}

/* Decide whether or not the specified function needs to be tested and
 * allocate/initialize data structures if needed. Returns a pointer to a
 * reference function if the function should be tested, otherwise NULL */
void *checkasm_check_func(void *const func, const char *const name, ...)
{
    char name_buf[256];
    va_list arg;

    va_start(arg, name);
    int name_length = vsnprintf(name_buf, sizeof(name_buf), name, arg);
    va_end(arg);

    if (!func || name_length <= 0 || (size_t)name_length >= sizeof(name_buf) ||
        (cfg.function_pattern && wildstrcmp(name_buf, cfg.function_pattern)))
    {
        return NULL;
    }

    state.current_func = get_func(&state.funcs, name_buf);

    state.funcs->color = 1;
    CheckasmFuncVersion *v = &state.current_func->versions;
    void *ref = func;

    if (v->func) {
        CheckasmFuncVersion *prev;
        do {
            /* Only test functions that haven't already been tested */
            if (v->func == func)
                return NULL;

            if (v->ok)
                ref = v->func;

            prev = v;
        } while ((v = v->next));

        v = prev->next = checkasm_malloc(sizeof(CheckasmFuncVersion));
    }

    name_length += state.suffix_length;
    if (name_length > state.max_function_name_length)
        state.max_function_name_length = name_length;

    v->func = func;
    v->ok = 1;
    v->cpu = state.cpu_flag;
    state.current_func_ver = v;
    if (cfg.list_functions) /* Save function names without running tests */
        return NULL;

    xor128_srand(cfg.seed);

    if (state.cpu_flag)
        state.num_checked++;

    return ref;
}

/* Decide whether or not the current function needs to be benchmarked */
int checkasm_bench_func(void)
{
    return !state.num_failed && cfg.bench;
}

/* Indicate that the current test has failed, return whether verbose printing
 * is requested. */
int checkasm_fail_func(const char *const msg, ...)
{
    if (state.current_func_ver && state.current_func_ver->cpu &&
        state.current_func_ver->ok)
    {
        va_list arg;

        print_cpu_name();
        fprintf(stderr, "   %s_%s (", state.current_func->name,
                cpu_suffix(state.current_func_ver->cpu));
        va_start(arg, msg);
        vfprintf(stderr, msg, arg);
        va_end(arg);
        fprintf(stderr, ")\n");

        state.current_func_ver->ok = 0;
        state.num_failed++;
    }
    return cfg.verbose;
}

unsigned checkasm_bench_runs(void)
{
    return cfg.bench_runs ? cfg.bench_runs : 1000;
}

/* Update benchmark results of the current function */
void checkasm_update_bench(const int iterations, const uint64_t cycles)
{
    state.current_func_ver->iterations += iterations;
    state.current_func_ver->cycles += cycles;
}

/* Print the outcome of all tests performed since
 * the last time this function was called */
void checkasm_report(const char *const name, ...)
{
    static int prev_checked, prev_failed;
    static size_t max_length;

    if (state.num_checked > prev_checked) {
        int pad_length = (int) max_length + 4;
        va_list arg;

        print_cpu_name();
        pad_length -= fprintf(stderr, " - %s.", state.current_test_name);
        va_start(arg, name);
        pad_length -= vfprintf(stderr, name, arg);
        va_end(arg);
        fprintf(stderr, "%*c", imax(pad_length, 0) + 2, '[');

        if (state.num_failed == prev_failed)
            color_fprintf(stderr, COLOR_GREEN, "OK");
        else
            color_fprintf(stderr, COLOR_RED, "FAILED");
        fprintf(stderr, "]\n");

        prev_checked = state.num_checked;
        prev_failed  = state.num_failed;
    } else if (!state.cpu_flag) {
        /* Calculate the amount of padding required
         * to make the output vertically aligned */
        size_t length = strlen(state.current_test_name);
        va_list arg;

        va_start(arg, name);
        length += vsnprintf(NULL, 0, name, arg);
        va_end(arg);

        if (length > max_length)
            max_length = length;
    }
}

void checkasm_set_signal_handler_state(const int enabled)
{
    state.sig = enabled ? SIG_ATOMIC_MAX : 0;
}

void checkasm_handle_signal(void)
{
    const int s = state.sig;
    checkasm_fail_func(s == SIGFPE ? "fatal arithmetic error" :
                       s == SIGILL ? "illegal instruction" :
                       s == SIGBUS ? "bus error" :
                                     "segmentation fault");
}

static int check_err(const char *const file, const int line,
                     const char *const name, const int w, const int h,
                     int *const err)
{
    if (*err)
        return 0;
    if (!checkasm_fail_func("%s:%d", file, line))
        return 1;
    *err = 1;
    fprintf(stderr, "%s (%dx%d):\n", name, w, h);
    return 0;
}

#define DEF_CHECKASM_CHECK_BODY(compare, type, fmt) \
do { \
    int aligned_w = (w + align_w - 1) & ~(align_w - 1); \
    int aligned_h = (h + align_h - 1) & ~(align_h - 1); \
    int err = 0; \
    stride1 /= sizeof(*buf1); \
    stride2 /= sizeof(*buf2); \
    int y = 0; \
    for (y = 0; y < h; y++) \
        if (!compare(&buf1[y*stride1], &buf2[y*stride2], w)) \
            break; \
    if (y != h) { \
        if (check_err(file, line, name, w, h, &err)) \
            return 1; \
        for (y = 0; y < h; y++) { \
            for (int x = 0; x < w; x++) \
                fprintf(stderr, " " fmt, buf1[x]); \
            fprintf(stderr, "    "); \
            for (int x = 0; x < w; x++) \
                fprintf(stderr, " " fmt, buf2[x]); \
            fprintf(stderr, "    "); \
            for (int x = 0; x < w; x++) \
                fprintf(stderr, "%c", buf1[x] != buf2[x] ? 'x' : '.'); \
            buf1 += stride1; \
            buf2 += stride2; \
            fprintf(stderr, "\n"); \
        } \
        buf1 -= h*stride1; \
        buf2 -= h*stride2; \
    } \
    for (y = -padding; y < 0; y++) \
        if (!compare(&buf1[y*stride1 - padding], &buf2[y*stride2 - padding], \
                     w + 2*padding)) { \
            if (check_err(file, line, name, w, h, &err)) \
                return 1; \
            fprintf(stderr, " overwrite above\n"); \
            break; \
        } \
    for (y = aligned_h; y < aligned_h + padding; y++) \
        if (!compare(&buf1[y*stride1 - padding], &buf2[y*stride2 - padding], \
                     w + 2*padding)) { \
            if (check_err(file, line, name, w, h, &err)) \
                return 1; \
            fprintf(stderr, " overwrite below\n"); \
            break; \
        } \
    for (y = 0; y < h; y++) \
        if (!compare(&buf1[y*stride1 - padding], &buf2[y*stride2 - padding], \
                     padding)) { \
            if (check_err(file, line, name, w, h, &err)) \
                return 1; \
            fprintf(stderr, " overwrite left\n"); \
            break; \
        } \
    for (y = 0; y < h; y++) \
        if (!compare(&buf1[y*stride1 + aligned_w], &buf2[y*stride2 + aligned_w], \
                     padding)) { \
            if (check_err(file, line, name, w, h, &err)) \
                return 1; \
            fprintf(stderr, " overwrite right\n"); \
            break; \
        } \
    return err; \
} while (0)

#define cmp_int(a, b, len) (!memcmp(a, b, (len) * sizeof(*(a))))
#define DEF_CHECKASM_CHECK_FUNC(type, fmt) \
int checkasm_check_##type(const char *file, int line, \
                          const type *buf1, ptrdiff_t stride1, \
                          const type *buf2, ptrdiff_t stride2, \
                          int w, int h, const char *name, \
                          int align_w, int align_h, \
                          int padding) \
{ \
    DEF_CHECKASM_CHECK_BODY(cmp_int, type, fmt); \
}

DEF_CHECKASM_CHECK_FUNC(int8_t,   "%4d")
DEF_CHECKASM_CHECK_FUNC(int16_t,  "%6d")
DEF_CHECKASM_CHECK_FUNC(int32_t,  "%9d")
DEF_CHECKASM_CHECK_FUNC(uint8_t,  "%02x")
DEF_CHECKASM_CHECK_FUNC(uint16_t, "%04x")
DEF_CHECKASM_CHECK_FUNC(uint32_t, "%08x")

int checkasm_check_float_ulp(const char *file, int line,
                             const float *buf1, ptrdiff_t stride1,
                             const float *buf2, ptrdiff_t stride2,
                             int w, int h, const char *name,
                             unsigned max_ulp, int align_w, int align_h,
                             int padding)
{
    #define cmp_float(a, b, len) float_near_ulp_array(a, b, max_ulp, len)
    DEF_CHECKASM_CHECK_BODY(cmp_float, float, "%g");
    #undef cmp_float
}

#if ARCH_X86_64
void checkasm_simd_warmup(void)
{
    if (state.simd_warmup)
        state.simd_warmup();
}
#endif

#if ARCH_ARM
void (*checkasm_checked_call_ptr)(void *func, int dummy, ...);
#endif
