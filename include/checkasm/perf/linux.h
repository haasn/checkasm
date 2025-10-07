/*
 * Copyright © 2025, Niklas Haas
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

#ifndef CHECKASM_PERF_LINUX_H
#define CHECKASM_PERF_LINUX_H

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>

#include "checkasm/attributes.h"

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

#endif /* CHECKASM_PERF_LINUX_H */
