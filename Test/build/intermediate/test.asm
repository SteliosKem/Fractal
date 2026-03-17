section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 24
    mov QWORD [rbp - 16], 200
    mov r10, QWORD [rbp - 16]
    mov DWORD [rbp - 8], r10d
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 24], r10d
    mov r10d, DWORD [rbp - 24]
    mov DWORD [rbp - 20], r10d
    mov eax, DWORD [rbp - 20]
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
