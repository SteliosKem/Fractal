section .text
global fib
fib:
    push rbp
    mov rbp, rsp
    sub rsp, 24
    mov DWORD [rbp - 4], ecx
    mov eax, DWORD [rbp - 4]
    cmp eax, 0
    setg BYTE [rbp - 8]
    movsx eax, BYTE [rbp - 8]
    mov eax, eax
    cmp eax, 0
    je .IF1
    sub rsp, 32
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 16], r10d
    sub DWORD [rbp - 16], 1
    mov ecx, DWORD [rbp - 16]
    call fib
    add rsp, 32
    mov DWORD [rbp - 12], eax
    mov DWORD [rbp - 20], 2
    sub rsp, 32
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 24], r10d
    sub DWORD [rbp - 24], 2
    mov ecx, DWORD [rbp - 24]
    call fib
    add rsp, 32
    mov r11d, DWORD [rbp - 20]
    imul r11d, eax
    mov DWORD [rbp - 20], r11d
    mov r10d, DWORD [rbp - 20]
    add DWORD [rbp - 12], r10d
    mov eax, DWORD [rbp - 12]
    mov rsp, rbp
    pop rbp
    ret
    jmp .IE1
.IF1:
    mov eax, 1
    mov rsp, rbp
    pop rbp
    ret
.IE1:
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 0
    sub rsp, 32
    mov ecx, 5
    call fib
    add rsp, 32
    mov eax, eax
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
