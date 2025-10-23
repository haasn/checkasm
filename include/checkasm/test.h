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
#include "checkasm/longjmp.h"
#include "checkasm/perf.h"
#include "checkasm/platform.h"

/********************************************
 * Internal checkasm API. Used inside tests *
 ********************************************/

CHECKASM_API void *checkasm_check_func(void *func, const char *name, ...)
    ATTR_FORMAT_PRINTF(2, 3);
CHECKASM_API int  checkasm_fail_func(const char *msg, ...) ATTR_FORMAT_PRINTF(1, 2);
CHECKASM_API void checkasm_report(const char *name, ...) ATTR_FORMAT_PRINTF(1, 2);
CHECKASM_API void checkasm_set_signal_handler_state(int enabled);
CHECKASM_API void checkasm_handle_signal(void);
CHECKASM_API extern checkasm_jmp_buf checkasm_context;

/* Mark a block of tests as expected to fail. If this is set, at least
 * one failure must be detected in between each report() call, otherwise
 * the whole test will be marked as failed. This state does not persist
 * between tests. */
CHECKASM_API void checkasm_should_fail(int);

/* Decide whether or not the specified function needs to be tested */
#define check_func(func, ...)                                                            \
    (func_ref = (func_type *) checkasm_check_func((func_new = func), __VA_ARGS__))

/* Declare the function prototype. The first argument is the return value,
 * the remaining arguments are the function parameters. Naming parameters
 * is optional. */
#define declare_func(ret, ...)                                                           \
    declare_new(ret, __VA_ARGS__);                                                       \
    typedef ret func_type(__VA_ARGS__);                                                  \
    func_type  *func_ref, *func_new;                                                     \
    if (checkasm_save_context(checkasm_context))                                         \
        checkasm_handle_signal();

/* Indicate that the current test has failed */
#define fail() checkasm_fail_func("%s:%d", __FILE__, __LINE__)

/* Print the test outcome */
#define report checkasm_report

/* Call the reference function */
#define call_ref(...)                                                                    \
    (checkasm_set_signal_handler_state(1), func_ref(__VA_ARGS__));                       \
    checkasm_set_signal_handler_state(0)

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
    /* Fallback functions for when ASM is not available */
    uint64_t (*start)(void);
    uint64_t (*stop)(uint64_t);
    const char *name, *unit;
} CheckasmPerf;

CHECKASM_API extern CheckasmPerf checkasm_perf;

#ifndef CHECKASM_PERF_START
  #define CHECKASM_PERF_SETUP()  const CheckasmPerf perf = checkasm_perf;
  #define CHECKASM_PERF_START(t) t = perf.start()
  #define CHECKASM_PERF_STOP(t)  t = perf.stop(t)
  #define CHECKASM_PERF_NAME     checkasm_perf.name
  #define CHECKASM_PERF_UNIT     checkasm_perf.unit
#endif

#define CALL4(...)                                                                       \
    do {                                                                                 \
        talt = 0;                                                                        \
        func_new(__VA_ARGS__);                                                           \
        talt = 1;                                                                        \
        func_new(__VA_ARGS__);                                                           \
        talt = 0;                                                                        \
        func_new(__VA_ARGS__);                                                           \
        talt = 1;                                                                        \
        func_new(__VA_ARGS__);                                                           \
    } while (0)

#define CALL16(...)                                                                      \
    do {                                                                                 \
        CALL4(__VA_ARGS__);                                                              \
        CALL4(__VA_ARGS__);                                                              \
        CALL4(__VA_ARGS__);                                                              \
        CALL4(__VA_ARGS__);                                                              \
    } while (0)

/* Benchmark the function */
CHECKASM_API int  checkasm_bench_func(void);
CHECKASM_API int  checkasm_bench_runs(void);
CHECKASM_API void checkasm_bench_update(int iterations, uint64_t cycles);
CHECKASM_API void checkasm_bench_finish(void);

#define bench_new(...)                                                                   \
    do {                                                                                 \
        if (checkasm_bench_func()) {                                                     \
            checkasm_set_signal_handler_state(1);                                        \
            CHECKASM_PERF_SETUP();                                                       \
            for (int truns; (truns = checkasm_bench_runs());) {                          \
                uint64_t tsum   = 0;                                                     \
                int      tcount = 0;                                                     \
                for (int ti = 0; ti < truns; ti++) {                                     \
                    uint64_t t;                                                          \
                    int      talt;                                                       \
                    (void) talt;                                                         \
                    CHECKASM_PERF_START(t);                                              \
                    CALL16(__VA_ARGS__);                                                 \
                    CALL16(__VA_ARGS__);                                                 \
                    CHECKASM_PERF_STOP(t);                                               \
                    if (t * tcount <= tsum * 4 && (ti > 0 || truns < 50)) {              \
                        tsum += t;                                                       \
                        tcount++;                                                        \
                    }                                                                    \
                }                                                                        \
                checkasm_clear_cpu_state();                                              \
                checkasm_bench_update(tcount, tsum);                                     \
            }                                                                            \
            checkasm_set_signal_handler_state(0);                                        \
            checkasm_bench_finish();                                                     \
        } else {                                                                         \
            const int talt = 0;                                                          \
            (void) talt;                                                                 \
            call_new(__VA_ARGS__);                                                       \
        }                                                                                \
    } while (0)

/* Alternates between two pointers. Intended to be used within bench_new()
 * calls for functions which modifies their input buffer(s) to ensure that
 * throughput, and not latency, is measured. */
#define alternate(a, b) (talt ? (b) : (a))

#endif /* CHECKASM_TEST_H */
