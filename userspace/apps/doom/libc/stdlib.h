#pragma once
#include <stddef.h>
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);
int atoi(const char* str);
void exit(int status);
int abs(int j);
int system(const char* command);
double atof(const char* str);
