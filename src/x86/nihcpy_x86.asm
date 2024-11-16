global    _nihcpy_x86

_nihcpy_x86:
    mov   rax, rdx
    mov   rdi, rdx
    rep   movsb
    ret
