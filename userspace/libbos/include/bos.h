#ifndef BOS_H
#define BOS_H

#include <stdint.h>
#include <stddef.h>

// Core System Calls
void bos_print(const char* str);
void bos_exit(void);
void bos_yield(void);
char bos_getc(void);

// File System Calls
int bos_open(const char* path);
int bos_read(int fd, void* buffer, size_t size);
int bos_close(int fd);

#endif
