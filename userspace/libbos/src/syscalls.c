#include "../include/bos.h"

#define SYS_YIELD  0
#define SYS_WRITE  1
#define SYS_SLEEP  2
#define SYS_UPTIME 3
#define SYS_GETPID 4
#define SYS_EXIT   5
#define SYS_OPEN   6
#define SYS_READ   7
#define SYS_CLOSE  8
#define SYS_GETC   9

void bos_exit(void) {
    __asm__ volatile("mov $5, %%rax; syscall" : : : "rax", "rcx", "r11", "memory");
}

void bos_yield(void) {
    __asm__ volatile("mov $0, %%rax; syscall" : : : "rax", "rcx", "r11", "memory");
}

void bos_print(const char* str) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(SYS_WRITE), "D"(str)
        : "rcx", "r11", "memory"
    );
}

int bos_open(const char* path) {
    int fd;
    __asm__ volatile (
        "syscall"
        : "=a"(fd)
        : "a"(SYS_OPEN), "D"(path)
        : "rcx", "r11", "memory"
    );
    return fd;
}

int bos_read(int fd, void* buffer, size_t size) {
    int bytes_read;
    __asm__ volatile (
        "syscall"
        : "=a"(bytes_read)
        : "a"(SYS_READ), "D"(fd), "S"(buffer), "d"(size)
        : "rcx", "r11", "memory"
    );
    return bytes_read;
}

int bos_close(int fd) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_CLOSE), "D"(fd)
        : "rcx", "r11", "memory"
    );
    return res;
}

char bos_getc(void) {
    char c;
    __asm__ volatile (
        "syscall"
        : "=a"(c)
        : "a"(SYS_GETC)
        : "rcx", "r11", "memory"
    );
    return c;
}
