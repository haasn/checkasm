/*
 * Copyright © 2025, Niklas Haas
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

#ifndef CHECKASM_TEST_H
#define CHECKASM_TEST_H

#include <stdint.h>

#include "checkasm/attributes.h"
#include "checkasm/checkasm.h"
#include "checkasm/perf.h"
#include "checkasm/platform.h"

/********************************************
 * Internal checkasm API. Used inside tests *
 ********************************************/

/* Decide whether or not the specified function version needs to be tested,
 * identified by an arbitrary nonzero key. Returns the key identifying
 * the reference version, or 0 if this check should not execute. */
CHECKASM_API CheckasmKey checkasm_check_key(CheckasmKey version, const char *name, ...)
    CHECKASM_PRINTF(2, 3);

CHECKASM_API int  checkasm_fail_func(const char *msg, ...) CHECKASM_PRINTF(1, 2);
CHECKASM_API void checkasm_report(const char *name, ...) CHECKASM_PRINTF(1, 2);
CHECKASM_API void checkasm_set_signal_handler_state(int enabled);

/* Call an arbitrary function while handling signals. Use checkasm_call_checked()
 * instead when testing new/asm function versions. */
#define checkasm_call(func, ...)                                                         \
    (checkasm_set_signal_handler_state(1), (func) (__VA_ARGS__));                        \
    checkasm_set_signal_handler_state(0)

/* Call an assembly function (matching the signature declared by declare_new()),
 * while handling signals and common assembly errors. */
#ifndef checkasm_call_checked
  #define checkasm_call_checked(func, ...) checkasm_call((func_type *) func, __VA_ARGS__)
#endif

/* Mark a block of tests as expected to fail when any of `cpu_flags` are set
 * (or -1 to always expect a failure). Returns whether or not these functions
 * should be executed. (If not, such tests should be silently skipped.)
 *
 * Note: All functions inside such a block *must* fail, otherwise the whole
 * test will be considered failed.
 */
CHECKASM_API int checkasm_should_fail(CheckasmCpu cpu_flags);

/* Wrapper around `checkasm_check` for normal, callable function pointers */
static void *checkasm_func_ref;
static void *checkasm_func_new;
#define func_ref (*(func_type **) &checkasm_func_ref)
#define func_new (*(func_type **) &checkasm_func_new)

#define checkasm_check_func(func, ...)                                                   \
    (checkasm_func_ref = (void *) checkasm_check_key(                                    \
         (CheckasmKey) (checkasm_func_new = func), __VA_ARGS__))

#define checkasm_call_ref(...) checkasm_call((func_type *) checkasm_func_ref, __VA_ARGS__)
#define checkasm_call_new(...)                                                           \
    checkasm_call_checked((func_type *) checkasm_func_new, __VA_ARGS__)

/* Short-hand aliases */
#define check_key  checkasm_check_key
#define check_func checkasm_check_func
#define call_ref   checkasm_call_ref
#define call_new   checkasm_call_new

/* Declare the function prototype (for checked calls). The first argument is
 * the return value, the remaining arguments are the function parameters.
 * Naming parameters is optional. */
#define declare_func(ret, ...)                                                           \
    declare_new(ret, __VA_ARGS__);                                                       \
    typedef ret func_type(__VA_ARGS__)

#ifndef declare_func_emms
  #define declare_func_emms(cpu_flags, ret, ...) declare_func(ret, __VA_ARGS__)
#endif

/* Indicate that the current test has failed */
#define fail() checkasm_fail_func("%s:%d", __FILE__, __LINE__)

/* Print the test outcome */
#define report checkasm_report

/* Verifies that clobbered callee-saved registers
 * are properly saved and restored */
CHECKASM_API void checkasm_checked_call(void *func, ...);

/* Clears any processor state possible left over after running a function */
#ifndef checkasm_clear_cpu_state
  #define checkasm_clear_cpu_state()                                                     \
      do {                                                                               \
      } while (0)
#endif

typedef struct CheckasmPerf {
    /* Start/stop callbacks, used whenever not using inline ASM */
    uint64_t (*start)(void);
    uint64_t (*stop)(uint64_t);
    const char *name, *unit;

#ifdef CHECKASM_PERF_ASM
    int asm_usable; /* Whether the ASM instruction is usable */
#endif
} CheckasmPerf;

CHECKASM_API const CheckasmPerf *checkasm_get_perf(void);

#define CHECKASM_PERF_CALL4(...)                                                         \
    do {                                                                                 \
        int tidx = 0;                                                                    \
        bench_func(__VA_ARGS__);                                                         \
        tidx = 1;                                                                        \
        bench_func(__VA_ARGS__);                                                         \
        tidx = 2;                                                                        \
        bench_func(__VA_ARGS__);                                                         \
        tidx = 3;                                                                        \
        bench_func(__VA_ARGS__);                                                         \
        (void) tidx;                                                                     \
    } while (0)

