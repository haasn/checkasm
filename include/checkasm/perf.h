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

#ifndef CHECKASM_PERF_H
#define CHECKASM_PERF_H

#include <stdint.h>

#include "checkasm/config.h"
#include "checkasm/attributes.h"

#if CONFIG_LINUX_PERF
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <linux/perf_event.h>
#endif

#if ARCH_X86
    #if defined(_MSC_VER) && !defined(__clang__)
        #include <intrin.h>
        #define readtime() (_mm_lfence(), __rdtsc())
    #else
        static inline uint64_t readtime(void) {
            uint32_t eax, edx;
            __asm__ __volatile__("lfence\nrdtsc" : "=a"(eax), "=d"(edx));
            return (((uint64_t)edx) << 32) | eax;
        }
        #define readtime readtime
    #endif
#elif CONFIG_MACOS_KPERF
    CHECKASM_API uint64_t checkasm_kperf_cycles(void);
    #define readtime() checkasm_kperf_cycles()
#elif (ARCH_AARCH64 || ARCH_ARM) && defined(__APPLE__)
    #include <mach/mach_time.h>
    #define readtime() mach_absolute_time()
#elif ARCH_AARCH64
    #ifdef _MSC_VER
        #include <windows.h>
        #define readtime() (_InstructionSynchronizationBarrier(), ReadTimeStampCounter())
    #else
        static inline uint64_t readtime(void) {
            uint64_t cycle_counter;
            /* This requires enabling user mode access to the cycle counter
             * (which can only be done from kernel space).
             * This could also read cntvct_el0 instead of pmccntr_el0; that register
             * might also be readable (depending on kernel version), but it has much
             * worse precision (it's a fixed 50 MHz timer). */
            __asm__ __volatile__("isb\nmrs %0, pmccntr_el0"
                                : "=r"(cycle_counter)
                                :: "memory");
            return cycle_counter;
        }
        #define readtime readtime
    #endif
#elif ARCH_ARM && !defined(_MSC_VER) && __ARM_ARCH >= 7
    static inline uint64_t readtime(void) {
        uint32_t cycle_counter;
        /* This requires enabling user mode access to the cycle counter (which
         * can only be done from kernel space). */
        __asm__ __volatile__("isb\nmrc p15, 0, %0, c9, c13, 0"
                            : "=r"(cycle_counter)
                            :: "memory");
        return cycle_counter;
    }
    #define readtime readtime
#elif ARCH_PPC64LE
    static inline uint64_t readtime(void) {
        uint32_t tbu, tbl, temp;

        __asm__ __volatile__(
            "1:\n"
            "mfspr %2,269\n"
            "mfspr %0,268\n"
            "mfspr %1,269\n"
            "cmpw   %2,%1\n"
            "bne    1b\n"
        : "=r"(tbl), "=r"(tbu), "=r"(temp)
        :
        : "cc");

        return (((uint64_t)tbu) << 32) | (uint64_t)tbl;
    }
    #define readtime readtime
#elif ARCH_RISCV
    #include <time.h>
    static inline uint64_t clock_gettime_nsec(void) {
        struct timespec ts;
    #ifdef CLOCK_MONOTONIC_RAW
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    #else
        clock_gettime(CLOCK_MONOTONIC, &ts);
    #endif
        return ((uint64_t)ts.tv_sec*1000000000u) + (uint64_t)ts.tv_nsec;
    }
    #define readtime clock_gettime_nsec
#elif ARCH_LOONGARCH
    static inline uint64_t readtime(void) {
    #if ARCH_LOONGARCH64
        uint64_t a, id;
        __asm__ __volatile__("rdtime.d  %0, %1"
                            : "=r"(a), "=r"(id)
                            :: );
        return a;
    #else
        uint32_t a, id;
        __asm__ __volatile__("rdtimel.w  %0, %1"
                            : "=r"(a), "=r"(id)
                            :: );
      return (uint64_t)a;
    #endif
    }
    #define readtime readtime
#endif

#if CONFIG_LINUX_PERF
    CHECKASM_API int checkasm_get_perf_sysfd(void);
    #define PERF_SETUP()\
        int sysfd = checkasm_get_perf_sysfd()
    #define PERF_START(t) do {\
        ioctl(sysfd, PERF_EVENT_IOC_RESET, 0);\
        ioctl(sysfd, PERF_EVENT_IOC_ENABLE, 0);\
    } while (0)
    #define PERF_STOP(t) do {\
        int _ret;\
        ioctl(sysfd, PERF_EVENT_IOC_DISABLE, 0);\
        _ret = read(sysfd, &t, sizeof(t));\
        (void)_ret;\
    } while (0)
#elif defined(readtime)
    #define PERF_SETUP()
    #define PERF_START(t) t = readtime();
    #define PERF_STOP(t)  t = readtime() - t
#endif

#endif /* CHECKASM_PERF_H */
