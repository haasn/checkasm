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

#ifndef CHECKASM_TEST_H
#define CHECKASM_TEST_H

#include <stdint.h>

#include "checkasm/attributes.h"
#include "checkasm/config.h"
#include "checkasm/perf.h"

/********************************************
 * Internal checkasm API. Used inside tests *
 ********************************************/

CHECKASM_API int checkasm_rand(void);
#define rnd checkasm_rand

CHECKASM_API void *checkasm_check_func(void *func, const char *name, ...) ATTR_FORMAT_PRINTF(2, 3);
CHECKASM_API int checkasm_bench_func(void);
CHECKASM_API int checkasm_fail_func(const char *msg, ...) ATTR_FORMAT_PRINTF(1, 2);
CHECKASM_API unsigned checkasm_bench_runs(void);
CHECKASM_API void checkasm_update_bench(int iterations, uint64_t cycles);
CHECKASM_API void checkasm_report(const char *name, ...) ATTR_FORMAT_PRINTF(1, 2);
CHECKASM_API void checkasm_set_signal_handler_state(int enabled);
CHECKASM_API void checkasm_handle_signal(void);

/* float compare utilities */
CHECKASM_API int float_near_ulp(float a, float b, unsigned max_ulp);
CHECKASM_API int float_near_abs_eps(float a, float b, float eps);
CHECKASM_API int float_near_abs_eps_ulp(float a, float b, float eps,
                                        unsigned max_ulp);
CHECKASM_API int float_near_ulp_array(const float *a, const float *b,
                                      unsigned max_ulp, int len);
CHECKASM_API int float_near_abs_eps_array(const float *a, const float *b,
                                          float eps, int len);
CHECKASM_API int float_near_abs_eps_array_ulp(const float *a, const float *b,
                                              float eps, unsigned max_ulp,
                                              int len);
CHECKASM_API int double_near_abs_eps(double a, double b, double eps);
CHECKASM_API int double_near_abs_eps_array(const double *a, const double *b,
                                           double eps, unsigned len);

/* Decide whether or not the specified function needs to be tested */
#define check_func(func, ...)\
    (func_ref = checkasm_check_func((func_new = (void *) func), __VA_ARGS__))

/* Declare the function prototype. The first argument is the return value,
 * the remaining arguments are the function parameters. Naming parameters
 * is optional. */
#define declare_func(ret, ...)\
    declare_new(ret, __VA_ARGS__)\
    void *func_ref, *func_new;\
    typedef ret func_type(__VA_ARGS__);\
    checkasm_handle_signal()

/* Indicate that the current test has failed */
#define fail() checkasm_fail_func("%s:%d", __FILE__, __LINE__)

/* Print the test outcome */
#define report checkasm_report

/* Call the reference function */
#define call_ref(...)\
    (checkasm_set_signal_handler_state(1),\
     ((func_type *)func_ref)(__VA_ARGS__));\
    checkasm_set_signal_handler_state(0)

/* Verifies that clobbered callee-saved registers
 * are properly saved and restored */
CHECKASM_API void checkasm_checked_call(void *func, ...);

#if ARCH_X86_64

/* YMM and ZMM registers on x86 are turned off to save power when they haven't
 * been used for some period of time. When they are used there will be a
 * "warmup" period during which performance will be reduced and inconsistent
 * which is problematic when trying to benchmark individual functions. We can
 * work around this by periodically issuing "dummy" instructions that uses
 * those registers to keep them powered on. */
CHECKASM_API void checkasm_simd_warmup(void);

/* The upper 32 bits of 32-bit data types are undefined when passed as function
 * parameters. In practice those bits usually end up being zero which may hide
 * certain bugs, such as using a register containing undefined bits as a pointer
 * offset, so we want to intentionally clobber those bits with junk to expose
 * any issues. The following set of macros automatically calculates a bitmask
 * specifying which parameters should have their upper halves clobbered. */
