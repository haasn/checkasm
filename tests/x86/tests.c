#include <checkasm/test.h>

#include "tests.h"

uint64_t checkasm_get_cpu_flags_x86(void)
{
    // TODO: implement runtime CPU detection
    return CHECKASM_CPU_FLAG_X86 |
           CHECKASM_CPU_FLAG_MMX |
           CHECKASM_CPU_FLAG_SSE2 |
           CHECKASM_CPU_FLAG_AVX2 |
           CHECKASM_CPU_FLAG_AVX512;
}

DEF_COPY_FUNC(copy_x86);
DEF_COPY_FUNC(copy_mmx);
DEF_COPY_FUNC(copy_sse2);
DEF_COPY_FUNC(copy_avx2);
DEF_COPY_FUNC(copy_avx512);

DEF_NOOP_FUNC(clobber_r0);
DEF_NOOP_FUNC(clobber_r1);
DEF_NOOP_FUNC(clobber_r2);
DEF_NOOP_FUNC(clobber_r3);
DEF_NOOP_FUNC(clobber_r4);
DEF_NOOP_FUNC(clobber_r5);
DEF_NOOP_FUNC(clobber_r6);
#if ARCH_X86_64
DEF_NOOP_FUNC(clobber_r7);
DEF_NOOP_FUNC(clobber_r8);
DEF_NOOP_FUNC(clobber_r9);
DEF_NOOP_FUNC(clobber_r10);
DEF_NOOP_FUNC(clobber_r11);
DEF_NOOP_FUNC(clobber_r12);
DEF_NOOP_FUNC(clobber_r13);
DEF_NOOP_FUNC(clobber_r14);
#endif

DEF_NOOP_FUNC(sigill_x86);
DEF_NOOP_FUNC(corrupt_stack_x86);

DEF_COPY_FUNC(copy_noemms_mmx);
DEF_COPY_FUNC(copy_novzeroupper_avx2);

static copy_func *get_copy_x86(void)
{
    const unsigned flags = checkasm_get_cpu_flags();
#if ARCH_X86
    if (flags & CHECKASM_CPU_FLAG_AVX512) return checkasm_copy_avx512;
    if (flags & CHECKASM_CPU_FLAG_AVX2)   return checkasm_copy_avx2;
    if (flags & CHECKASM_CPU_FLAG_SSE2)   return checkasm_copy_sse2;
    if (flags & CHECKASM_CPU_FLAG_MMX)    return checkasm_copy_mmx;
    if (flags & CHECKASM_CPU_FLAG_X86)    return checkasm_copy_x86;
#endif
    return checkasm_copy_c;
}

#if ARCH_X86_64
    #if _WIN32
        #define NUM_SAFE 7
    #else
        #define NUM_SAFE 9
    #endif
    #define NUM_REGS 15
#else
    #define NUM_SAFE 3
    #define NUM_REGS 7
#endif

static noop_func *get_clobber(int reg)
{
    if (!(checkasm_get_cpu_flags() & CHECKASM_CPU_FLAG_X86))
        return NULL;

    switch (reg) {
    case 0:  return checkasm_clobber_r0;
    case 1:  return checkasm_clobber_r1;
    case 2:  return checkasm_clobber_r2;
    case 3:  return checkasm_clobber_r3;
    case 4:  return checkasm_clobber_r4;
    case 5:  return checkasm_clobber_r5;
    case 6:  return checkasm_clobber_r6;
#if ARCH_X86_64
    case 7:  return checkasm_clobber_r7;
    case 8:  return checkasm_clobber_r8;
    case 9:  return checkasm_clobber_r9;
    case 10: return checkasm_clobber_r10;
    case 11: return checkasm_clobber_r11;
    case 12: return checkasm_clobber_r12;
    case 13: return checkasm_clobber_r13;
    case 14: return checkasm_clobber_r14;
#endif
    /* can't clobber rsp without completely crashing the program */
    default: return NULL;
    }
}

DEF_NOOP_GETTER(CHECKASM_CPU_FLAG_X86,  sigill_x86)
DEF_NOOP_GETTER(CHECKASM_CPU_FLAG_X86,  corrupt_stack_x86)
DEF_COPY_GETTER(CHECKASM_CPU_FLAG_MMX,  copy_noemms_mmx)
DEF_COPY_GETTER(CHECKASM_CPU_FLAG_AVX2, copy_novzeroupper_avx2)

static void check_clobber(int from, int to)
{
    declare_func(void, int);

    for (int reg = from; reg < to; reg++) {
        noop_func *clobber = get_clobber(reg);
        if (!clobber)
            break;

        if (check_func(clobber, "clobber_r%d", reg)) {
            call_new(0);
        }
    }

    report("clobber");
}

void checkasm_check_x86(void)
{
    checkasm_test_copy(get_copy_x86(),                  "copy");
    check_clobber(0, NUM_SAFE);

    checkasm_should_fail(1);
    checkasm_test_noop(get_sigill_x86(),                "sigill");
    checkasm_test_noop(get_corrupt_stack_x86(),         "corrupt_stack");
    checkasm_test_copy(get_copy_noemms_mmx(),           "noemms");
    checkasm_test_copy(get_copy_novzeroupper_avx2(),    "novzeroupper");
    check_clobber(NUM_SAFE, NUM_REGS);
    checkasm_should_fail(0);
}
