section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 12
    mov DWORD [rbp - 4], 0
.LS1:
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 8], r10d
    add DWORD [rbp - 8], 1
    mov r10d, DWORD [rbp - 8]
    mov DWORD [rbp - 4], r10d
    mov eax, DWORD [rbp - 4]
    cmp eax, 10
    sete BYTE [rbp - 12]
    movsx eax, BYTE [rbp - 12]
    mov eax, eax
    cmp eax, 0
    je .IE2
    jmp .LE1
.IE2:
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
