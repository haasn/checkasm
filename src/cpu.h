/*
 * Copyright © 2018-2022, VideoLAN and dav1d authors
 * Copyright © 2018-2022, Two Orioles, LLC
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

#ifndef CHECKASM_CPU_H
#define CHECKASM_CPU_H

#include "config.h"

#if ARCH_X86

/* Initializes internal state for checkasm_checked_call().
 * Returns cpuid and model name. */
unsigned checkasm_init_x86(char *name);

#elif ARCH_RISCV

/* Initializes internal state for checkasm_checked_call().
 * Returns the vector length in bytes, or 0 if no RVV */
int checkasm_init_riscv(void);

#elif ARCH_AARCH64 && HAVE_SVE

/* Returns the SVE vector length in bytes, or 0 if no SVE */
int checkasm_sve_length(void);

#elif ARCH_ARM

void checkasm_checked_call_vfp(void *func, int dummy, ...);
void checkasm_checked_call_novfp(void *func, int dummy, ...);

/* Returns a nonzero value if NEON is available, 0 otherwise */
int checkasm_has_neon(void);

#endif

unsigned long checkasm_getauxval(unsigned long);

#endif /* CHECKASM_CPU_H */
