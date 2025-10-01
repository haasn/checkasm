#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "checkasm.h"


static const CheckasmCpuFlag flags[] = {
    { "x86", "x86", EXAMPLE_CPU_FLAG_X86 }
};

static const CheckasmTest tests[] = {
    { "nihcpy", checkasm_check_nihcpy },
};

static void print_usage(const char *const progname)
{
    fprintf(stderr,
            "Usage: %s [options...] <random seed>\n"
            "    <random seed>              Use fixed value to seed the PRNG\n"
            "Options:\n"
            "    --affinity=<cpu>           Run the process on CPU <cpu>\n"
            "    --bench -b                 Benchmark the tested functions\n"
            "    --function=<pattern> -f    Test only the functions matching <pattern>\n"
            "    --help -h                  Print this usage info\n"
            "    --list-cpuflags            List available cpu flags\n"
            "    --list-functions           List available functions\n"
            "    --list-tests               List available tests\n"
            "    --runs=<shift>             Number of benchmark iterations to run (log 2)\n"
            "    --test=<pattern> -t        Test only <pattern>\n"
            "    --verbose -v               Print verbose output on failure\n",
            progname);
}

static int parseull(unsigned long long *const dst, const char *const str, const int base)
{
    char *end;
    errno = 0;
    *dst = strtoull(str, &end, base);
    return errno || end == str || *end;
}

int main(int argc, char *argv[])
{
    CheckasmConfig cfg = {
        #define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
        .cpu_flags      = flags,
        .nb_cpu_flags   = ARRAY_LEN(flags),
        .tests          = tests,
        .nb_tests       = ARRAY_LEN(tests),
        .get_cpu_flags  = example_get_cpu_flags,
        .set_cpu_flags  = example_set_cpu_flags,

    };

    while (argc > 1) {
        if (!strncmp(argv[1], "--help", 6) || !strcmp(argv[1], "-h")) {
            print_usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[1], "--list-cpuflags")) {
            checkasm_list_cpu_flags(&cfg);
            return 0;
        } else if (!strcmp(argv[1], "--list-tests")) {
            checkasm_list_tests(&cfg);
            return 0;
        } else if (!strcmp(argv[1], "--list-functions")) {
            cfg.list_functions = 1;
        } else if (!strcmp(argv[1], "--bench") || !strcmp(argv[1], "-b")) {
            cfg.bench = 1;
        } else if (!strncmp(argv[1], "--runs=", 7)) {
            const char *const s = argv[1] + 7;
            unsigned long long runs_log2;
            if (!parseull(&runs_log2, s, 10) || runs_log2 > 63) {
                fprintf(stderr, "checkasm: invalid number of runs (1 << %s)\n", s);
                print_usage(argv[0]);
                return 1;
            }
            cfg.bench_runs = 1u << runs_log2;
        } else if (!strncmp(argv[1], "--test=", 7)) {
            cfg.test_pattern = argv[1] + 7;
        } else if (!strcmp(argv[1], "-t")) {
            cfg.test_pattern = argc > 1 ? argv[2] : "";
            argc--;
            argv++;
        } else if (!strncmp(argv[1], "--function=", 11)) {
            cfg.function_pattern = argv[1] + 11;
        } else if (!strcmp(argv[1], "-f")) {
            cfg.function_pattern = argc > 1 ? argv[2] : "";
            argc--;
            argv++;
        } else if (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-v")) {
            cfg.verbose = 1;
        } else if (!strncmp(argv[1], "--affinity=", 11)) {
            const char *const s = argv[1] + 11;
            unsigned long long affinity;
            if (!parseull(&affinity, s, 16)) {
                fprintf(stderr, "checkasm: invalid cpu affinity (%s)\n", s);
                print_usage(argv[0]);
                return 1;
            }
            cfg.cpu_affinity = affinity;
        } else {
            unsigned long long seed;
            if (!parseull(&seed, argv[1], 10)) {
                fprintf(stderr, "checkasm: unknown option (%s)\n", argv[1]);
                print_usage(argv[0]);
                return 1;
            }
            cfg.seed = (unsigned) seed;
        }

        argc--;
        argv++;
    }

    return checkasm_run(&cfg);
}
