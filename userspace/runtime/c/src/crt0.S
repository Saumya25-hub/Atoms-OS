/*
 * ATOMS OS — Userspace CRT0 Entry Point (crt0.S)
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

.global _start
.extern main
.extern exit

.section .text
_start:
    xorq %rbp, %rbp
    movq $0, %rdi
    movq $0, %rsi
    andq $-16, %rsp
    call main
    movq %rax, %rdi
    call exit
.Lhalt:
    hlt
    jmp .Lhalt
