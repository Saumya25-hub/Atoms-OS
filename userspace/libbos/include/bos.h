#ifndef BOS_H
#define BOS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char name[64];
    uint32_t size;
    uint8_t is_directory;
} bos_dirent_t;

// Core System Calls
void bos_print(const char* str);
void bos_exit(void);
void bos_yield(void);
char bos_getc(void);

// Process System Calls
uint64_t bos_spawn(const char* path);

// File System Calls
int bos_open(const char* path);
int bos_read(int fd, void* buffer, size_t size);
int bos_close(int fd);
int bos_readdir(const char* path, int index, bos_dirent_t* out_entry);

#endif
