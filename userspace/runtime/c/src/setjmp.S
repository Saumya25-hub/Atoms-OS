/*
 * ATOMS OS — Userspace x86_64 System V ABI setjmp / longjmp (GNU AS)
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

.global setjmp
.global longjmp

.section .text

/* int setjmp(jmp_buf env); */
setjmp:
    movq %rbx, 0(%rdi)
    movq %rsp, 8(%rdi)
    movq %rbp, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)
    movq (%rsp), %rdx
    movq %rdx, 56(%rdi)
    xorl %eax, %eax
    ret

/* void longjmp(jmp_buf env, int val); */
longjmp:
    movq 0(%rdi), %rbx
    movq 8(%rdi), %rsp
    movq 16(%rdi), %rbp
    movq 24(%rdi), %r12
    movq 32(%rdi), %r13
    movq 40(%rdi), %r14
    movq 48(%rdi), %r15
    movq 56(%rdi), %rdx
    movq %rdx, (%rsp)

    movl %esi, %eax
    testl %eax, %eax
    jnz .Ldone
    movl $1, %eax
.Ldone:
    ret
