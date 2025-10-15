section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 20
    mov DWORD [rbp - 16], 2
    mov DWORD [rbp - 20], 5
    add DWORD [rbp - 20], 1
    mov r10d, DWORD [rbp - 20]
    add DWORD [rbp - 16], r10d
    mov r10d, DWORD [rbp - 16]
    mov DWORD [rbp - 12], r10d
    sub DWORD [rbp - 12], 5
    mov r10d, DWORD [rbp - 12]
    mov DWORD [rbp - 8], r10d
    sub DWORD [rbp - 8], 1
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r10d
    sub DWORD [rbp - 4], 1
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
