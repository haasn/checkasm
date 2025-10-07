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

#ifndef CHECKASM_PLATFORM_X86_64_H
#define CHECKASM_PLATFORM_X86_64_H

#include <stdint.h>

#include "checkasm/attributes.h"

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

#if defined(__STDC__) && defined(__STDC_VERSION) && __STDC__ && __STDC_VERSION >= 201112L
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
         void *, unsigned)) (void *) checkasm_checked_call;\
    init_clobber_mask(__VA_ARGS__, void*, void*, void*, void*,\
                      void*, void*, void*, void*, void*, void*,\
                      void*, void*, void*, void*, void*);

#define call_new(...)\
    (checkasm_set_signal_handler_state(1),\
     checkasm_simd_warmup(),\
     checked_call(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8,\
                  7, 6, 5, 4, 3, 2, 1, func_new, clobber_mask));\
    checkasm_set_signal_handler_state(0)

/* x86-64 needs 32- and 64-byte alignment for AVX2 and AVX-512. */
#define ALIGN_64_VAL 64
#define ALIGN_32_VAL 32
#define ALIGN_16_VAL 16

#endif /* CHECKASM_PLATFORM_X86_64_H */
