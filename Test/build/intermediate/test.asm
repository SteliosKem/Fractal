extern putchar, 
section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov DWORD [rbp - 4], 0
.LS1:
    mov eax, DWORD [rbp - 4]
    cmp eax, 26
    setl BYTE [rbp - 8]
    movsx eax, BYTE [rbp - 8]
    mov eax, eax
    cmp eax, 0
    je .LE1
    sub rsp, 32
    mov DWORD [rbp - 12], 65
    mov r10d, DWORD [rbp - 4]
    add DWORD [rbp - 12], r10d
    mov ecx, DWORD [rbp - 12]
    call putchar
    add rsp, 32
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 16], r10d
    add DWORD [rbp - 16], 1
    mov r10d, DWORD [rbp - 16]
    mov DWORD [rbp - 4], r10d
    jmp .LS1
.LE1:
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
