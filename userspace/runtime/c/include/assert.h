/*
 * ATOMS OS — Userspace C Runtime assert.h
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_USER_ASSERT_H
#define ATOMS_USER_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

void __assert_fail(const char *expr, const char *file, int line, const char *func) __attribute__((noreturn));

#undef assert
#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_ASSERT_H */
