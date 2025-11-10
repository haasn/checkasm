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

#include "config.h"

#include "internal.h"

#if ARCH_AARCH64 && (!defined(_MSC_VER) || defined(__clang__))

static inline uint64_t checkasm_cntvct(void)
{
    uint64_t tick_counter;
    /* A different timer register, that usually (always?) is readable.
     * On older ARM architectures, this would have a system specified
     * frequency (1-50 MHz), while on newer ARM architectures, it has a
     * fixed frequency of 1 GHz, making it quite usable. */
    __asm__ __volatile__("isb\nmrs %0, cntvct_el0" : "=r"(tick_counter)::"memory");
    return tick_counter;
}

static inline uint64_t checkasm_cntfrq(void)
{
    uint64_t frequency;
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(frequency));
    return frequency;
}

static uint64_t perf_start(void)
{
    return checkasm_cntvct();
}

static uint64_t perf_stop(uint64_t t)
{
    return checkasm_cntvct() - t;
}

COLD int checkasm_perf_init_arm(CheckasmPerf *perf)
{
    /* Try using the alternative timing register. */
    if (!checkasm_save_context(checkasm_context)) {
        checkasm_set_signal_handler_state(1);
        checkasm_cntvct();
        uint64_t frequency = checkasm_cntfrq();
        checkasm_set_signal_handler_state(0);

        /* If the timer has a frequency less than 100 MHz, let's not
         * use it and stick to the default gettime() fallback. */
        if (frequency < 100000000)
            return 1;

        perf->start = perf_start;
        perf->stop  = perf_stop;
        perf->name  = "aarch64 (cntvct)";
        if (frequency == 1000000000) /* 1 GHz */
            perf->unit = "nsec";
        else
            perf->unit = "tick";

        return 0;
    }
    return 1;
}
#else
COLD int checkasm_perf_init_arm(CheckasmPerf *perf)
{
    /* Nothing to try; no timer chosen. */
    return 1;
}
#endif
