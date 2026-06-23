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
#define SYS_SPAWN   10
#define SYS_READDIR 11
#define SYS_PS              12
#define SYS_GET_KEY_EVENT   13
#define SYS_GET_HEAP_STATS  14
#define SYS_HEAP_DUMP       15
#define SYS_MEMMAP          16
#define SYS_DMESG           17
#define SYS_TASK_INFO       18
#define SYS_STRESS_HEAP     19
#define SYS_HEAP_VALIDATE   20
#define SYS_HEAP_WALK       21
#define SYS_HEAP_TRACE_TOGGLE 22
#define SYS_WRITE_FILE      23
#define SYS_MKDIR           24
#define SYS_CREATE          25
#define SYS_RENAME          26
#define SYS_DELETE          27

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

uint64_t bos_spawn(const char* path) {
    uint64_t pid;
    __asm__ volatile (
        "syscall"
        : "=a"(pid)
        : "a"(SYS_SPAWN), "D"(path)
        : "rcx", "r11", "memory"
    );
    return pid;
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

int bos_write(int fd, const void* buffer, size_t size) {
    int bytes_written;
    __asm__ volatile (
        "syscall"
        : "=a"(bytes_written)
        : "a"(SYS_WRITE_FILE), "D"(fd), "S"(buffer), "d"(size)
        : "rcx", "r11", "memory"
    );
    return bytes_written;
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

int bos_readdir(const char* path, int index, bos_dirent_t* out_entry) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_READDIR), "D"(path), "S"(index), "d"(out_entry)
        : "rcx", "r11", "memory"
    );
    return res;
}

int bos_mkdir(const char* path) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_MKDIR), "D"(path)
        : "rcx", "r11", "memory"
    );
    return res;
}

int bos_create(const char* path) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_CREATE), "D"(path)
        : "rcx", "r11", "memory"
    );
    return res;
}

void bos_heapinfo(void) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(SYS_GET_HEAP_STATS)
        : "rcx", "r11", "memory"
    );
}

void bos_ps(void) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(SYS_PS)
        : "rcx", "r11", "memory"
    );
}

int bos_get_key_event(bos_key_event_t* event) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_GET_KEY_EVENT), "D"(event)
        : "rcx", "r11", "memory"
    );
    return res;
}

void bos_heapdump(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_HEAP_DUMP) : "rcx", "r11", "memory");
}

void bos_memmap(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_MEMMAP) : "rcx", "r11", "memory");
}

void bos_dmesg(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_DMESG) : "rcx", "r11", "memory");
}

void bos_taskinfo(int pid) {
    __asm__ volatile ("syscall" : : "a"(SYS_TASK_INFO), "D"((uint64_t)pid) : "rcx", "r11", "memory");
}

void bos_stressheap(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_STRESS_HEAP) : "rcx", "r11", "memory");
}

void bos_heapvalidate(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_HEAP_VALIDATE) : "rcx", "r11", "memory");
}

void bos_heapwalk(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_HEAP_WALK) : "rcx", "r11", "memory");
}

void bos_heaptrace_toggle(void) {
    __asm__ volatile ("syscall" : : "a"(SYS_HEAP_TRACE_TOGGLE) : "rcx", "r11", "memory");
}

int bos_rename(const char* old_path, const char* new_name) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_RENAME), "D"(old_path), "S"(new_name)
        : "rcx", "r11", "memory"
    );
    return res;
}

int bos_delete(const char* path) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_DELETE), "D"(path)
        : "rcx", "r11", "memory"
    );
    return res;
}
