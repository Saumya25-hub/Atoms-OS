; ============================================================
; ATOMS OS — Userspace x86_64 System V ABI setjmp / longjmp
; Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
; ============================================================

global setjmp
global longjmp

section .text

; int setjmp(jmp_buf env);
; System V ABI: rdi = env (pointer to 8 uint64_t)
setjmp:
    mov [rdi + 0],  rbx
    mov [rdi + 8],  rsp
    mov [rdi + 16], rbp
    mov [rdi + 24], r12
    mov [rdi + 32], r13
    mov [rdi + 40], r14
    mov [rdi + 48], r15
    mov rdx, [rsp]          ; Return address of caller
    mov [rdi + 56], rdx
    xor eax, eax            ; Returns 0 on direct call
    ret

; void longjmp(jmp_buf env, int val);
; System V ABI: rdi = env, rsi = val
longjmp:
    mov rbx, [rdi + 0]
    mov rsp, [rdi + 8]
    mov rbp, [rdi + 16]
    mov r12, [rdi + 24]
    mov r13, [rdi + 32]
    mov r14, [rdi + 40]
    mov r15, [rdi + 48]
    mov rdx, [rdi + 56]     ; Saved RIP
    mov [rsp], rdx          ; Put saved RIP on top of stack for ret

    mov eax, esi            ; Return value
    test eax, eax
    jnz .valid_val
    mov eax, 1              ; If val was 0, longjmp returns 1
.valid_val:
    ret
