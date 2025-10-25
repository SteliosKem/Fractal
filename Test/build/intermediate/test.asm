section .text
global fib
fib:
    push rbp
    mov rbp, rsp
    sub rsp, 36
    mov DWORD [rbp - 4], ecx
    mov DWORD [rbp - 8], edx
    mov DWORD [rbp - 12], r8d
    mov DWORD [rbp - 16], r9d
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 36], r10d
    mov r10d, DWORD [rbp - 8]
    add DWORD [rbp - 36], r10d
    mov r10d, DWORD [rbp - 36]
    mov DWORD [rbp - 32], r10d
    mov r10d, DWORD [rbp - 12]
    add DWORD [rbp - 32], r10d
    mov r10d, DWORD [rbp - 32]
    mov DWORD [rbp - 28], r10d
    mov r10d, DWORD [rbp - 16]
    add DWORD [rbp - 28], r10d
    mov r10d, DWORD [rbp - 28]
    mov DWORD [rbp - 24], r10d
    mov r10d, DWORD [rbp + 16]
    add DWORD [rbp - 24], r10d
    mov r10d, DWORD [rbp - 24]
    mov DWORD [rbp - 20], r10d
    mov r10d, DWORD [rbp + 24]
    add DWORD [rbp - 20], r10d
    mov eax, DWORD [rbp - 20]
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov DWORD [rbp - 4], 5
    sub rsp, 40
    mov ecx, DWORD [rbp - 4]
    mov edx, DWORD [rbp - 4]
    mov r8d, DWORD [rbp - 4]
    mov r9d, DWORD [rbp - 4]
    mov r10d, DWORD [rbp - 4]
    mov DWORD [rbp - 8], r10d
    add DWORD [rbp - 8], 2
    movsx rax, DWORD [rbp - 8]
    mov rax, rax
    push rax
    movsx rax, DWORD [rbp - 4]
    mov rax, rax
    push rax
    call fib
    add rsp, 56
    mov eax, eax
    mov rsp, rbp
    pop rbp
    ret
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
