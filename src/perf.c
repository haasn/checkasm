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

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#include "checkasm/perf.h"
#include "checkasm/test.h"
#include "internal.h"
#include "stats.h"

#if CONFIG_MACOS_KPERF
  #include <dlfcn.h>
#endif

static uint64_t perf_start_gettime(void)
{
    return checkasm_gettime_nsec();
}

static uint64_t perf_stop_gettime(uint64_t t)
{
    return checkasm_gettime_nsec() - t;
}

CheckasmPerf checkasm_perf = {
    .start = perf_start_gettime,
    .stop  = perf_stop_gettime,
    .name  = "gettime",
    .unit  = "nsec",
};

#if CONFIG_MACOS_KPERF

static int (*kpc_get_thread_counters)(int, unsigned int, void *);

  #define CFGWORD_EL0A64EN_MASK       (0x20000)
  #define CPMU_CORE_CYCLE             0x02
  #define KPC_CLASS_FIXED_MASK        (1 << 0)
  #define KPC_CLASS_CONFIGURABLE_MASK (1 << 1)
  #define COUNTERS_COUNT              10
  #define CONFIG_COUNT                8
  #define KPC_MASK                    (KPC_CLASS_CONFIGURABLE_MASK | KPC_CLASS_FIXED_MASK)

COLD int checkasm_perf_init(void)
{
    uint64_t config[COUNTERS_COUNT] = { 0 };

    void *kperf
        = dlopen("/System/Library/PrivateFrameworks/kperf.framework/kperf", RTLD_LAZY);
    if (!kperf) {
        fprintf(stderr, "checkasm: Unable to load kperf: %s\n", dlerror());
        return 1;
    }

    int (*kpc_force_all_ctrs_set)(int)          = dlsym(kperf, "kpc_force_all_ctrs_set");
    int (*kpc_set_counting)(uint32_t)           = dlsym(kperf, "kpc_set_counting");
    int (*kpc_set_thread_counting)(uint32_t)    = dlsym(kperf, "kpc_set_thread_counting");
    int (*kpc_set_config)(uint32_t, void *)     = dlsym(kperf, "kpc_set_config");
    uint32_t (*kpc_get_counter_count)(uint32_t) = dlsym(kperf, "kpc_get_counter_count");
    uint32_t (*kpc_get_config_count)(uint32_t)  = dlsym(kperf, "kpc_get_config_count");
    kpc_get_thread_counters                     = dlsym(kperf, "kpc_get_thread_counters");

    if (!kpc_get_thread_counters) {
        fprintf(stderr, "checkasm: Unable to load kpc_get_thread_counters\n");
        return 1;
    }

    if (!kpc_get_counter_count || kpc_get_counter_count(KPC_MASK) != COUNTERS_COUNT) {
        fprintf(stderr, "checkasm: Unxpected kpc_get_counter_count\n");
        return 1;
    }
    if (!kpc_get_config_count || kpc_get_config_count(KPC_MASK) != CONFIG_COUNT) {
        fprintf(stderr, "checkasm: Unxpected kpc_get_config_count\n");
        return 1;
    }

    config[0] = CPMU_CORE_CYCLE | CFGWORD_EL0A64EN_MASK;

    if (!kpc_set_config || kpc_set_config(KPC_MASK, config)) {
        fprintf(stderr, "checkasm: The kperf API needs to be run as root\n");
        return 1;
    }
    if (!kpc_force_all_ctrs_set || kpc_force_all_ctrs_set(1)) {
        fprintf(stderr, "checkasm: kpc_force_all_ctrs_set failed\n");
        return 1;
    }
    if (!kpc_set_counting || kpc_set_counting(KPC_MASK)) {
        fprintf(stderr, "checkasm: kpc_set_counting failed\n");
        return 1;
    }
    if (!kpc_set_counting || kpc_set_thread_counting(KPC_MASK)) {
        fprintf(stderr, "checkasm: kpc_set_thread_counting failed\n");
        return 1;
    }

    return 0;
}

uint64_t checkasm_kperf_cycles(void)
{
    uint64_t counters[COUNTERS_COUNT];
    if (kpc_get_thread_counters(0, COUNTERS_COUNT, counters))
        return -1;

    return counters[0];
}

