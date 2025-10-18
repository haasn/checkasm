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

#ifndef CHECKASM_UTILS_H
#define CHECKASM_UTILS_H

#include <stdint.h>

#include "checkasm/attributes.h"

CHECKASM_API int checkasm_rand(void);

/* memory manipulation / initialization utilities */
CHECKASM_API void checkasm_randomize(void *buf, size_t bytes);
CHECKASM_API void checkasm_randomize_mask8 (uint8_t  *buf, int width, uint8_t  mask);
CHECKASM_API void checkasm_randomize_mask16(uint16_t *buf, int width, uint16_t mask);

CHECKASM_API void checkasm_clear(void *buf, size_t bytes);
CHECKASM_API void checkasm_clear8 (uint8_t  *buf, int width, uint8_t  val);
CHECKASM_API void checkasm_clear16(uint16_t *buf, int width, uint16_t val);

#define CLEAR_BUF(buf)     checkasm_clear(buf, sizeof(buf))
#define RANDOMIZE_BUF(buf) checkasm_randomize(buf, sizeof(buf))

/* float compare utilities */
CHECKASM_API int checkasm_float_near_ulp(float a, float b, unsigned max_ulp);
CHECKASM_API int checkasm_float_near_abs_eps(float a, float b, float eps);
CHECKASM_API int checkasm_float_near_abs_eps_ulp(float a, float b, float eps,
                                                 unsigned max_ulp);
CHECKASM_API int checkasm_float_near_ulp_array(const float *a, const float *b,
                                               unsigned max_ulp, int len);
CHECKASM_API int checkasm_float_near_abs_eps_array(const float *a, const float *b,
                                                   float eps, int len);
CHECKASM_API int checkasm_float_near_abs_eps_array_ulp(const float *a, const float *b,
                                                       float eps, unsigned max_ulp, int len);
CHECKASM_API int checkasm_double_near_abs_eps(double a, double b, double eps);
CHECKASM_API int checkasm_double_near_abs_eps_array(const double *a, const double *b,
                                                    double eps, unsigned len);

/* Defined for backwards compatibility */
#define float_near_ulp               checkasm_float_near_ulp
#define float_near_abs_eps           checkasm_float_near_abs_eps
#define float_near_abs_eps_ulp       checkasm_float_near_abs_eps_ulp
#define float_near_ulp_array         checkasm_float_near_ulp_array
#define float_near_abs_eps_array     checkasm_float_near_abs_eps_array
#define float_near_abs_eps_array_ulp checkasm_float_near_abs_eps_array_ulp
#define double_near_abs_eps          checkasm_double_near_abs_eps
#define double_near_abs_eps_array    checkasm_double_near_abs_eps_array

/*
 * API for variables, struct members (ALIGN_ARR()) like:
 *   uint8_t var[1][2][3][4]
 * becomes:
 *   ALIGN_ARR(uint8_t var[1][2][3][4], alignment).
 */
#ifdef _MSC_VER
    #define ALIGN_ARR(ll, a) __declspec(align(a)) ll
#else
    #define ALIGN_ARR(line, align) line __attribute__((aligned(align)))
#endif

/*
 * API for stack alignment (ALIGN_STK_$align()) of variables like:
 * uint8_t var[1][2][3][4]
 * becomes:
 * ALIGN_STK_$align(uint8_t, var, 1, [2][3][4])
 */
#define ALIGN_STK_64(type, var, sz1d, sznd) \
    ALIGN_ARR(type var[sz1d]sznd, ALIGN_64_VAL)
#define ALIGN_STK_32(type, var, sz1d, sznd) \
    ALIGN_ARR(type var[sz1d]sznd, ALIGN_32_VAL)
#define ALIGN_STK_16(type, var, sz1d, sznd) \
    ALIGN_ARR(type var[sz1d]sznd, ALIGN_16_VAL)

#define CHECKASM_ROUND(x,a) (((x)+((a)-1)) & ~((a)-1))
#define PIXEL_RECT(name, w, h) \
    ALIGN_STK_64(pixel, name##_buf, ((h)+32)*(CHECKASM_ROUND(w,64)+64) + 64,); \
    ptrdiff_t name##_stride = sizeof(pixel)*(CHECKASM_ROUND(w,64)+64); \
    (void)name##_stride; \
    int name##_buf_h = (h)+32; \
    (void)name##_buf_h;\
    pixel *name = name##_buf + (CHECKASM_ROUND(w,64)+64)*16 + 64

#define CLEAR_PIXEL_RECT(name)     CLEAR_BUF(name##_buf)
#define RANDOMIZE_PIXEL_RECT(name) RANDOMIZE_BUF(name##_buf)

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

#ifndef CONCAT
    #define CONCAT2(a,b) a ## b
    #define CONCAT(a,b) CONCAT2(a, b)
#endif

#define checkasm_check2(type, ...)       CONCAT(checkasm_check_, type)(__FILE__, __LINE__, __VA_ARGS__)
#define checkasm_check(type, ...)        checkasm_check2(type, __VA_ARGS__, 0, 0, 0)
#define checkasm_check_padded(type, ...) checkasm_check2(type, __VA_ARGS__)

#define checkasm_check_pixel(...)              checkasm_check(PIXEL_TYPE, __VA_ARGS__)
#define checkasm_check_pixel_padded(...)       checkasm_check2(PIXEL_TYPE, __VA_ARGS__, 1, 1, 8)
#define checkasm_check_pixel_padded_align(...) checkasm_check2(PIXEL_TYPE, __VA_ARGS__, 8)
#define checkasm_check_coef(...)  checkasm_check(COEF_TYPE,  __VA_ARGS__)

#endif /* CHECKASM_UTILS_H */
