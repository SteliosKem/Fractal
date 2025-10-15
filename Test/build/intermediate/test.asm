section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 4
    mov DWORD [rbp - 4], 10
    not DWORD [rbp - 4]
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
