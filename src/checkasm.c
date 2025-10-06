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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_internal.h"

#include "checkasm.h"
#include "cpu.h"
#include "internal.h"
#include "test.h"

#ifndef _WIN32
    #if HAVE_PTHREAD_SETAFFINITY_NP
        #include <pthread.h>
        #if HAVE_PTHREAD_NP_H
            #include <pthread_np.h>
        #endif
    #endif
#endif

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
    int suffix_length;
    int max_function_name_length;
#if ARCH_X86_64
    void (*simd_warmup)(void);
#endif
} state;

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
                const double cycles = avg_cycles_per_call(v);
                const double ratio = cycles ? baseline / cycles : 0.0;
                if (cfg.separator) {
                    printf("%s%c%s%c%.1f\n", f->name, cfg.separator,
                           cpu_suffix(v->cpu), cfg.separator, cycles);
                } else {
                    const int pad = 10 + state.max_function_name_length -
                        printf("%s_%s:", f->name, cpu_suffix(v->cpu));
                    printf("%*.1f (%5.2fx)\n", imax(pad, 0), cycles, ratio);
                }
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
            checkasm_srand(cfg.seed);
            state.current_test_name = cfg.tests[i].name;
            cfg.tests[i].func();
        }
    }
}

/* Print the name of the current CPU flag, but only do it once */
static void print_cpu_name(void)
{
    if (state.cpu_flag_name) {
        checkasm_fprintf(stderr, COLOR_YELLOW, "%s:\n", state.cpu_flag_name);
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
    checkasm_setup_fprintf(stdout);

    for (int i = 0; i < cfg->nb_cpu_flags; i++) {
        if (cfg->cpu_flags[i].flag & cpu_flags)
            checkasm_fprintf(stdout, COLOR_GREEN, "%s", cfg->cpu_flags[i].suffix);
        else
            checkasm_fprintf(stdout, COLOR_RED, "~%s", cfg->cpu_flags[i].suffix);
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

    checkasm_set_signal_handlers();
    set_cpu_affinity(cfg.cpu_affinity);
    checkasm_setup_fprintf(cfg.list_functions ? stdout : stderr);

    if (!cfg.seed)
        cfg.seed = get_seed();

    if (cfg.bench && checkasm_perf_init())
        return 1;

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
            state.nop_time = checkasm_measure_nop_time();
            if (cfg.verbose) {
                if (cfg.separator)
                    printf("nop%c%c%.1f\n", cfg.separator, cfg.separator, state.nop_time);
                else
                    printf("nop:%*.1f\n", state.max_function_name_length + 6, state.nop_time);
            }
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

    checkasm_srand(cfg.seed);

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
            checkasm_fprintf(stderr, COLOR_GREEN, "OK");
        else
            checkasm_fprintf(stderr, COLOR_RED, "FAILED");
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

static void print_usage(const char *const progname)
{
    fprintf(stderr,
            "Usage: %s [options...] <random seed>\n"
            "    <random seed>              Use fixed value to seed the PRNG\n"
            "Options:\n"
            "    --affinity=<cpu>           Run the process on CPU <cpu>\n"
            "    --bench -b                 Benchmark the tested functions\n"
            "    --csv, --tsv               Output results in rows of comma or tab separated values.\n"
            "    --function=<pattern> -f    Test only the functions matching <pattern>\n"
            "    --help -h                  Print this usage info\n"
            "    --list-cpuflags            List available cpu flags\n"
            "    --list-functions           List available functions\n"
            "    --list-tests               List available tests\n"
            "    --runs=<shift>             Number of benchmark iterations to run (log 2)\n"
            "    --test=<pattern> -t        Test only <pattern>\n"
            "    --verbose -v               Print verbose output on failure\n",
            progname);
}

static int parseu(unsigned *const dst, const char *const str, const int base)
{
    unsigned long val;
    char *end;
    errno = 0;
    val = strtoul(str, &end, base);
    if (errno || end == str || *end || val > (unsigned) -1)
        return 0;
    *dst = val;
    return 1;
}

int checkasm_main(CheckasmConfig *config, int argc, const char *argv[])
{
    while (argc > 1) {
        if (!strncmp(argv[1], "--help", 6) || !strcmp(argv[1], "-h")) {
            print_usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[1], "--list-cpuflags")) {
            checkasm_list_cpu_flags(config);
            return 0;
        } else if (!strcmp(argv[1], "--list-tests")) {
            checkasm_list_tests(config);
            return 0;
        } else if (!strcmp(argv[1], "--list-functions")) {
            config->list_functions = 1;
        } else if (!strcmp(argv[1], "--bench") || !strcmp(argv[1], "-b")) {
            config->bench = 1;
        } else if (!strcmp(argv[1], "--csv")) {
            config->separator = ',';
        } else if (!strcmp(argv[1], "--tsv")) {
            config->separator = '\t';
        } else if (!strncmp(argv[1], "--runs=", 7)) {
            const char *const s = argv[1] + 7;
            unsigned runs_log2;
            if (!parseu(&runs_log2, s, 10) || runs_log2 > 31) {
                fprintf(stderr, "checkasm: invalid number of runs (1 << %s)\n", s);
                print_usage(argv[0]);
                return 1;
            }
            config->bench_runs = 1u << runs_log2;
        } else if (!strncmp(argv[1], "--test=", 7)) {
            config->test_pattern = argv[1] + 7;
        } else if (!strcmp(argv[1], "-t")) {
            config->test_pattern = argc > 1 ? argv[2] : "";
            argc--;
            argv++;
        } else if (!strncmp(argv[1], "--function=", 11)) {
            config->function_pattern = argv[1] + 11;
        } else if (!strcmp(argv[1], "-f")) {
            config->function_pattern = argc > 1 ? argv[2] : "";
            argc--;
            argv++;
        } else if (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-v")) {
            config->verbose = 1;
        } else if (!strncmp(argv[1], "--affinity=", 11)) {
            const char *const s = argv[1] + 11;
            if (!parseu(&config->cpu_affinity, s, 16)) {
                fprintf(stderr, "checkasm: invalid cpu affinity (%s)\n", s);
                print_usage(argv[0]);
                return 1;
            }
        } else {
            if (!parseu(&config->seed, argv[1], 10)) {
                fprintf(stderr, "checkasm: unknown option (%s)\n", argv[1]);
                print_usage(argv[0]);
                return 1;
            }
        }

        argc--;
        argv++;
    }

    return checkasm_run(config);
}
