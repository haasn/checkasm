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

#ifndef CHECKASM_PLATFORM_X86_32_H
#define CHECKASM_PLATFORM_X86_32_H

#include "checkasm/attributes.h"

CHECKASM_API void checkasm_checked_call_emms(void *func, ...);

#define declare_new(ret, ...)                                                            \
    ret (*checked_call)(void *, __VA_ARGS__, int, int, int, int, int, int, int, int,     \
                        int, int, int, int, int, int, int)                               \
        = (ret (*)(void *, __VA_ARGS__, int, int, int, int, int, int, int, int, int,     \
                   int, int, int, int, int, int))(void *) checkasm_checked_call;         \
    int emms_needed = 0;                                                                 \
    (void) emms_needed;

#define call_new(...)                                                                    \
    (checkasm_set_signal_handler_state(1),                                               \
     checked_call(func_new, __VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, \
                  1));                                                                   \
    checkasm_set_signal_handler_state(0)

#define declare_func_emms(cpu_flags, ret, ...)                                           \
    declare_func(ret, __VA_ARGS__);                                                      \
    if (checkasm_get_cpu_flags() & (cpu_flags)) {                                        \
        checked_call = (ret (*)(void *, __VA_ARGS__, int, int, int, int, int, int, int,  \
                                int, int, int, int, int, int, int,                       \
                                int))(void *) checkasm_checked_call_emms;                \
        emms_needed  = 1;                                                                \
    }

#define checkasm_clear_cpu_state()                                                       \
    do {                                                                                 \
        if (emms_needed)                                                                 \
            __asm__ volatile("emms" ::: "memory");                                       \
    } while (0)

#define CHECKASM_ALIGNMENT 64

#endif /* CHECKASM_PLATFORM_X86_32_H */
