/*
 * Copyright © 2022-2025 Rémi Denis-Courmont.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
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

#include "config.h"
#include "cpu.h"
#include "internal.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <limits.h>
#if HAVE_SYS_HWPROBE_H
#include <sys/hwprobe.h>
#elif HAVE_ASM_HWPROBE_H
#include <asm/hwprobe.h>
#include <sys/syscall.h>
#include <unistd.h>

static int __riscv_hwprobe(struct riscv_hwprobe *pairs, size_t pair_count,
                           size_t cpu_count, unsigned long *cpus,
                           unsigned int flags)
{
        return syscall(__NR_riscv_hwprobe, pairs, pair_count, cpu_count, cpus,
                       flags);
}
#endif
#if HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
#include <sys/auxv.h>
#define HWCAP_RV(letter) (1ul << ((letter) - 'A'))
#endif

/* CPU capabilities relevant to the checked call harness. */
// Bit 0 reserved for float.
#define RISCV_VECTOR (1 << 1)

static int checkasm_hwprobe(void)
{
    int flags = 0;
#if HAVE_SYS_HWPROBE_H || HAVE_ASM_HWPROBE_H
    struct riscv_hwprobe pairs[] = {
        { RISCV_HWPROBE_KEY_IMA_EXT_0, 0 },
    };

    if (__riscv_hwprobe(pairs, ARRAY_SIZE(pairs), 0, NULL, 0) == 0) {
#ifdef RISCV_HWPROBE_EXT_ZVE32X
        if (pairs[0].value & RISCV_HWPROBE_EXT_ZVE32X)
            flags |= RISCV_VECTOR;
#endif
#ifdef RISCV_HWPROBE_IMA_V
        // A few Linux kernel (6.6+) versions recognise V but not Zve32x.
        if (pairs[0].value & RISCV_HWPROBE_IMA_V)
            flags |= RISCV_VECTOR;
#endif
    }
    /*
     * We purposedly do not fallback to HWCAP on Linux. Kernel versions without
     * `hwprobe()` and do not support vectors.
     */
#elif HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
    {
        const unsigned long hwcap = checkasm_getauxval(AT_HWCAP);

        if (hwcap & HWCAP_RV('V'))
            flags |= RISCV_VECTOR;
    }
#endif
#ifdef __riscv_vector
    flags |= RISCV_VECTOR;
#endif
    return flags;
}

static int checkasm_cpu_flags(void)
{
    static atomic_int cpu_flags = INT_MIN;
    int flags = atomic_load_explicit(&cpu_flags, memory_order_relaxed);

    if (flags < 0) {
        flags = checkasm_hwprobe();
        atomic_store_explicit(&cpu_flags, flags, memory_order_relaxed);
    }

    return flags;
}

int checkasm_has_vector(void)
{
    return (checkasm_cpu_flags() & RISCV_VECTOR) != 0;
}

unsigned checkasm_vlen(void)
{
    unsigned long value = 0;

    if (checkasm_has_vector())
        __asm__ ("csrr %0, vlenb\n" : "=r" (value));

    return ((unsigned)value) * 8;
}
