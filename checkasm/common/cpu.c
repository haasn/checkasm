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

#include <errno.h>

#include "cpu.h"

#if HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
#include <sys/auxv.h>
#endif

unsigned checkasm_cpu_flags = 0U;
unsigned checkasm_cpu_flags_mask = ~0U;

COLD void checkasm_init_cpu(void) {
#if HAVE_ASM && !__has_feature(memory_sanitizer)
// memory sanitizer is inherently incompatible with asm
#if ARCH_AARCH64 || ARCH_ARM
    checkasm_cpu_flags = checkasm_get_cpu_flags_arm();
#elif ARCH_LOONGARCH
    checkasm_cpu_flags = checkasm_get_cpu_flags_loongarch();
#elif ARCH_PPC64LE
    checkasm_cpu_flags = checkasm_get_cpu_flags_ppc();
#elif ARCH_RISCV
    checkasm_cpu_flags = checkasm_get_cpu_flags_riscv();
#elif ARCH_X86
    checkasm_cpu_flags = checkasm_get_cpu_flags_x86();
#endif
#endif
}

COLD void checkasm_set_cpu_flags_mask(const unsigned mask) {
    checkasm_cpu_flags_mask = mask;
}

COLD unsigned long checkasm_getauxval(unsigned long type) {
#if HAVE_GETAUXVAL
    return getauxval(type);
#elif HAVE_ELF_AUX_INFO
    unsigned long aux = 0;
    int ret = elf_aux_info(type, &aux, sizeof(aux));
    if (ret != 0)
        errno = ret;
    return aux;
#else
    errno = ENOSYS;
    return 0;
#endif
}