#ifdef _WIN32
/* Integer and floating-point parameters share "register slots". */
    #define IGNORED_FP_ARGS 0
#else
/* Up to 8 floating-point parameters are passed in XMM registers, which are
 * handled orthogonally from integer parameters passed in GPR registers. */
    #define IGNORED_FP_ARGS 8
#endif

#if HAVE_C11_GENERIC
#define clobber_type(arg) _Generic((void (*)(void*, arg))NULL,\
     void (*)(void*, int32_t ): clobber_mask |= 1 << mpos++,\
     void (*)(void*, uint32_t): clobber_mask |= 1 << mpos++,\
     void (*)(void*, float   ): mpos += (fp_args++ >= IGNORED_FP_ARGS),\
     void (*)(void*, double  ): mpos += (fp_args++ >= IGNORED_FP_ARGS),\
     default:                   mpos++)
#define init_clobber_mask(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, ...)\
    unsigned clobber_mask = 0;\
    {\
        int mpos = 0, fp_args = 0;\
        clobber_type(a); clobber_type(b); clobber_type(c); clobber_type(d);\
        clobber_type(e); clobber_type(f); clobber_type(g); clobber_type(h);\
        clobber_type(i); clobber_type(j); clobber_type(k); clobber_type(l);\
        clobber_type(m); clobber_type(n); clobber_type(o); clobber_type(p);\
    }
#else
    /* Skip parameter clobbering on compilers without support for _Generic() */
    #define init_clobber_mask(...) unsigned clobber_mask = 0
#endif

#define declare_new(ret, ...)\
    ret (*checked_call)(__VA_ARGS__, int, int, int, int, int, int, int,\
                        int, int, int, int, int, int, int, int, int,\
                        void*, unsigned) =\
        (ret (*)(__VA_ARGS__, int, int, int, int, int, int, int,\
         int, int, int, int, int, int, int, int, int,\
         void *, unsigned))checkasm_checked_call;\
    init_clobber_mask(__VA_ARGS__, void*, void*, void*, void*,\
                      void*, void*, void*, void*, void*, void*,\
                      void*, void*, void*, void*, void*);
#define call_new(...)\
    (checkasm_set_signal_handler_state(1),\
     checkasm_simd_warmup(),\
     checked_call(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8,\
                  7, 6, 5, 4, 3, 2, 1, func_new, clobber_mask));\
    checkasm_set_signal_handler_state(0)

#elif ARCH_X86_32
    #define declare_new(ret, ...)\
        ret (*checked_call)(void *, __VA_ARGS__, int, int, int, int, int, int,\
                            int, int, int, int, int, int, int, int, int) =\
            (ret (*)(void *, __VA_ARGS__, int, int, int, int, int, int,\
                     int, int, int, int, int, int, int, int, int))checkasm_checked_call;\
    #define call_new(...)\
        (checkasm_set_signal_handler_state(1),\
        checked_call(func_new, __VA_ARGS__, 15, 14, 13, 12,\
                    11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1));\
        checkasm_set_signal_handler_state(0)
#elif ARCH_ARM
    /* Use a dummy argument, to offset the real parameters by 2, not only 1.
    * This makes sure that potential 8-byte-alignment of parameters is kept
    * the same even when the extra parameters have been removed. */
    CHECKASM_API extern void (*checkasm_checked_call_ptr)(void *func, int dummy, ...);
    #define declare_new(ret, ...)\
        ret (*checked_call)(void *, int dummy, __VA_ARGS__,\
                            int, int, int, int, int, int, int, int,\
                            int, int, int, int, int, int, int) =\
        (ret (*)(void *, int, __VA_ARGS__, \
                 int, int, int, int, int, int, int, int,\
                 int, int, int, int, int, int, int))checkasm_checked_call_ptr;
    #define call_new(...)\
        (checkasm_set_signal_handler_state(1),\
        checked_call(func_new, 0, __VA_ARGS__, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0));\
        checkasm_set_signal_handler_state(0)
