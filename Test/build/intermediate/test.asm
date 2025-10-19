section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 12
    mov DWORD [rbp - 4], 0
.LS1:
    mov eax, DWORD [rbp - 4]
    cmp eax, 5
    setl BYTE [rbp - 8]
    movsx eax, BYTE [rbp - 8]
    mov eax, eax
    cmp eax, 0
    je .LE1
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 12], r10d
    add DWORD [rbp - 12], 2
    mov r10d, DWORD [rbp - 12]
    mov DWORD [rbp - 4], r10d
    jmp .LS1
.LE1:
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
