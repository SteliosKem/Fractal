section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 24
    mov DWORD [rbp - 12], 2
    mov DWORD [rbp - 16], 5
    add DWORD [rbp - 16], 1
    mov r10d, DWORD [rbp - 16]
    sub DWORD [rbp - 12], r10d
    mov r10d, DWORD [rbp - 12]
    mov DWORD [rbp - 8], r10d
    sub DWORD [rbp - 8], 5
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r10d
    mov DWORD [rbp - 24], 1
    add DWORD [rbp - 24], 1
    mov r10d, DWORD [rbp - 24]
    mov DWORD [rbp - 20], r10d
    neg DWORD [rbp - 20]
    mov r10d, DWORD [rbp - 20]
    add DWORD [rbp - 4], r10d
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