#define CHECKASM_PERF_CALL16(...)                                                        \
    do {                                                                                 \
        CHECKASM_PERF_CALL4(__VA_ARGS__);                                                \
        CHECKASM_PERF_CALL4(__VA_ARGS__);                                                \
        CHECKASM_PERF_CALL4(__VA_ARGS__);                                                \
        CHECKASM_PERF_CALL4(__VA_ARGS__);                                                \
    } while (0)

/* Naive loop; used when perf.start/stop() is expected to be slow or imprecise, or when
 * we have no ASM cycle counters, or when the number of iterations is low */
#define CHECKASM_PERF_BENCH_SIMPLE(count, time, ...)                                     \
    do {                                                                                 \
        time = perf.start();                                                             \
        for (int tidx = 0; tidx < count; tidx++)                                         \
            bench_func(__VA_ARGS__);                                                     \
        time = perf.stop(time);                                                          \
    } while (0)

/* Unrolled loop with inline outlier rejection; used when we have asm cycle counters */
#define CHECKASM_PERF_BENCH_ASM(total_count, time, ...)                                  \
    do {                                                                                 \
        int      tcount_trim = 0;                                                        \
        uint64_t tsum_trim   = 0;                                                        \
        for (int titer = 0; titer < total_count; titer += 32) {                          \
            uint64_t t = CHECKASM_PERF_ASM();                                            \
            CHECKASM_PERF_CALL16(__VA_ARGS__);                                           \
            CHECKASM_PERF_CALL16(__VA_ARGS__);                                           \
            t = CHECKASM_PERF_ASM() - t;                                                 \
            if (t * tcount_trim <= tsum_trim * 4 && (titer > 0 || total_count < 1000)) { \
                tsum_trim += t;                                                          \
                tcount_trim++;                                                           \
            }                                                                            \
        }                                                                                \
        time        = tsum_trim;                                                         \
        total_count = tcount_trim << 5;                                                  \
    } while (0)

/* Select the best benchmarking method at runtime */
#ifdef CHECKASM_PERF_ASM
  #ifndef CHECKASM_PERF_ASM_USABLE
    #define CHECKASM_PERF_ASM_USABLE perf.asm_usable
  #endif
  #define CHECKASM_PERF_BENCH(count, time, ...)                                          \
      do {                                                                               \
          const CheckasmPerf perf = *checkasm_get_perf();                                \
          if (CHECKASM_PERF_ASM_USABLE && count >= 128) {                                \
              CHECKASM_PERF_BENCH_ASM(count, time, __VA_ARGS__);                         \
          } else {                                                                       \
              CHECKASM_PERF_BENCH_SIMPLE(count, time, __VA_ARGS__);                      \
          }                                                                              \
      } while (0)
#else /* !CHECKASM_PERF_ASM */
  #define CHECKASM_PERF_BENCH(count, time, ...)                                          \
      do {                                                                               \
          const CheckasmPerf perf = *checkasm_get_perf();                                \
          CHECKASM_PERF_BENCH_SIMPLE(count, time, __VA_ARGS__);                          \
      } while (0)
#endif

/* Benchmark the function */
CHECKASM_API int  checkasm_bench_func(void);
CHECKASM_API int  checkasm_bench_runs(void);
CHECKASM_API void checkasm_bench_update(int iterations, uint64_t cycles);
CHECKASM_API void checkasm_bench_finish(void);

#define bench_new(...)                                                                   \
    do {                                                                                 \
        if (checkasm_bench_func()) {                                                     \
            func_type *const bench_func = checkasm_func_new;                             \
            checkasm_set_signal_handler_state(1);                                        \
            for (int truns; (truns = checkasm_bench_runs());) {                          \
                uint64_t time;                                                           \
                CHECKASM_PERF_BENCH(truns, time, __VA_ARGS__);                           \
                checkasm_clear_cpu_state();                                              \
                checkasm_bench_update(truns, time);                                      \
            }                                                                            \
            checkasm_set_signal_handler_state(0);                                        \
            checkasm_bench_finish();                                                     \
        } else {                                                                         \
            const int tidx = 0;                                                          \
            (void) tidx;                                                                 \
            call_new(__VA_ARGS__);                                                       \
        }                                                                                \
    } while (0)

/* Alternates between two pointers. Intended to be used within bench_new()
 * calls for functions which modifies their input buffer(s) to ensure that
 * throughput, and not latency, is measured. */
#define alternate(a, b) ((tidx & 1) ? (b) : (a))

/* Suppress unused variable warnings */
static inline void checkasm_unused(void)
{
    (void) checkasm_func_ref;
    (void) checkasm_func_new;
}

#endif /* CHECKASM_TEST_H */
