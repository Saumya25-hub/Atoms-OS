/*
 * ATOMS OS — Userspace CRT0 Entry Point (crt0.S)
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

.global _start
.extern main
.extern exit
.extern __libc_init_array

.section .text
_start:
    xorq %rbp, %rbp
    andq $-16, %rsp
    call __libc_init_array
    movq $0, %rdi
    movq $0, %rsi
    call main
    movq %rax, %rdi
    call exit
.Lhalt:
    hlt
    jmp .Lhalt
