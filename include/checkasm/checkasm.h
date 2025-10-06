/*
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

typedef struct CheckasmCpuFlag {
    const char *name;
    const char *suffix;
    uint64_t flag;
} CheckasmCpuFlag;

typedef struct CheckasmTest {
    const char *name;
    void (*func)(void);
} CheckasmTest;

typedef struct CheckasmConfig {
    /* List of CPU flags understood by the implementation. */
    const CheckasmCpuFlag *cpu_flags;
    int nb_cpu_flags;

    /* List of tests */
    const CheckasmTest *tests;
    int nb_tests;

    /* External functions to get and set (override) CPU flags. A value of
     * (uint64_t) -1 will disable any override and enable all CPU flags. */
    uint64_t (*get_cpu_flags)(void);
    void (*set_cpu_flags)(uint64_t flags);

    /* Pattern of tests/functions to enable. NULL means all. */
    const char *test_pattern;
    const char *function_pattern;

    /* If nonzero, enable verbose printing of failing test data. */
    int verbose;

    /* If set, outputs benchmark numbers separated by this character */
    char separator;

    /* If nonzero, enable benchmarking, with the specified number of
     * iterations, defaulting to 1024 if left unset. */
    int bench;
    unsigned bench_runs;

    /* If nonzero, use the specified seed for random number generation. */
    unsigned seed;

    /* If nonzero, the process will be pinned to the specified CPU (id) */
    unsigned cpu_affinity;
} CheckasmConfig;

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