#else

COLD int checkasm_perf_init(void)
{
  #if CONFIG_LINUX_PERF
    if (!checkasm_perf_init_linux(&checkasm_perf))
        return 0;
  #endif

    if (!checkasm_save_context(checkasm_context)) {
        uint64_t t;
        (void) t;
        checkasm_set_signal_handler_state(1);
        CHECKASM_PERF_SETUP();
        CHECKASM_PERF_START(t);
        CHECKASM_PERF_STOP(t);
        checkasm_set_signal_handler_state(0);
        return 0;
    } else {
        fprintf(stderr, "checkasm: unable to access cycle counter\n");
        return 1;
    }
}

#endif

/* Measure the overhead of the timing code */
COLD CheckasmVar checkasm_measure_nop_cycles(void)
{
    const uint64_t target_nsec = 10000000; /* 10 ms */

    CheckasmStats stats;
    checkasm_stats_reset(&stats);

    void (*const func_new)(void *) = checkasm_noop;
    void *const ptr0 = (void *) 0x1000, *const ptr1 = (void *) 0x2000;

    CHECKASM_PERF_SETUP();

    for (uint64_t total_nsec = 0; total_nsec < target_nsec;) {
        const int runs   = stats.next_count;
        uint64_t  cycles = 0;
        int       count  = 0;

        /* Measure the overhead of the timing code (in cycles) */
        uint64_t nsec = checkasm_gettime_nsec();
        for (int i = 0; i < runs; i++) {
            uint64_t t;
            int      talt;
            (void) talt;
            CHECKASM_PERF_START(t);
            CALL16(alternate(ptr0, ptr1));
            CALL16(alternate(ptr0, ptr1));
            CHECKASM_PERF_STOP(t);
            if (t * count <= cycles * 4 && (i > 0 || runs < 50)) {
                cycles += t;
                count++;
            }
        }
        nsec = checkasm_gettime_nsec() - nsec;
        checkasm_stats_add(&stats, (CheckasmSample) { cycles, count });

        total_nsec += nsec;
        if (nsec < target_nsec >> 7)
            checkasm_stats_count_grow(&stats);
    }

    return checkasm_stats_estimate(&stats);
}

COLD CheckasmVar checkasm_measure_perf_scale(void)
{
    /* Try to make the loop long enough to be measurable, but not too long
     * to avoid being affected by CPU frequency scaling or preemption */
    const uint64_t target_nsec = 100000 / 2; /* 100 us */

    /* Estimate the time per loop iteration in two different ways */
    CheckasmStats stats_cycles;
    CheckasmStats stats_nsec;
    checkasm_stats_reset(&stats_cycles);
    checkasm_stats_reset(&stats_nsec);

    CHECKASM_PERF_SETUP();

    while (stats_cycles.nb_samples < (int) ARRAY_SIZE(stats_cycles.samples)) {
        const int iters = stats_cycles.next_count;

        /* Warm up the CPU a tiny bit */
        for (int i = 0; i < 100; i++)
            checkasm_noop(NULL);

        uint64_t cycles;
        CHECKASM_PERF_START(cycles);
        for (int i = 0; i < iters; i++)
            checkasm_noop(NULL);
        CHECKASM_PERF_STOP(cycles);

        /* Measure the same loop with wallclock time instead of cycles */
        uint64_t nsec = checkasm_gettime_nsec();
        for (int i = 0; i < iters; i++)
            checkasm_noop(NULL);
        nsec = checkasm_gettime_nsec() - nsec;

        checkasm_stats_add(&stats_cycles, (CheckasmSample) { cycles, iters });
        checkasm_stats_add(&stats_nsec, (CheckasmSample) { nsec, iters });

        if (nsec < target_nsec)
            checkasm_stats_count_grow(&stats_cycles);
    }

    CheckasmVar est_cycles = checkasm_stats_estimate(&stats_cycles);
    CheckasmVar est_nsec   = checkasm_stats_estimate(&stats_nsec);
    return checkasm_var_div(est_nsec, est_cycles);
}
