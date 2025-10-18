section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov DWORD [rbp - 4], 5
    mov eax, DWORD [rbp - 4]
    cmp eax, 5
    sete BYTE [rbp - 8]
    movsx eax, BYTE [rbp - 8]
    mov eax, eax
    cmp eax, 0
    je .IF0
    mov DWORD [rbp - 4], 8
    mov DWORD [rbp - 4], 5
    jmp .IE0
.IF0:
    mov DWORD [rbp - 4], 1
.IE0:
    mov eax, DWORD [rbp - 4]
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
