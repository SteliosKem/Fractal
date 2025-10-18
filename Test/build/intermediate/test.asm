section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 28
    mov DWORD [rbp - 8], 5
    add DWORD [rbp - 8], 1
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r10d
    mov DWORD [rbp - 16], 9
    mov r11d, DWORD [rbp - 16]
    imul r11d, 2
    mov DWORD [rbp - 16], r11d
    mov r10d, DWORD [rbp - 16]
    mov DWORD [rbp - 12], r10d
    add DWORD [rbp - 12], 1
    mov r10d, DWORD [rbp - 12]
    mov DWORD [rbp - 4], r10d
    mov DWORD [rbp - 20], 100
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 24], r10d
    mov r10d, DWORD [rbp - 20]
    mov DWORD [rbp - 28], r10d
    mov r11d, DWORD [rbp - 28]
    imul r11d, 2
    mov DWORD [rbp - 28], r11d
    mov r10d, DWORD [rbp - 28]
    add DWORD [rbp - 24], r10d
    mov eax, DWORD [rbp - 24]
    mov rsp, rbp
    pop rbp
    ret
