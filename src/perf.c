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

#if CHECKASM_LINUX_PERF
    #include <sys/syscall.h>
#elif CHECKASM_MACOS_KPERF
    #include <dlfcn.h>
#endif

#if CHECKASM_LINUX_PERF

static int perf_sysfd;

COLD int checkasm_perf_init(void)
{
    struct perf_event_attr attr = {
        .type           = PERF_TYPE_HARDWARE,
        .size           = sizeof(struct perf_event_attr),
        .config         = PERF_COUNT_HW_CPU_CYCLES,
        .disabled       = 1, // start counting only on demand
        .exclude_kernel = 1,
        .exclude_hv     = 1,
#if !ARCH_X86
        .exclude_guest  = 1,
#endif
    };

    fprintf(stderr, "benchmarking with Linux Perf Monitoring API\n");
    perf_sysfd = syscall(SYS_perf_event_open, &attr, 0, -1, -1, 0);
    if (perf_sysfd == -1) {
        perror("perf_event_open");
        return 1;
    }
    return 0;
}

int checkasm_get_perf_sysfd(void)
{
    return perf_sysfd;
}

#elif CHECKASM_MACOS_KPERF

static int (*kpc_get_thread_counters)(int, unsigned int, void *);

#define CFGWORD_EL0A64EN_MASK (0x20000)
#define CPMU_CORE_CYCLE 0x02
#define KPC_CLASS_FIXED_MASK        (1 << 0)
#define KPC_CLASS_CONFIGURABLE_MASK (1 << 1)
#define COUNTERS_COUNT 10
#define CONFIG_COUNT 8
#define KPC_MASK (KPC_CLASS_CONFIGURABLE_MASK | KPC_CLASS_FIXED_MASK)

COLD int checkasm_perf_init(void)
{
    uint64_t config[COUNTERS_COUNT] = { 0 };

    void *kperf = dlopen("/System/Library/PrivateFrameworks/kperf.framework/kperf", RTLD_LAZY);
    if (!kperf) {
        fprintf(stderr, "checkasm: Unable to load kperf: %s\n", dlerror());
        return 1;
    }

    int (*kpc_force_all_ctrs_set)(int) = dlsym(kperf, "kpc_force_all_ctrs_set");
    int (*kpc_set_counting)(uint32_t) = dlsym(kperf, "kpc_set_counting");
    int (*kpc_set_thread_counting)(uint32_t) = dlsym(kperf, "kpc_set_thread_counting");
    int (*kpc_set_config)(uint32_t, void *) = dlsym(kperf, "kpc_set_config");
    uint32_t (*kpc_get_counter_count)(uint32_t) = dlsym(kperf, "kpc_get_counter_count");
    uint32_t (*kpc_get_config_count)(uint32_t) = dlsym(kperf, "kpc_get_config_count");
    kpc_get_thread_counters = dlsym(kperf, "kpc_get_thread_counters");

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

    fprintf(stderr, "benchmarking with macOS kperf API\n");
    return 0;
}

uint64_t checkasm_kperf_cycles(void) {
    uint64_t counters[COUNTERS_COUNT];
    if (kpc_get_thread_counters(0, COUNTERS_COUNT, counters))
        return -1;

    return counters[0];
}

#elif defined(readtime)

COLD int checkasm_perf_init(void)
{
    if (!checkasm_save_context()) {
        checkasm_set_signal_handler_state(1);
        readtime();
        checkasm_set_signal_handler_state(0);
        return 0;
    } else {
        fprintf(stderr, "checkasm: unable to access cycle counter\n");
        return 1;
    }
}

#else

COLD int checkasm_perf_init(void)
{
    fprintf(stderr, "checkasm: benchmarking not supported on this platform\n");
    return 1;
}

#endif

#ifdef PERF_START
static int cmp_nop(const void *a, const void *b)
{
    return *(const uint16_t*)a - *(const uint16_t*)b;
}

/* Measure the overhead of the timing code (in decicycles) */
COLD double checkasm_measure_nop_time(void)
{
    uint16_t nops[10000];
    int nop_sum = 0;

    PERF_SETUP();
    for (int i = 0; i < 10000; i++) {
        uint64_t t;
        PERF_START(t);
        PERF_STOP(t);
        nops[i] = (uint16_t) t;
    }

    qsort(nops, 10000, sizeof(uint16_t), cmp_nop);
    for (int i = 2500; i < 7500; i++)
        nop_sum += nops[i];

    return nop_sum / 5000.0;
}
#else
COLD double checkasm_measure_nop_time(void) { return 0.0; }
#endif
