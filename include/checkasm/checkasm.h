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

#ifndef CHECKASM_CHECKASM_H
#define CHECKASM_CHECKASM_H

#include <stdint.h>

#include "checkasm/attributes.h"

typedef uint64_t CheckasmCpu;

typedef struct CheckasmCpuInfo {
    const char *name;
    const char *suffix;
    CheckasmCpu flag;
} CheckasmCpuInfo;

typedef struct CheckasmTest {
    const char *name;
    void (*func)(void);
} CheckasmTest;

typedef enum CheckasmFormat {
    CHECKASM_FORMAT_PRETTY, // Pretty-printed (colored) text output
    CHECKASM_FORMAT_CSV,    // Comma-separated values, with optional header
    CHECKASM_FORMAT_TSV,    // Tab-separated values, with optional header
    CHECKASM_FORMAT_JSON,   // JSON structured output, with all measurement data
    CHECKASM_FORMAT_HTML,   // Interactive HTML report
} CheckasmFormat;

typedef struct CheckasmConfig {
    /* List of CPU flags understood by the implementation. These will be tested
     * in incremental order, each test run inheriting any active flags from
     * previously tested CPUs. */
    const CheckasmCpuInfo *cpu_flags;
    int                    nb_cpu_flags;

    /* List of tests */
    const CheckasmTest *tests;
    int                 nb_tests;

    /* Detected CPU flags. Note: Any extra flags not included in `cpu_flags`
     * will also be transparently included in checkasm_get_cpu_flags(), and
     * can thus be used to signal flags that should be assumed to always be
     * enabled. (e.g. CPU_FLAG_FAST_* modifiers) */
    CheckasmCpu cpu;

    /* If provided, this function will be called any time the active set of
     * CPU flags changes, with the new set of flags as argument; including
     * once at the start of the program with the baseline set of flags. */
    void (*set_cpu_flags)(CheckasmCpu new_flags);

    /* Pattern of tests/functions to enable. NULL means all. */
    const char *test_pattern;
    const char *function_pattern;

    /* If nonzero, enable benchmarking, with the specified target time (µs)
     * per function tested, defaulting to 1000 µs if left unset. */
    int      bench;
    unsigned bench_usec;

    /* Output format options */
    CheckasmFormat format;  /* how to report benchmark results */
    int            verbose; /* print verbose timing and failure info */

    /* If nonzero, use the specified seed for random number generation. */
    unsigned seed;

    /* Repeat the test (and benchmark, if enabled) this many times, on
     * successive seeds. Setting -1 effectively tests every possible seed. */
    unsigned repeat;

    /* If nonzero, the process will be pinned to the specified CPU (id) */
    unsigned cpu_affinity;
} CheckasmConfig;

/* Returns the current (masked) set of CPU flags. */
CHECKASM_API CheckasmCpu checkasm_get_cpu_flags(void);

/**
 * Print a list of all cpuflags/tests/functions available for testing.
 */
CHECKASM_API void checkasm_list_cpu_flags(const CheckasmConfig *config);
CHECKASM_API void checkasm_list_tests(const CheckasmConfig *config);
CHECKASM_API void checkasm_list_functions(const CheckasmConfig *config);

/**
 * Run all tests (and benchmarks) matching the specified patterns.
 *
 * Returns 0 on success, or a negative AVERROR code on failure.
 */
CHECKASM_API int checkasm_run(const CheckasmConfig *config);

/**
 * Entry-point for main() that parses common args (mutating `config`), and
 * prints usage on failure. The user should set project-specific fields in
 * `config` before calling this function; in particular `config.cpu_flags`,
 * `config.tests` and `config.{get,set}_cpu_flags`.
 */
CHECKASM_API int checkasm_main(CheckasmConfig *config, int argc, const char *argv[]);

#endif /* CHECKASM_CHECKASM_H */