#elif ARCH_AARCH64 && !defined(__APPLE__)
    CHECKASM_API void checkasm_stack_clobber(uint64_t clobber, ...);
    #define declare_new(ret, ...)\
        ret (*checked_call)(void *, int, int, int, int, int, int, int,\
                            __VA_ARGS__, int, int, int, int, int, int, int, int,\
                            int, int, int, int, int, int, int) =\
        (ret (*)(void *, int, int, int, int, int, int, int,\
                 __VA_ARGS__, int, int, int, int, int, int, int, int,\
                 int, int, int, int, int, int, int))checkasm_checked_call;
    #define CLOB (UINT64_C(0xdeadbeefdeadbeef))
    #define call_new(...)\
        (checkasm_set_signal_handler_state(1),\
        checkasm_stack_clobber(CLOB, CLOB, CLOB, CLOB, CLOB, CLOB,\
                                CLOB, CLOB, CLOB, CLOB, CLOB, CLOB,\
                                CLOB, CLOB, CLOB, CLOB, CLOB, CLOB,\
                                CLOB, CLOB, CLOB, CLOB, CLOB),\
        checked_call(func_new, 0, 0, 0, 0, 0, 0, 0, __VA_ARGS__,\
                    7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0));\
        checkasm_set_signal_handler_state(0)
#elif ARCH_RISCV
    #define declare_new(ret, ...)\
        ret (*checked_call)(void *, int, int, int, int, int, int, int,\
                            __VA_ARGS__, int, int, int, int, int, int, int, int,\
                            int, int, int, int, int, int, int) =\
        (ret (*)(void *, int, int, int, int, int, int, int,\
                 __VA_ARGS__, int, int, int, int, int, int, int, int,\
                 int, int, int, int, int, int, int))checkasm_checked_call;
    #define call_new(...)\
        (checkasm_set_signal_handler_state(1),\
        checked_call(func_new, 0, 0, 0, 0, 0, 0, 0, __VA_ARGS__,\
                    7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0));\
        checkasm_set_signal_handler_state(0)
#elif ARCH_LOONGARCH
    #define declare_new(ret, ...)\
        ret (*checked_call)(void *, int, int, int, int, int, int, int,\
                            __VA_ARGS__, int, int, int, int, int, int, int, int,\
                            int, int, int, int, int, int, int) =\
        (ret (*)(void *, int, int, int, int, int, int, int,\
                 __VA_ARGS__, int, int, int, int, int, int, int, int,\
                 int, int, int, int, int, int, int))checkasm_checked_call;
    #define call_new(...)\
        (checkasm_set_signal_handler_state(1),\
        checked_call(func_new, 0, 0, 0, 0, 0, 0, 0, __VA_ARGS__,\
                    7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0));\
        checkasm_set_signal_handler_state(0)
#else
    #define declare_new(ret, ...)
    #define call_new(...)\
        (checkasm_set_signal_handler_state(1),\
        ((func_type *)func_new)(__VA_ARGS__));\
        checkasm_set_signal_handler_state(0)
#endif

#define CALL4(...)\
    do {\
        talt = 0;\
        tfunc(__VA_ARGS__); \
        talt = 1;\
        tfunc(__VA_ARGS__); \
        talt = 0;\
        tfunc(__VA_ARGS__); \
        talt = 1;\
        tfunc(__VA_ARGS__); \
    } while (0)

#define CALL16(...)\
    do {\
        CALL4(__VA_ARGS__); \
        CALL4(__VA_ARGS__); \
        CALL4(__VA_ARGS__); \
        CALL4(__VA_ARGS__); \
    } while (0)

