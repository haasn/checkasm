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

#ifndef CHECKASM_PLATFORM_H
#define CHECKASM_PLATFORM_H

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64ec__)                    \
    || defined(_M_ARM64EC)
  #include "checkasm/platform/aarch64.h"
#elif defined(__arm__) || defined(_M_ARM)
  #include "checkasm/platform/arm.h"
#elif defined(__x86_64__) || defined(_M_AMD64)
  #include "checkasm/platform/x86_64.h"
#elif defined(__i386__) || defined(_M_IX86)
  #include "checkasm/platform/x86_32.h"
#elif defined(__powerpc64__) && defined(__LITTLE_ENDIAN__)
  #include "checkasm/platform/ppc64le.h"
#elif defined(__riscv)
  #include "checkasm/platform/riscv.h"
#elif defined(__loongarch__)
  #include "checkasm/platform/loongarch.h"
#endif

#ifndef checkasm_declare_impl
  #define checkasm_declare_impl(ret, ...)
#endif

#ifndef CHECKASM_ALIGNMENT
  #define CHECKASM_ALIGNMENT 8
#endif

#endif /* CHECKASM_PLATFORM_H */
