#pragma once
#include <stdarg.h>
#include <stddef.h>
typedef void FILE;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
FILE* fopen(const char* path, const char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
int printf(const char* format, ...);
int puts(const char* s);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);
int sscanf(const char* s, const char* format, ...);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);
int fprintf(FILE* stream, const char* format, ...);
int remove(const char* filename);
#define stdin ((FILE*)0)
#define stdout ((FILE*)1)
#define stderr ((FILE*)2)
int rename(const char* old_filename, const char* new_filename);
int fflush(FILE* stream);
int putchar(int c);
int vfprintf(FILE* stream, const char* format, va_list ap);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
