/*
 * Copyright © 2025, Martin Storsjo
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

#include <stdint.h>

#include "checkasm_config.h"
#include "cpu.h"
#include "internal.h"

#if ARCH_AARCH64

  #ifdef _WIN32
    #include <windows.h>
  #elif defined(__APPLE__)
    #include <sys/sysctl.h>

static int have_feature(const char *feature)
{
    int    supported = 0;
    size_t size      = sizeof(supported);
    if (sysctlbyname(feature, &supported, &size, NULL, 0) != 0) {
        return 0;
    }
    return supported;
}
  #elif HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO

    #include <sys/auxv.h>

    #define HWCAP_AARCH64_SVE  (1 << 22)
    #define HWCAP2_AARCH64_SME (1 << 23)

  #endif

int checkasm_has_sve(void)
{
  #ifdef _WIN32
    #ifdef PF_ARM_SVE_INSTRUCTIONS_AVAILABLE
    return IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE);
    #else
    return 0;
    #endif
  #elif defined(__APPLE__)
    /* No sysctlbyname feature defined for SVE yet */
    return 0;
  #elif HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
    return checkasm_getauxval(AT_HWCAP) & HWCAP_AARCH64_SVE;
  #else
    return 0;
  #endif
}

int checkasm_has_sme(void)
{
  #ifdef _WIN32
    #ifdef PF_ARM_SME_INSTRUCTIONS_AVAILABLE
    return IsProcessorFeaturePresent(PF_ARM_SME_INSTRUCTIONS_AVAILABLE);
    #else
    return 0;
    #endif
  #elif defined(__APPLE__)
    return have_feature("hw.optional.arm.FEAT_SME");
  #elif HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
    return checkasm_getauxval(AT_HWCAP2) & HWCAP2_AARCH64_SME;
  #else
    return 0;
  #endif
}

#elif ARCH_ARM

  #if HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
    #include <sys/auxv.h>

    #ifndef HWCAP_ARM_VFP
      #define HWCAP_ARM_VFP (1 << 6)
    #endif
    #ifndef HWCAP_ARM_NEON
      #define HWCAP_ARM_NEON (1 << 12)
    #endif
    #ifndef HWCAP_ARM_VFPD32
      #define HWCAP_ARM_VFPD32 (1 << 19)
    #endif

  #endif

int checkasm_has_vfpd32(void)
{
  #if (defined(__APPLE__) && __ARM_ARCH >= 7) || defined(_WIN32)
    return 1;
  #elif HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
    /* HWCAP_ARM_VFPD32 was introduced in Linux 3.7, alternatively check for NEON as a fallback */
    return checkasm_getauxval(AT_HWCAP) & (HWCAP_ARM_NEON | HWCAP_ARM_VFPD32);
  #else
    return 0;
  #endif
}

int checkasm_has_vfp(void)
{
  #if defined(__APPLE__) || defined(_WIN32)
    return 1;
  #elif HAVE_GETAUXVAL || HAVE_ELF_AUX_INFO
    return checkasm_getauxval(AT_HWCAP) & HWCAP_ARM_VFP;
  #else
    return 0;
  #endif
}

#endif // ARCH_ARM
