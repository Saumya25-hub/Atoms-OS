/*
 * ATOMS OS — Userspace C Runtime Standard Library Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "../include/stdlib.h"
#include "../include/atoms_syscall.h"
#include "../include/stdio.h"
#include "../include/errno.h"
#include "../include/assert.h"
#include <stdint.h>
#include <stddef.h>

/* Global and per-thread errno fallback */
static int s_global_errno = 0;

int *__errno_location(void) {
    return &s_global_errno;
}

/* ELF Constructor / Destructor Arrays */
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));
extern void (*__fini_array_start[])(void) __attribute__((weak));
extern void (*__fini_array_end[])(void) __attribute__((weak));

void __libc_init_array(void) {
    if (__init_array_start && __init_array_end) {
        size_t count = __init_array_end - __init_array_start;
        for (size_t i = 0; i < count; i++) {
            if (__init_array_start[i]) {
                __init_array_start[i]();
            }
        }
    }
}

void __libc_fini_array(void) {
    if (__fini_array_start && __fini_array_end) {
        size_t count = __fini_array_end - __fini_array_start;
        for (size_t i = count; i > 0; i--) {
            if (__fini_array_start[i - 1]) {
                __fini_array_start[i - 1]();
            }
        }
    }
}

void exit(int status) {
    __libc_fini_array();
    __atoms_syscall1(SYS_EXIT, (uint64_t)status);
    while (1) {}
}

void abort(void) {
    puts("[FATAL] Process abort() invoked");
    exit(134);
    while (1) {}
}

void __assert_fail(const char *expr, const char *file, int line, const char *func) {
    printf("[ASSERTION FAILED] %s at %s:%d (%s)\n", expr, file, line, func ? func : "unknown");
    abort();
    while (1) {}
}

int abs(int j) {
    return (j < 0) ? -j : j;
}

long labs(long j) {
    return (j < 0) ? -j : j;
}

long strtol(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long acc;
    char c;
    unsigned long cutoff;
    int neg = 0, any, cutlim;

    if (!s) {
        errno = EINVAL;
        return 0;
    }

    do {
        c = *s++;
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');

    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+') {
        c = *s++;
    }

    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        base = (c == '0') ? 8 : 10;
    }

    cutoff = neg ? -(unsigned long)-9223372036854775808ULL : 9223372036854775807ULL;
    cutlim = cutoff % (unsigned long)base;
    cutoff /= (unsigned long)base;

    for (acc = 0, any = 0;; c = *s++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'Z')
            c -= 'A' - 10;
        else if (c >= 'a' && c <= 'z')
            c -= 'a' - 10;
        else
            break;

        if (c >= base)
            break;

        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
        } else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }

    if (any < 0) {
        acc = neg ? -9223372036854775808ULL : 9223372036854775807ULL;
        errno = ERANGE;
    } else if (neg) {
        acc = -acc;
    }

    if (endptr != 0) {
        *endptr = (char *)(any ? s - 1 : nptr);
    }

    return (long)acc;
}
