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

#ifndef CHECKASM_PERF_AARCH64_H
#define CHECKASM_PERF_AARCH64_H

#include <stdint.h>

static inline uint64_t checkasm_pmccntr(void)
{
    uint64_t cycle_counter;
    /* This requires enabling user mode access to the cycle counter
     * (which can only be done from kernel space).
     * This could also read cntvct_el0 instead of pmccntr_el0; that register
     * might also be readable (depending on kernel version), but it has much
     * worse precision (it's a fixed 50 MHz timer). */
    __asm__ __volatile__("isb\nmrs %0, pmccntr_el0" : "=r"(cycle_counter)::"memory");
    return cycle_counter;
}

#define CHECKASM_PERF_SETUP()
#define CHECKASM_PERF_START(t) t = checkasm_pmccntr();
#define CHECKASM_PERF_STOP(t)  t = checkasm_pmccntr() - t
#define CHECKASM_PERF_NAME     "aarch64 (pmccntr)"
#define CHECKASM_PERF_UNIT     "cycle"

#endif /* CHECKASM_PERF_AARCH64_H */
