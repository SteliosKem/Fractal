section .text
global _main
_main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov eax, 2
    cmp eax, 5
    setl BYTE [rbp - 8]
    movsx eax, BYTE [rbp - 8]
    mov eax, eax
    cmp eax, 0
    setl BYTE [rbp - 4]
    movsx eax, BYTE [rbp - 4]
    mov eax, eax
    mov eax, eax
    mov rsp, rbp
    pop rbp
    ret
