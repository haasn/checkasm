%undef private_prefix
%define private_prefix example

%include "src/config.asm"
%include "src/x86/x86inc.asm"

extern memcpy

SECTION .text

%macro blockcopy_vec 0-1 ; suffix
cglobal blockcopy%1, 3, 3, 1, dst, src, size
    cmp     sizeq, mmsize
    jl memcpy

    add     dstq, sizeq
    add     srcq, sizeq
    neg     sizeq
.loop:
    mova    m0, [srcq + sizeq]
    mova    [dstq + sizeq], m0
    add     sizeq, mmsize
    jl .loop

    ; emit emms after all MMX functions unless suffix is _noemms
%ifnidn %1, _noemms
  %if mmsize == 8
    emms
  %endif
%endif

    ; skip vzeroupper if suffix is _novzeroupper
%ifidn %1, _novzeroupper
    ret
%else
    RET
%endif
%endmacro

; Baseline x86 functions
cglobal blockcopy_x86, 3, 3, 0, dst, src, size
    mov     rcx, sizeq
    rep     movsb
    RET

cglobal sigill_x86
    ud2

%macro clobber 2 ; register, suffix
cglobal clobber%2
    xor     %1, %1
    RET
%endmacro

; should succeed
clobber r0,  _r0
clobber r1,  _r1
clobber r2,  _r2
clobber r3,  _r3
clobber r4,  _r4
clobber r5,  _r5
clobber r6,  _r6
clobber r7,  _r7
clobber r8,  _r8
; should fail
clobber r9,  _r9
clobber r10, _r10
clobber r11, _r11
clobber r12, _r12
clobber r13, _r13
clobber r14, _r14

cglobal corrupt_stack_x86
    xor rax, rax
    mov [rsp+8], rax
    RET

; MMX functions
INIT_MMX mmx
blockcopy_vec
blockcopy_vec _noemms

cglobal noemms, 3, 3, 0, dst, src, size
    RET

; SSE2 functions
INIT_XMM sse2
blockcopy_vec

; AVX2 functions
INIT_YMM avx2
blockcopy_vec
blockcopy_vec _novzeroupper

; AVX512 functions
INIT_YMM avx512
blockcopy_vec
