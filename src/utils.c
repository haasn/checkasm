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

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#ifdef _WIN32
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x04
    #endif
#endif

#include "checkasm/test.h"
#include "internal.h"

// xor128 from Marsaglia, George (July 2003). "Xorshift RNGs".
//             Journal of Statistical Software. 8 (14).
//             doi:10.18637/jss.v008.i14.
static uint32_t xs_state[4];

void checkasm_srand(unsigned seed) {
    xs_state[0] = seed;
    xs_state[1] = ( seed & 0xffff0000) | (~seed & 0x0000ffff);
    xs_state[2] = (~seed & 0xffff0000) | ( seed & 0x0000ffff);
    xs_state[3] = ~seed;
}

int checkasm_rand(void) {
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

void checkasm_randomize(void *bufp, size_t bytes)
{
    uint8_t *buf = bufp;
    while (bytes--)
        *buf++ = checkasm_rand() & 0xFF;
}

void checkasm_randomize_mask8(uint8_t *buf, int width, uint8_t mask)
{
    while (width--)
        *buf++ = checkasm_rand() & mask;
}

void checkasm_randomize_mask16(uint16_t *buf, int width, uint16_t mask)
{
    while (width--)
        *buf++ = checkasm_rand() & mask;
}

void checkasm_clear(void *buf, size_t bytes)
{
    memset(buf, 0xAA, bytes);
}

void checkasm_clear8(uint8_t *buf, int width, uint8_t val)
{
    memset(buf, val, width);
}

void checkasm_clear16(uint16_t *buf, int width, uint16_t val)
{
    while (width--)
        *buf++ = val;
}

/* Print colored text to stderr if the terminal supports it */
static int use_printf_color;
void checkasm_fprintf(FILE *const f, const int color, const char *const fmt, ...)
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

COLD void checkasm_setup_fprintf(FILE *const f)
{
#ifdef _WIN32
  #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HANDLE con = GetStdHandle(f == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    DWORD con_mode = 0;
    use_printf_color = con && con != INVALID_HANDLE_VALUE &&
                       GetConsoleMode(con, &con_mode) &&
                       SetConsoleMode(con, con_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  #endif
#elif HAVE_UNISTD_H
    if (isatty(f == stderr ? 2 : 1)) {
        const char *const term = getenv("TERM");
        use_printf_color = term && strcmp(term, "dumb");
    }
#endif
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
