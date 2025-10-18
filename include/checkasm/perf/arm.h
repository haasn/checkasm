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

#ifndef CHECKASM_PERF_ARM_H
#define CHECKASM_PERF_ARM_H

#if !defined(_MSC_VER) && __ARM_ARCH >= 7

  #include <stdint.h>

static inline uint64_t readtime_counter(void)
{
    uint32_t cycle_counter;
    /* This requires enabling user mode access to the cycle counter (which
     * can only be done from kernel space). */
    __asm__ __volatile__("isb\nmrc p15, 0, %0, c9, c13, 0" : "=r"(cycle_counter)::"memory");
    return cycle_counter;
}

  #define CHECKASM_PERF_SETUP()
  #define CHECKASM_PERF_START(t) t = readtime_counter();
  #define CHECKASM_PERF_STOP(t)  t = readtime_counter() - t
  #define CHECKASM_PERF_NAME     "arm (ccnt)"
  #define CHECKASM_PERF_UNIT     "cycle"

#endif /* !defined(_MSC_VER) && __ARM_ARCH >= 7 */
#endif /* CHECKASM_PERF_ARM_H */
