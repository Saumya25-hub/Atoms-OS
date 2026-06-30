#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include <stddef.h>

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
size_t strlen(const char* s);
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

#endif // KERNEL_STRING_H
