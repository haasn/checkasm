#include <checkasm/checkasm.h>
#include "example.h"

void checkasm_check_good(void);
void checkasm_check_bad(void);
void checkasm_check_ugly(void);

static const CheckasmCpuFlag flags[] = {
    { "Bad C",          "badc",     EXAMPLE_CPU_FLAG_BAD_C  },
#if ARCH_X86
    { "Generic x86",    "x86",      EXAMPLE_CPU_FLAG_X86    },
    { "MMX",            "mmx",      EXAMPLE_CPU_FLAG_MMX    },
    { "SSE2",           "sse2",     EXAMPLE_CPU_FLAG_SSE2   },
    { "AVX-2",          "avx2",     EXAMPLE_CPU_FLAG_AVX2   },
    { "AVX-512",        "avx512",   EXAMPLE_CPU_FLAG_AVX512 },
#endif
};

static const CheckasmTest tests[] = {
    { "good", checkasm_check_good },
    { "bad",  checkasm_check_bad },
    { "ugly", checkasm_check_ugly },
};

int main(int argc, const char *argv[])
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

    return checkasm_main(&cfg, argc, argv);
}
