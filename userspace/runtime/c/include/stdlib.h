/*
 * ATOMS OS — Userspace C Runtime stdlib.h
 * Adapted from musl libc (MIT License)
 */

#ifndef ATOMS_USER_STDLIB_H
#define ATOMS_USER_STDLIB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t size);
void  free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

void  exit(int status);
void  abort(void);

int   abs(int j);
long  labs(long j);
int   atoi(const char *nptr);
long  strtol(const char *nptr, char **endptr, int base);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_STDLIB_H */
