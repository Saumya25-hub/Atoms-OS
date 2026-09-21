/*
 * ATOMS OS — Userspace C Runtime setjmp.h
 * System V x86_64 ABI Non-Local Jumps
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_USER_SETJMP_H
#define ATOMS_USER_SETJMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * jmp_buf for x86_64 System V ABI:
 * Stores 8 64-bit registers:
 * [0] = rbx
 * [1] = rsp
 * [2] = rbp
 * [3] = r12
 * [4] = r13
 * [5] = r14
 * [6] = r15
 * [7] = rip (return address)
 */
typedef uint64_t jmp_buf[8];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_SETJMP_H */
