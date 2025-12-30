#include "tests.h"
#include <checkasm/checkasm.h>

static const CheckasmCpuInfo cpus[] = {
    { "Bad C",           "badc",    CHECKASM_CPU_FLAG_BAD_C   },
#if ARCH_X86
    { "Generic x86",     "x86",     CHECKASM_CPU_FLAG_X86     },
    { "MMX",             "mmx",     CHECKASM_CPU_FLAG_MMX     },
    { "SSE2",            "sse2",    CHECKASM_CPU_FLAG_SSE2    },
    { "AVX-2",           "avx2",    CHECKASM_CPU_FLAG_AVX2    },
    { "AVX-512",         "avx512",  CHECKASM_CPU_FLAG_AVX512  },
#endif
#if ARCH_RISCV
    { "Generic RISC-V",  "rvi",     CHECKASM_CPU_FLAG_RVI     },
    { "Floating point",  "rvf",     CHECKASM_CPU_FLAG_RVF     },
    { "Vector",          "rvv",     CHECKASM_CPU_FLAG_RVV     },
#endif
#if ARCH_AARCH64
    { "Generic aarch64", "aarch64", CHECKASM_CPU_FLAG_AARCH64 },
#endif
#if ARCH_ARM
    { "Generic ARM",     "arm",     CHECKASM_CPU_FLAG_ARM     },
    { "VFP",             "vfp",     CHECKASM_CPU_FLAG_VFP     },
    { "NEON",            "neon",    CHECKASM_CPU_FLAG_NEON    },
#endif
    {0}
};

static const CheckasmTest tests[] = {
    { "generic", checkasm_check_generic },
#if ARCH_X86
    { "x86",     checkasm_check_x86     },
#elif ARCH_RISCV
    { "riscv", checkasm_check_riscv },
#elif ARCH_AARCH64
    { "aarch64", checkasm_check_aarch64 },
#elif ARCH_ARM
    { "arm", checkasm_check_arm },
#endif
    {0}
};

int main(int argc, const char *argv[])
{
    CheckasmConfig cfg = {
        .cpu_flags = cpus,
        .tests     = tests,
        .cpu       = CHECKASM_CPU_FLAG_BAD_C,
    };

#if ARCH_X86
    cfg.cpu |= checkasm_get_cpu_flags_x86();
#elif ARCH_RISCV
    cfg.cpu |= checkasm_get_cpu_flags_riscv();
#elif ARCH_AARCH64
    cfg.cpu |= checkasm_get_cpu_flags_aarch64();
#elif ARCH_ARM
    cfg.cpu |= checkasm_get_cpu_flags_arm();
#endif

    return checkasm_main(&cfg, argc, argv);
}
