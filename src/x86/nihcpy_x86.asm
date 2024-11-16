%ifdef PREFIX
    %define mangle(x) _ %+ x
%else
    %define mangle(x) x
%endif

global    mangle(nihcpy_x86)

mangle(nihcpy_x86):
    mov   rax, rdx
    mov   rdi, rdx
    rep   movsb
    ret
