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

#ifndef CHECKASM_ATTRIBUTES_H
#define CHECKASM_ATTRIBUTES_H

#include <stddef.h>
#include <assert.h>

#ifdef __GNUC__
  #define ATTR_ALIAS __attribute__((may_alias))
  #if defined(__MINGW32__) && !defined(__clang__)
    #define ATTR_FORMAT_PRINTF(fmt, attr) __attribute__((__format__(__gnu_printf__, fmt, attr)))
  #else
    #define ATTR_FORMAT_PRINTF(fmt, attr) __attribute__((__format__(__printf__, fmt, attr)))
  #endif
  #define COLD __attribute__((cold))
#else
  #define ATTR_ALIAS
  #define ATTR_FORMAT_PRINTF(fmt, attr)
  #define COLD
#endif

/*
 * API for variables, struct members (ALIGN()) like:
 *   uint8_t var[1][2][3][4]
 * becomes:
 *   ALIGN(uint8_t var[1][2][3][4], alignment).
 */
#ifdef _MSC_VER
  #define ALIGN(ll, a) __declspec(align(a)) ll
#else
  #define ALIGN(line, align) line __attribute__((aligned(align)))
#endif

/*
 * API for stack alignment (ALIGN_STK()) of variables like:
 *   uint8_t var[1][2][3][4]
 * becomes:
 *   ALIGN_STK(uint8_t, var, 1, [2][3][4])
 */
#if ARCH_X86_64
/* x86-64 needs 32- and 64-byte alignment for AVX2 and AVX-512. */
  #define ALIGN_STK(type, var, sz1d, sznd) ALIGN(type var[sz1d]sznd, 64)
#elif ARCH_AARCH64 || ARCH_ARM || ARCH_LOONGARCH || ARCH_PPC64LE || ARCH_X86_32
/* ARM doesn't benefit from anything more than 16-byte alignment. */
  #define ALIGN_STK(type, var, sz1d, sznd) ALIGN(type var[sz1d]sznd, 16)
#else
/* No need for extra alignment on platforms without assembly. */
  #define ALIGN_STK(type, var, sz1d, sznd) ALIGN(type var[sz1d]sznd, 8)
#endif

#endif /* CHECKASM_ATTRIBUTES_H */
