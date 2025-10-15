section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 24
    mov DWORD [rbp - 16], 2
    mov DWORD [rbp - 20], 5
    add DWORD [rbp - 20], 1
    mov r10d, DWORD [rbp - 20]
    add DWORD [rbp - 16], r10d
    mov r10d, DWORD [rbp - 16]
    mov DWORD [rbp - 12], r10d
    add DWORD [rbp - 12], 5
    mov r10d, DWORD [rbp - 12]
    mov DWORD [rbp - 8], r10d
    add DWORD [rbp - 8], 1
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r10d
    mov DWORD [rbp - 24], 1
    neg DWORD [rbp - 24]
    mov r10d, DWORD [rbp - 24]
    add DWORD [rbp - 4], r10d
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
