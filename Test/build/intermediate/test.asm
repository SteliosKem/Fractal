section .text
global _main
_main:
    push rbp
    mov rbp, rsp
    sub rsp, 0
    mov eax, 5
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
