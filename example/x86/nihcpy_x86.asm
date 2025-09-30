%ifdef PREFIX
    %define mangle(x) _ %+ x
%else
    %define mangle(x) x
%endif

global    mangle(nihcpy_x86)

mangle(nihcpy_x86):
    mov   rax, rdx
    mov   rcx, rdx
    rep   movsb
    ret
