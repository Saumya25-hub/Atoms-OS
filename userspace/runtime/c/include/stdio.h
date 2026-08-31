/*
 * ATOMS OS — Userspace C Runtime stdio.h
 * Adapted from musl libc (MIT License)
 */

#ifndef ATOMS_USER_STDIO_H
#define ATOMS_USER_STDIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int printf(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

int puts(const char *s);
int putchar(int c);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_STDIO_H */
