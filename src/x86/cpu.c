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

#include <stdint.h>

#include "checkasm/osdep.h"

#include "config.h"
#include "internal.h"
#include "cpu.h"

#if ARCH_X86_64

typedef struct {
    uint32_t eax, ebx, edx, ecx;
} CpuidRegisters;

void checkasm_cpu_cpuid(CpuidRegisters *regs, unsigned leaf, unsigned subleaf);
uint64_t checkasm_cpu_xgetbv(unsigned xcr);

void checkasm_warmup_avx2(void);
void checkasm_warmup_avx512(void);
static void noop(void) {}

typedef void (*checkasm_simd_warmup_func)(void);
static COLD checkasm_simd_warmup_func get_simd_warmup(void)
{
    checkasm_simd_warmup_func simd_warmup = noop;

    union {
        CpuidRegisters r;
        struct {
            uint32_t max_leaf;
            char vendor[12];
        };
    } cpu;
    checkasm_cpu_cpuid(&cpu.r, 0, 0);

    if (cpu.max_leaf >= 1) {
        CpuidRegisters r;
        checkasm_cpu_cpuid(&r, 1, 0);

        /* We only support >128-bit SIMD on x86-64. */
        #define X(reg, mask) (((reg) & (mask)) == (mask))
        if (X(r.ecx, 0x18000000)) /* OSXSAVE/AVX */ {
            const uint64_t xcr0 = checkasm_cpu_xgetbv(0);
            if (X(xcr0, 0x00000006)) /* XMM/YMM */ {
                if (cpu.max_leaf >= 7) {
                    checkasm_cpu_cpuid(&r, 7, 0);
                    if (X(r.ebx, 0x00000128)) /* BMI1/BMI2/AVX2 */ {
                        simd_warmup = checkasm_warmup_avx2;
                        if (X(xcr0, 0x000000e0)) /* ZMM/OPMASK */ {
                            if (X(r.ebx, 0xd0230000) && X(r.ecx, 0x00005f42))
                                simd_warmup = checkasm_warmup_avx512;
                        }
                    }
                }
            }
        }
    }

    return simd_warmup;
}

void checkasm_simd_warmup(void)
{
    static checkasm_simd_warmup_func simd_warmup = NULL;
    if (!simd_warmup)
        simd_warmup = get_simd_warmup();

    simd_warmup();
}

#endif // ARCH_X86_64