/* Benchmark the function */
#ifdef PERF_START
#define bench_new(...)\
    do {\
        if (checkasm_bench_func()) {\
            func_type *const tfunc = (func_type *) func_new;\
            checkasm_set_signal_handler_state(1);\
            uint64_t tsum = 0;\
            int tcount = 0;\
            PERF_SETUP();\
            unsigned truns = checkasm_bench_runs() >> 3;\
            if (!truns)\
                truns = 1;\
            for (unsigned ti = 0; ti < truns; ti++) {\
                uint64_t t;\
                int talt; (void)talt;\
                PERF_START(t);\
                CALL16(__VA_ARGS__);\
                CALL16(__VA_ARGS__);\
                PERF_STOP(t);\
                if (t*tcount <= tsum*4 && ti > 0) {\
                    tsum += t;\
                    tcount++;\
                }\
            }\
            checkasm_set_signal_handler_state(0);\
            checkasm_update_bench(tcount, tsum);\
        } else {\
            const int talt = 0; (void)talt;\
            call_new(__VA_ARGS__);\
        }\
    } while (0)
#else
#define bench_new(...) do {} while (0)
#endif

/* Alternates between two pointers. Intended to be used within bench_new()
 * calls for functions which modifies their input buffer(s) to ensure that
 * throughput, and not latency, is measured. */
#define alternate(a, b) (talt ? (b) : (a))

#define ROUND_UP(x,a) (((x)+((a)-1)) & ~((a)-1))
#define PIXEL_RECT(name, w, h) \
    ALIGN_STK(pixel, name##_buf, ((h)+32)*(ROUND_UP(w,64)+64) + 64,); \
    ptrdiff_t name##_stride = sizeof(pixel)*(ROUND_UP(w,64)+64); \
    (void)name##_stride; \
    pixel *name = name##_buf + (ROUND_UP(w,64)+64)*16 + 64

#define CLEAR_PIXEL_RECT(name) \
    memset(name##_buf, 0x99, sizeof(name##_buf)) \

#define DECL_CHECKASM_CHECK_FUNC(type) \
CHECKASM_API int checkasm_check_##type(const char *const file, const int line, \
                                       const type *const buf1, const ptrdiff_t stride1, \
                                       const type *const buf2, const ptrdiff_t stride2, \
                                       const int w, const int h, const char *const name, \
                                       const int align_w, const int align_h, \
                                       const int padding)

DECL_CHECKASM_CHECK_FUNC(int8_t);
DECL_CHECKASM_CHECK_FUNC(int16_t);
DECL_CHECKASM_CHECK_FUNC(int32_t);
DECL_CHECKASM_CHECK_FUNC(uint8_t);
DECL_CHECKASM_CHECK_FUNC(uint16_t);
DECL_CHECKASM_CHECK_FUNC(uint32_t);

CHECKASM_API int checkasm_check_float_ulp(const char *file, int line,
                                          const float *buf1, ptrdiff_t stride1,
                                          const float *buf2, ptrdiff_t stride2,
                                          int w, int h, const char *name,
                                          unsigned max_ulp, int align_w, int align_h,
                                          int padding);

#define CONCAT2(a,b) a ## b
#define CONCAT(a,b) CONCAT2(a, b)

#define checkasm_check2(prefix, ...) CONCAT(checkasm_check_, prefix)(__FILE__, __LINE__, __VA_ARGS__)
#define checkasm_check(prefix, ...) checkasm_check2(prefix, __VA_ARGS__, 0, 0, 0)

#ifdef BITDEPTH
    #define checkasm_check_pixel(...) checkasm_check(PIXEL_TYPE, __VA_ARGS__)
    #define checkasm_check_pixel_padded(...) checkasm_check2(PIXEL_TYPE, __VA_ARGS__, 1, 1, 8)
    #define checkasm_check_pixel_padded_align(...) checkasm_check2(PIXEL_TYPE, __VA_ARGS__, 8)
    #define checkasm_check_coef(...)  checkasm_check(COEF_TYPE,  __VA_ARGS__)
#endif

#endif /* CHECKASM_TEST_H */
