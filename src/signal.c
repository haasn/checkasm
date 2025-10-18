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

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>

#include "config.h"

#include "internal.h"
#include "test.h"

#ifdef _WIN32
  #include <windows.h>
  #ifndef SIGBUS
    /* non-standard, use the same value as mingw-w64 */
    #define SIGBUS 10
  #endif
#endif

checkasm_jmp_buf checkasm_context;

static volatile sig_atomic_t sig; // SIG_ATOMIC_MAX = signal handling enabled

void checkasm_set_signal_handler_state(const int enabled)
{
    sig = enabled ? SIG_ATOMIC_MAX : 0;
}

#ifdef _WIN32

  #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
static LONG NTAPI signal_handler(EXCEPTION_POINTERS *const e)
{
    if (sig == SIG_ATOMIC_MAX) {
        int s;
        switch (e->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:    s = SIGFPE; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:      s = SIGILL; break;
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_STACK_OVERFLOW:        s = SIGSEGV; break;
        case EXCEPTION_IN_PAGE_ERROR:         s = SIGBUS; break;
        default:                              return EXCEPTION_CONTINUE_SEARCH;
        }
        sig = s;
        checkasm_load_context(checkasm_context);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
  #endif

#else // !_WIN32

static void signal_handler(int s);

static const struct sigaction signal_handler_act = {
    .sa_handler = signal_handler,
    .sa_flags   = SA_RESETHAND,
};

static void signal_handler(const int s)
{
    if (sig == SIG_ATOMIC_MAX) {
        sig = s;
        sigaction(s, &signal_handler_act, NULL);
        checkasm_load_context(checkasm_context);
    }
}
#endif

COLD void checkasm_set_signal_handlers(void)
{
    static int handlers_set;
    if (handlers_set)
        return;

#ifdef _WIN32
  #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    AddVectoredExceptionHandler(0, signal_handler);
  #endif
#else // !_WIN32
    sigaction(SIGBUS, &signal_handler_act, NULL);
    sigaction(SIGFPE, &signal_handler_act, NULL);
    sigaction(SIGILL, &signal_handler_act, NULL);
    sigaction(SIGSEGV, &signal_handler_act, NULL);
#endif

    handlers_set = 1;
}

void checkasm_handle_signal(void)
{
    const int s = sig;
    checkasm_fail_func(s == SIGFPE   ? "fatal arithmetic error"
                       : s == SIGILL ? "illegal instruction"
                       : s == SIGBUS ? "bus error"
                                     : "segmentation fault");
}
