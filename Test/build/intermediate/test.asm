section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov DWORD [rbp - 8], 6
    neg DWORD [rbp - 8]
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r10d
    neg DWORD [rbp - 4]
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
