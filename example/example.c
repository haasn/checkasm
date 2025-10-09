#include <string.h>
#include <assert.h>

#include "example.h"

/* Example CPU flags (dummy) */
static uint64_t cpu_flag_mask = (uint64_t) -1;
uint64_t example_get_cpu_flags(void)
{
    return cpu_flag_mask;
}

void example_set_cpu_flags(uint64_t flags)
{
    cpu_flag_mask = flags;
}

/* Good functions */
DEF_COPY_FUNC(blockcopy_x86);
DEF_COPY_FUNC(blockcopy_mmx);
DEF_COPY_FUNC(blockcopy_sse2);
DEF_COPY_FUNC(blockcopy_avx2);
DEF_COPY_FUNC(blockcopy_avx512);

DEF_NOOP_FUNC(clobber_r0);
DEF_NOOP_FUNC(clobber_r1);
DEF_NOOP_FUNC(clobber_r2);
DEF_NOOP_FUNC(clobber_r3);
DEF_NOOP_FUNC(clobber_r4);
DEF_NOOP_FUNC(clobber_r5);
DEF_NOOP_FUNC(clobber_r6);
DEF_NOOP_FUNC(clobber_r7);
DEF_NOOP_FUNC(clobber_r8);

static DEF_COPY_FUNC(blockcopy_c)
{
    memcpy(dst, src, size);
}

static DEF_NOOP_FUNC(noop_c) { }

/* Bad functions */
DEF_NOOP_FUNC(sigill_x86);

static DEF_COPY_FUNC(wrong)
{
    memset(dst, 0xFF, size);
}

static DEF_COPY_FUNC(overwrite_left)
{
    memcpy(dst, src, size);
    dst[-1] = 0xFF;
}

static DEF_COPY_FUNC(overwrite_right)
{
    memcpy(dst, src, size);
    dst[size] = 0xFF;
}

static DEF_COPY_FUNC(underwrite)
{
    memcpy(dst, src, size - 4);
}

static DEF_NOOP_FUNC(segfault)
{
    volatile int *bad = NULL;
    *bad = 0;
}

/* Ugly functions */
DEF_COPY_FUNC(blockcopy_noemms_mmx);
DEF_COPY_FUNC(blockcopy_novzeroupper_avx2);
DEF_NOOP_FUNC(corrupt_stack_x86);

DEF_NOOP_FUNC(clobber_r9);
DEF_NOOP_FUNC(clobber_r10);
DEF_NOOP_FUNC(clobber_r11);
DEF_NOOP_FUNC(clobber_r12);
DEF_NOOP_FUNC(clobber_r13);
DEF_NOOP_FUNC(clobber_r14);
DEF_NOOP_FUNC(clobber_rsp);

/* Function getters */
copy_func *get_good_blockcopy(void)
{
    const unsigned flags = example_get_cpu_flags();
#if ARCH_X86_64
    if (flags & EXAMPLE_CPU_FLAG_AVX512) return example_blockcopy_avx512;
    if (flags & EXAMPLE_CPU_FLAG_AVX2)   return example_blockcopy_avx2;
    if (flags & EXAMPLE_CPU_FLAG_SSE2)   return example_blockcopy_sse2;
    if (flags & EXAMPLE_CPU_FLAG_MMX)    return example_blockcopy_mmx;
    if (flags & EXAMPLE_CPU_FLAG_X86)    return example_blockcopy_x86;
#endif
    return example_blockcopy_c;
}

int num_preserved_regs(void)
{
#if ARCH_X86_64
    #if _WIN32
        return 7;
    #else
        return 9;
    #endif
#else
    return 0;
#endif
}

noop_func *get_clobber(int reg)
{
    const unsigned flag = example_get_cpu_flags() & EXAMPLE_CPU_FLAG_X86;
    switch (reg) {
    case 0:  return flag ? example_clobber_r0  : example_noop_c;
    case 1:  return flag ? example_clobber_r1  : example_noop_c;
    case 2:  return flag ? example_clobber_r2  : example_noop_c;
    case 3:  return flag ? example_clobber_r3  : example_noop_c;
    case 4:  return flag ? example_clobber_r4  : example_noop_c;
    case 5:  return flag ? example_clobber_r5  : example_noop_c;
    case 6:  return flag ? example_clobber_r6  : example_noop_c;
    case 7:  return flag ? example_clobber_r7  : example_noop_c;
    case 8:  return flag ? example_clobber_r8  : example_noop_c;
    case 9:  return flag ? example_clobber_r9  : example_noop_c;
    case 10: return flag ? example_clobber_r10 : example_noop_c;
    case 11: return flag ? example_clobber_r11 : example_noop_c;
    case 12: return flag ? example_clobber_r12 : example_noop_c;
    case 13: return flag ? example_clobber_r13 : example_noop_c;
    case 14: return flag ? example_clobber_r14 : example_noop_c;
    /* can't clobber rsp without completely crashing the program */
    default: return NULL;
    }
}

copy_func *get_bad_wrong(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_BAD_C) return example_wrong;
    return example_blockcopy_c;
}

copy_func *get_bad_overwrite_left(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_BAD_C) return example_overwrite_left;
    return example_blockcopy_c;
}

copy_func *get_bad_overwrite_right(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_BAD_C) return example_overwrite_right;
    return example_blockcopy_c;
}

noop_func *get_bad_segfault(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_BAD_C) return example_segfault;
    return example_noop_c;
}

noop_func *get_bad_sigill(void)
{
    const unsigned flags = example_get_cpu_flags();
#if ARCH_X86_64
    if (flags & EXAMPLE_CPU_FLAG_X86) return example_sigill_x86;
#endif
    return example_noop_c;
}

copy_func *get_bad_underwrite(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_BAD_C) return example_underwrite;
    return example_blockcopy_c;
}

copy_func *get_ugly_noemms(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_MMX) return example_blockcopy_noemms_mmx;
    return example_blockcopy_c;
}

copy_func *get_ugly_novzeroupper(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_AVX2) return example_blockcopy_novzeroupper_avx2;
    return example_blockcopy_c;
}

noop_func *get_ugly_stack(void)
{
    const unsigned flags = example_get_cpu_flags();
    if (flags & EXAMPLE_CPU_FLAG_X86) return example_corrupt_stack_x86;
    return example_noop_c;
}
