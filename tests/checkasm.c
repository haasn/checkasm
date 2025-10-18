#include "tests.h"
#include <checkasm/checkasm.h>

static const CheckasmCpuInfo cpus[] = {
    { "Bad C",          "badc",   CHECKASM_CPU_FLAG_BAD_C  },
#if ARCH_X86
    { "Generic x86",    "x86",    CHECKASM_CPU_FLAG_X86    },
    { "MMX",            "mmx",    CHECKASM_CPU_FLAG_MMX    },
    { "SSE2",           "sse2",   CHECKASM_CPU_FLAG_SSE2   },
    { "AVX-2",          "avx2",   CHECKASM_CPU_FLAG_AVX2   },
    { "AVX-512",        "avx512", CHECKASM_CPU_FLAG_AVX512 },
#endif
#if ARCH_RISCV
    { "Generic RISC-V", "riscv",  CHECKASM_CPU_FLAG_RISCV  },
    { "RVV",            "rvv",    CHECKASM_CPU_FLAG_RVV    },
#endif
};

static const CheckasmTest tests[] = {
    { "generic", checkasm_check_generic },
#if ARCH_X86
    { "x86",     checkasm_check_x86     },
#elif ARCH_RISCV
    { "riscv", checkasm_check_riscv },
#endif
};

int main(int argc, const char *argv[])
{
    CheckasmConfig cfg = {
#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
        .cpu_flags    = cpus,
        .nb_cpu_flags = ARRAY_LEN(cpus),
        .tests        = tests,
        .nb_tests     = ARRAY_LEN(tests),
        .cpu          = CHECKASM_CPU_FLAG_BAD_C,
    };

#if ARCH_X86
    cfg.cpu |= checkasm_get_cpu_flags_x86();
#elif ARCH_RISCV
    cfg.cpu |= checkasm_get_cpu_flags_riscv();
#endif

    return checkasm_main(&cfg, argc, argv);
}
