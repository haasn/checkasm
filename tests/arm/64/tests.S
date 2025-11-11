/******************************************************************************
 * Copyright © 2025 Martin Storsjo
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
 *****************************************************************************/

#include "src/config.h"
#include "src/arm/asm.S"

function sigill_aarch64, export=1
        .inst 0 // udf #0
        ret
endfunc

.macro clobber_x reg
function clobber_\reg, export=1
        mov             \reg, #0x42
        ret
endfunc
.endm

.macro clobber_d reg
function clobber_d\reg, export=1
        movi            v\reg\().8b, #0x42
        ret
endfunc
.endm

.macro clobber_v_upper reg
function clobber_v\reg\()_upper, export=1
        str             d\reg, [sp, #-16]!
        movi            v\reg\().16b, #0x42
        ldr             d\reg, [sp], #16
        ret
endfunc
.endm

.irp r, x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17
clobber_x \r
.endr
// x18 skipped
.irp r, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29
clobber_x \r
.endr

.irp r, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
clobber_d \r
.endr

.irp r, 8, 9, 10, 11, 12, 13, 14, 15
clobber_v_upper \r
.endr

// void clobber_stack_args_aarch64(int, int, int, int, int, int, int, int, int, int, int);
//
// Write into the stack of the caller, overwriting the arguments that are on
// the stack. This is legal to do.
function clobber_stack_args_aarch64, export=1
        mov             x0, #0x42
        str             x0, [sp, #0]
        str             x0, [sp, #8]
        str             x0, [sp, #16]
        ret
endfunc

// void clobber_stack_aarch64(int, int, int, int, int, int, int, int, int, int, int);
//
// Write into the stack of the caller, writing past the arguments on the stack
// that we are allowed to touch.
function clobber_stack_aarch64, export=1
        mov             x0, #0x42
        str             x0, [sp, #0]
        str             x0, [sp, #8]
        str             x0, [sp, #16]
        str             x0, [sp, #24]
        ret
endfunc

// void check_clobber_upper_aarch64(int, int, int, int, int, int, int, int, int, int, int);
//
// Check that the upper half of parameters that are passed as int actually
// are filled with garbage.
//
// This test has a bit of an inverted role; this test tries to verify the
// actions of the checked_call wrapper. If it finds a fault, it triggers
// a crash, to indicate a fault.
.macro check_clobber_reg reg
function check_clobber_upper_\reg\()_aarch64, export=1
        lsr             \reg, \reg, #32
        cbnz            \reg, 9f
        .inst 0 // udf #0
9:
        ret
endfunc
.endm

.macro check_clobber_stack arg
function check_clobber_upper_stack\arg\()_aarch64, export=1
        ldr             x0, [sp, #8*\arg]
        lsr             x0, x0, #32
        cbnz            x0, 9f
        .inst 0 // udf #0
9:
        ret
endfunc
.endm

.irpc r, 01234567
check_clobber_reg x\r
.endr

.irpc r, 012
check_clobber_stack \r
.endr
