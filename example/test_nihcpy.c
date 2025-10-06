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

static void check_nihcpy(nihcpy_func fun, size_t size)
{
    int8_t *src   = malloc(size);
    int8_t *c_dst = malloc(size);
    int8_t *a_dst = malloc(size);

    declare_func(void,
        void *dest, const void *src, size_t n);

    if (check_func(fun, "nihcpy_%zu", size)) {

        /* Initialize the source buffer */
        for (size_t i = 0; i < size; i++)
            src[i] = rnd() & 7;

        memset(c_dst, 0x88, size);
        memset(a_dst, 0x88, size);

        call_ref(c_dst, src, size);
        call_new(a_dst, src, size);
        checkasm_check(int8_t, c_dst, size, a_dst, size, size, 1, "nihcpy_data");

        bench_new(a_dst, src, size);
    }

    report("nihcpy_%zu", size);
}

void checkasm_check_nihcpy(void)
{
    nihcpy_func fun = get_nihcpy_func();
    check_nihcpy(fun, 3);
    check_nihcpy(fun, 8);
    check_nihcpy(fun, 16);
    check_nihcpy(fun, 512);
    check_nihcpy(fun, 2049);
}
