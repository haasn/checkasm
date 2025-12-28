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

/* Generate uniformly distributed random variables */
CHECKASM_API int      checkasm_rand(void);        /* [0, INT_MAX] */
CHECKASM_API double   checkasm_randf(void);       /* [0.0, 1.0) */
CHECKASM_API uint32_t checkasm_rand_uint32(void); /* [0, UINT32_MAX] */
CHECKASM_API int32_t  checkasm_rand_int32(void);  /* [INT32_MIN, INT32_MAX] */

/* Generate normally distributed random variables according to a given
 * distribution */
typedef struct CheckasmDist {
    double mean;
    double stddev;
} CheckasmDist;

#define checkasm_dist_standard ((CheckasmDist) { 0.0, 1.0 })
CHECKASM_API double checkasm_rand_dist(CheckasmDist);
CHECKASM_API double checkasm_rand_norm(void); /* standard distribution */

/* Memory manipulation / initialization utilities */
CHECKASM_API void checkasm_randomize(void *buf, size_t bytes);
CHECKASM_API void checkasm_randomize_mask8(uint8_t *buf, int width, uint8_t mask);
CHECKASM_API void checkasm_randomize_mask16(uint16_t *buf, int width, uint16_t mask);

CHECKASM_API void checkasm_randomize_range(double *buf, int width, double range);
CHECKASM_API void checkasm_randomize_rangef(float *buf, int width, float range);
CHECKASM_API void checkasm_randomize_dist(double *buf, int width, CheckasmDist);
CHECKASM_API void checkasm_randomize_distf(float *buf, int width, CheckasmDist);
CHECKASM_API void checkasm_randomize_norm(double *buf, int width);
CHECKASM_API void checkasm_randomize_normf(float *buf, int width);

CHECKASM_API void checkasm_clear(void *buf, size_t bytes);
CHECKASM_API void checkasm_clear8(uint8_t *buf, int width, uint8_t val);
CHECKASM_API void checkasm_clear16(uint16_t *buf, int width, uint16_t val);

/**
 * "Initializes" a buffer to a random mixture of pathological bytes, test
 * patterns, or random data, in order to try and trigger things like overflow,
 * underflow and so on. The exact behavior can depend heavily on the chosen
 * random seed.
 */
CHECKASM_API void checkasm_init(void *buf, size_t bytes);
CHECKASM_API void checkasm_init_mask8(uint8_t *buf, int width, uint8_t mask);
CHECKASM_API void checkasm_init_mask16(uint16_t *buf, int width, uint16_t mask);

#define CLEAR_BUF(buf)      checkasm_clear(buf, sizeof(buf))
#define RANDOMIZE_BUF(buf)  checkasm_randomize(buf, sizeof(buf))
#define INITIALIZE_BUF(buf) checkasm_init(buf, sizeof(buf))

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
                                                       float eps, unsigned max_ulp,
                                                       int len);
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

#ifdef _MSC_VER
  #define CHECKASM_ALIGN(x) __declspec(align(CHECKASM_ALIGNMENT)) x
#else
  #define CHECKASM_ALIGN(x) x __attribute__((aligned(CHECKASM_ALIGNMENT)))
#endif

#define DECL_CHECK_FUNC(NAME, TYPE)                                                      \
    int (NAME)(const char *const file, const int line, const TYPE *const buf1,           \
               const ptrdiff_t stride1, const TYPE *const buf2, const ptrdiff_t stride2, \
               const int w, const int h, const char *const buf_name, const int align_w,  \
               const int align_h, const int padding)

#define DECL_CHECKASM_CHECK_FUNC(type)                                                   \
    CHECKASM_API DECL_CHECK_FUNC(checkasm_check_impl_##type, type)

DECL_CHECKASM_CHECK_FUNC(int);
DECL_CHECKASM_CHECK_FUNC(int8_t);
DECL_CHECKASM_CHECK_FUNC(int16_t);
DECL_CHECKASM_CHECK_FUNC(int32_t);

DECL_CHECKASM_CHECK_FUNC(unsigned);
DECL_CHECKASM_CHECK_FUNC(uint8_t);
DECL_CHECKASM_CHECK_FUNC(uint16_t);
DECL_CHECKASM_CHECK_FUNC(uint32_t);

CHECKASM_API int checkasm_check_impl_float_ulp(const char *file, int line,
                                               const float *buf1, ptrdiff_t stride1,
                                               const float *buf2, ptrdiff_t stride2,
                                               int w, int h, const char *name,
                                               unsigned max_ulp, int align_w, int align_h,
                                               int padding);

#define checkasm_check_impl2(type) checkasm_check_impl_##type
#define checkasm_check_impl(type)  checkasm_check_impl2(type)

#define checkasm_check1(type, ...)       checkasm_check_impl_##type(__VA_ARGS__)
#define checkasm_check2(type, ...)       checkasm_check1(type, __FILE__, __LINE__, __VA_ARGS__)
#define checkasm_check(type, ...)        checkasm_check2(type, __VA_ARGS__, 0, 0, 0)
#define checkasm_check_padded(type, ...) checkasm_check2(type, __VA_ARGS__)

/* Helpers for defining aligned, padded rectangular buffers */
#define CHECKASM_ROUND(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define BUF_RECT(type, name, w, h)                                                       \
    DECL_CHECK_FUNC(*checkasm_check_impl_##name##_type, type)                            \
        = checkasm_check_impl_##type;                                                    \
    CHECKASM_ALIGN(type name##_buf[((h) + 32) * (CHECKASM_ROUND(w, 64) + 64) + 64]);     \
    ptrdiff_t name##_stride = sizeof(type) * (CHECKASM_ROUND(w, 64) + 64);               \
    int       name##_buf_h  = (h) + 32;                                                  \
    (void) checkasm_check_impl(name##_type);                                             \
    (void) name##_stride;                                                                \
    (void) name##_buf_h;                                                                 \
    type *name = name##_buf + (CHECKASM_ROUND(w, 64) + 64) * 16 + 64

#define CLEAR_BUF_RECT(name)      CLEAR_BUF(name##_buf)
#define INITIALIZE_BUF_RECT(name) INITIALIZE_BUF(name##_buf)
#define RANDOMIZE_BUF_RECT(name)  RANDOMIZE_BUF(name##_buf)

#define checkasm_check_rect(rect1, ...) checkasm_check(rect1##_type, rect1, __VA_ARGS__)
#define checkasm_check_rect_padded(rect1, ...)                                           \
    checkasm_check_padded(rect1##_type, rect1, __VA_ARGS__, 1, 1, 8)
#define checkasm_check_rect_padded_align(rect1, ...)                                     \
    checkasm_check_padded(rect1##_type, rect1, __VA_ARGS__, 8)

#endif /* CHECKASM_UTILS_H */
