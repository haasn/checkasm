/*
 * Copyright © 2025, Niklas Haas
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CHECKASM_INTERNAL_H
#define CHECKASM_INTERNAL_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
  #include <mach/mach_time.h>
#endif

#include "checkasm/attributes.h"
#include "checkasm/test.h"

#ifdef __GNUC__
  #define COLD __attribute__((cold))
#else
  #define COLD
#endif

#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

#ifdef _MSC_VER
  #define NOINLINE __declspec(noinline)
#elif __has_attribute(noclone)
  #define NOINLINE __attribute__((noinline, noclone))
#else
  #define NOINLINE __attribute__((noinline))
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void checkasm_srand(unsigned seed);

/* Hidden alias without public visibility, for use in asm */
int checkasm_fail_internal(const char *msg, ...) ATTR_FORMAT_PRINTF(1, 2);

#define COLOR_DEFAULT -1
#define COLOR_RED     31
#define COLOR_GREEN   32
#define COLOR_YELLOW  33
#define COLOR_BLUE    34
#define COLOR_MAGENTA 35
#define COLOR_CYAN    36
#define COLOR_WHITE   37

/* Colored variant of fprintf for terminals that support it */
void checkasm_setup_fprintf(FILE *const f);
void checkasm_fprintf(FILE *const f, const int color, const char *const fmt, ...)
    ATTR_FORMAT_PRINTF(3, 4);

/* Light-weight helper for printing nested JSON objects */
typedef struct CheckasmJson {
    FILE *file;
    int   level;
    int   nonempty;
} CheckasmJson;

void checkasm_json(CheckasmJson *json, const char *key, const char *const fmt, ...)
    ATTR_FORMAT_PRINTF(3, 4);
void checkasm_json_str(CheckasmJson *json, const char *key, const char *const str);
void checkasm_json_push(CheckasmJson *json, const char *const key);
void checkasm_json_pop(CheckasmJson *json);

/* Platform specific signal handling */
void checkasm_set_signal_handlers(void);

typedef struct CheckasmVar {
    double lmean, lvar; /* log mean and variance */
} CheckasmVar;

/* Platform specific timing code */
int checkasm_perf_init(void);
int checkasm_perf_init_linux(CheckasmPerf *perf);
int checkasm_perf_init_macos(CheckasmPerf *perf);

CheckasmVar checkasm_measure_nop_cycles(uint64_t target_cycles); /* cycles per iter */
CheckasmVar checkasm_measure_perf_scale(void);                   /* ns per cycle */
void        checkasm_noop(void *);

/* Miscellaneous helpers */
static inline int imax(const int a, const int b)
{
    return a > b ? a : b;
}

static inline int imin(const int a, const int b)
{
    return a < b ? a : b;
}

static inline uint64_t checkasm_gettime_nsec(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    LARGE_INTEGER        ts;
    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ts);
    return UINT64_C(1000000000) * ts.QuadPart / freq.QuadPart;
#elif defined(__APPLE__) && defined(__MACH__)
    static mach_timebase_info_data_t tb_info;
    if (!tb_info.denom) {
        if (mach_timebase_info(&tb_info) != KERN_SUCCESS)
            return -1;
    }
    return mach_absolute_time() * tb_info.numer / tb_info.denom;
#else
    struct timespec ts;
  #ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  #else
    clock_gettime(CLOCK_MONOTONIC, &ts);
  #endif
    return UINT64_C(1000000000) * ts.tv_sec + ts.tv_nsec;
#endif
}

#endif /* CHECKASM_INTERNAL_H */
