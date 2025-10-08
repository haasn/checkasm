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

#include <checkasm/test.h>
#include "example.h"

typedef uint8_t pixel;
#define PIXEL_TYPE uint8_t

static void check_copy(copy_func fun, const char *name)
{
    #define WIDTH 128
    PIXEL_RECT(c_dst, WIDTH, 1);
    PIXEL_RECT(a_dst, WIDTH, 1);
    ALIGN_STK_64(uint8_t, src, WIDTH, );

    /* Initialize the source buffer */
    for (int i = 0; i < WIDTH; i++)
        src[i] = (uint8_t) rnd();

    declare_func(void, uint8_t *dest, const uint8_t *src, size_t n);

    for (size_t w = 8; w <= WIDTH; w *= 2) {
        if (check_func(fun, "%s_%zu", name, w)) {
            CLEAR_PIXEL_RECT(c_dst);
            CLEAR_PIXEL_RECT(a_dst);

            call_ref(c_dst, src, w);
            call_new(a_dst, src, w);
            checkasm_check_pixel_padded(c_dst, c_dst_stride,
                                        a_dst, a_dst_stride,
                                        w, 1, "dst data");

            bench_new(a_dst, src, w);
        }
    }

    report("%s", name);
}

static void check_noop(noop_func fun, const char *name)
{
    declare_func(void, int);

    if (check_func(fun, "%s", name)) {
        call_ref(0);
        call_new(0);
    }

    report("%s", name);
}

static void check_clobber(int num)
{
    declare_func(void, int);

    for (int reg = 0; reg < num; reg++) {
        noop_func *clobber = get_clobber(reg);
        if (!clobber)
            break;

        if (check_func(clobber, "clobber_r%d", reg))
            call_new(0);
    }

    report("clobber");
}

void checkasm_check_good(void)
{
    check_copy(get_good_blockcopy(),        "blockcopy");
    check_clobber(num_preserved_regs());
}

void checkasm_check_bad(void)
{
    checkasm_should_fail(1);

    check_copy(get_bad_wrong(),             "wrong");
    check_copy(get_bad_overwrite_left(),    "overwrite_left");
    check_copy(get_bad_overwrite_right(),   "overwrite_right");
    check_copy(get_bad_underwrite(),        "underwrite");

    check_noop(get_bad_segfault(),          "segfault");
    check_noop(get_bad_sigill(),            "sigill");

    checkasm_should_fail(0);
}

void checkasm_check_ugly(void)
{
    checkasm_should_fail(1);

    check_copy(get_ugly_noemms(),           "noemms");
    check_copy(get_ugly_novzeroupper(),     "novzeroupper");
    check_noop(get_ugly_stack(),            "corrupt_stack");

    check_clobber(NUM_REGS);

    checkasm_should_fail(0);
}
