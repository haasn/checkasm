/*
 * Copyright © 2024, Marvin Scholz
 * Copyright © 2019, VideoLAN and dav1d authors
 * Copyright © 2019, Two Orioles, LLC
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"

typedef uint8_t pixel;
#define PIXEL_TYPE uint8_t

void checkasm_test_copy(copy_func fun, const char *name)
{
#define WIDTH 256
    PIXEL_RECT(c_dst, WIDTH, 1);
    PIXEL_RECT(a_dst, WIDTH, 1);

    ALIGN_STK_64(uint8_t, src, WIDTH, );
    RANDOMIZE_BUF(src);

    declare_func(void, uint8_t *dest, const uint8_t *src, size_t n);

    for (size_t w = 1; w <= WIDTH; w *= 2) {
        if (check_func(fun, "%s_%zu", name, w)) {
            CLEAR_PIXEL_RECT(c_dst);
            CLEAR_PIXEL_RECT(a_dst);

            call_ref(c_dst, src, w);
            call_new(a_dst, src, w);
            checkasm_check_pixel_padded(c_dst, c_dst_stride, a_dst, a_dst_stride, w, 1, "dst data");

            bench_new(a_dst, src, w);
        }
    }

    report("%s", name);
}

void checkasm_test_noop(noop_func fun, const char *name)
{
    declare_func(void, int);

    if (check_func(fun, "%s", name)) {
        call_ref(0);
        call_new(0);
    }

    report("%s", name);
}

static DEF_COPY_FUNC(memset)
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
    if (size < 4)
        return;
    memcpy(dst, src, size - 4);
}

static DEF_NOOP_FUNC(segfault)
{
    volatile int *bad = NULL;
    *bad              = 0;
}

DEF_COPY_GETTER(CHECKASM_CPU_FLAG_BAD_C, memset)
DEF_COPY_GETTER(CHECKASM_CPU_FLAG_BAD_C, overwrite_left)
DEF_COPY_GETTER(CHECKASM_CPU_FLAG_BAD_C, overwrite_right)
DEF_COPY_GETTER(CHECKASM_CPU_FLAG_BAD_C, underwrite)
DEF_NOOP_GETTER(CHECKASM_CPU_FLAG_BAD_C, segfault)

void checkasm_check_generic(void)
{
    checkasm_test_copy(checkasm_copy_c, "copy");

    checkasm_should_fail(1);
    checkasm_test_copy(get_memset(), "memset");
    checkasm_test_copy(get_overwrite_left(), "overwrite_left");
    checkasm_test_copy(get_overwrite_right(), "overwrite_right");
    checkasm_test_copy(get_underwrite(), "underwrite");
    checkasm_test_noop(get_segfault(), "segfault");
}
