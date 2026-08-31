; ATOMS OS — Userspace Application C Runtime Bootstrap (crt0.asm)
; Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
; Sets up 16-byte stack alignment and calls main(argc, argv)

global _start
extern main
extern exit

section .text
_start:
    ; Terminate stack frame linkage
    xor rbp, rbp

    ; Pop argc and argv from initial stack if provided, or default to 0
    mov rdi, 0          ; argc = 0
    mov rsi, 0          ; argv = NULL

    ; Ensure 16-byte stack alignment for SSE / C++ ABI
    and rsp, -16

    ; Call application main()
    call main

    ; Pass return value from main() into exit(status)
    mov rdi, rax
    call exit

    ; Infinite halt if exit returns
.halt:
    hlt
    jmp .halt
