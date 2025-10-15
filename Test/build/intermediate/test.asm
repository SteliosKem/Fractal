section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 20
    mov DWORD [rbp - 4], 2
    mov DWORD [rbp - 8], 2
    mov DWORD [rbp - 20], 5
    add DWORD [rbp - 20], 1
    mov r10d, DWORD [rbp - 20]
    mov DWORD [rbp - 16], r10d
    mov r11d, DWORD [rbp - 16]
    imul r11d, 3
    mov DWORD [rbp - 16], r11d
    mov r10d, DWORD [rbp - 16]
    mov DWORD [rbp - 12], r10d
    mov r11d, DWORD [rbp - 12]
    imul r11d, 2
    mov DWORD [rbp - 12], r11d
    mov r10d, DWORD [rbp - 12]
    add DWORD [rbp - 8], r10d
    mov r11d, DWORD [rbp - 4]
    imul r11d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r11d
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
