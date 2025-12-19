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
#include <stddef.h>
#include <stdint.h>

#if ARCH_X86

/* Initializes internal state for checkasm_checked_call().
 * Returns cpuid and model name. */
unsigned checkasm_init_x86(char *name);

#elif ARCH_RISCV

/* Gets the CPU identification registers. */
int checkasm_get_cpuids(uint32_t *vendor, uint64_t *arch, uint64_t *imp);

/* Checks if floating point registers are supported. */
int checkasm_has_float(void);

/* Checks if vector registers are supported. */
int checkasm_has_vector(void);

/* Returns the vector length in bits, 0 if unavailable. */
unsigned checkasm_vlen(void);

#elif ARCH_AARCH64

/* Returns a nonzero value if SVE is available, 0 otherwise */
int checkasm_has_sve(void);

  #if HAVE_SVE

/* Returns the SVE vector length in bytes, or 0 if no SVE */
int checkasm_sve_length(void);

  #endif

#elif ARCH_ARM

void checkasm_checked_call_vfp(void *func, int dummy, ...);
void checkasm_checked_call_novfp(void *func, int dummy, ...);

/* Returns a nonzero value if NEON is available, 0 otherwise */
int checkasm_has_neon(void);
/* Returns a nonzero value if VFP is available, 0 otherwise */
int checkasm_has_vfp(void);

#endif

unsigned long checkasm_getauxval(unsigned long);
const char *checkasm_get_brand_string(char *buf, size_t buflen);
const char *checkasm_get_jedec_vendor_name(unsigned bank, unsigned offset);

#endif /* CHECKASM_CPU_H */
