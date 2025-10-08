/*
 * Copyright © 2025, Niklas Haas
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

#ifndef EXAMPLE_CHECKASM_H
#define EXAMPLE_CHECKASM_H

#include <stdint.h>
#include <stddef.h>

#include "src/config.h"

/* Example CPU flags (dummy) */
enum {
    EXAMPLE_CPU_FLAG_BAD_C  = 1 << 0, // dummy flag for "bad" C implementations
#if ARCH_X86
    EXAMPLE_CPU_FLAG_X86    = 1 << 1,
    EXAMPLE_CPU_FLAG_MMX    = 1 << 2,
    EXAMPLE_CPU_FLAG_SSE2   = 1 << 3,
    EXAMPLE_CPU_FLAG_AVX2   = 1 << 4,
    EXAMPLE_CPU_FLAG_AVX512 = 1 << 5,
#endif
};

uint64_t example_get_cpu_flags(void);
void example_set_cpu_flags(uint64_t flags);

/**
 * Copy `size` bytes from `src` to `dst`. `size` is a power of two. All buffers
 * are padded to a multiple of the largest vector size.
 */
typedef void (copy_func)(uint8_t *dst, const uint8_t *src, size_t size);
#define DEF_COPY_FUNC(NAME) \
    void example_##NAME(uint8_t *restrict dst, const uint8_t *restrict src, size_t size)

/**
 * Do nothing. Used to test side effects, stack corruption etc.
 * The singular int parameter is just to have at least one parameter,
 * which is required by `declare_func`.
 */
typedef void (noop_func)(int unused);
#define DEF_NOOP_FUNC(NAME) void example_##NAME(int unused)

copy_func *get_good_blockcopy(void);

copy_func *get_bad_wrong(void);
copy_func *get_bad_overwrite_left(void);
copy_func *get_bad_overwrite_right(void);
copy_func *get_bad_underwrite(void);
noop_func *get_bad_segfault(void);
noop_func *get_bad_sigill(void);

copy_func *get_ugly_noemms(void);
copy_func *get_ugly_novzeroupper(void);
noop_func *get_ugly_stack(void);

/* Test register clobbering */
#define NUM_REGS 16
int num_preserved_regs(void);
noop_func *get_clobber(int reg);

#endif /* EXAMPLE_CHECKASM_H */
