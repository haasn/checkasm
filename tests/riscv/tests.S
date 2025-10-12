/******************************************************************************
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2023, Nathan Egge
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

#define PRIVATE_PREFIX checkasm_

#include "src/config.h"
#include "src/riscv/asm.S"

function get_vlenb, export=1, ext=v
  csrr a0, vlenb
  ret
endfunc

function copy_rvv, export=1, ext=v
  vsetvli t0, a2, e8, m8, ta, ma
  vle8.v v0, (a1)
  add a1, a1, t0
  sub a2, a2, t0
  vse8.v v0, (a0)
  add a0, a0, t0
  bnez a2, copy_rvv
  ret
endfunc

.macro clobber_fn reg
  function clobber_\reg, export=1
    li \reg, 0
    ret
  endfunc
.endm

.irp reg, t0, t1, t2, s0, s1, a0, a1, a2, a3, a4, a5, a6, a7, \
          s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, t3, t4, t5, t6
clobber_fn \reg
.endr

function clobber_ra, export=1
  mv t0, ra
  li ra, 0
  jr t0
endfunc
