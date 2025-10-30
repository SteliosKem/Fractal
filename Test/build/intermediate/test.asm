extern 
section .text
global _main
_main:
    push rbp
    mov rbp, rsp
    sub rsp, 4
    mov DWORD [rbp - 4], 200
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
