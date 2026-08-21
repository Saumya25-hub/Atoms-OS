#include "../include/bos.h"

#define SYS_WRITE  0
#define SYS_EXIT   1
#define SYS_GETPID 2
#define SYS_YIELD  3
#define SYS_UPTIME 4
#define SYS_OPEN   6
#define SYS_READ   7
#define SYS_CLOSE  8
#define SYS_GETC   9
#define SYS_SPAWN   10
#define SYS_READDIR 11
#define SYS_PS              12
#define SYS_GET_KEY_EVENT   13
#define SYS_SURFACE_PRESENT 41
#define SYS_GET_INPUT_EVENT 42
#define SYS_GL_INIT_CONTEXT    45
#define SYS_GL_PRESENT_FRAME   46
#define SYS_GL_DESTROY_CONTEXT 47
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
#define SYS_CLEAR_SCREEN    28
#define SYS_SET_CURSOR      29
#define SYS_SEEK            40
#define SYS_SURFACE_PRESENT 41

void bos_exit(void) {
    __asm__ volatile("mov $1, %%rax; syscall" : : : "rax", "rcx", "r11", "memory");
}

void bos_yield(void) {
    __asm__ volatile("mov $3, %%rax; syscall" : : : "rax", "rcx", "r11", "memory");
}

uint32_t bos_uptime(void) {
    uint32_t ms;
    __asm__ volatile (
        "syscall"
        : "=a"(ms)
        : "a"(SYS_UPTIME)
        : "rcx", "r11", "memory"
    );
    return ms;
}

void bos_print(const char* str) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(SYS_WRITE), "D"(str)
        : "rcx", "r11", "memory"
    );
}

void bos_clear_screen(void) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(SYS_CLEAR_SCREEN)
        : "rcx", "r11", "memory"
    );
}

void bos_set_cursor(uint16_t x, uint16_t y) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(SYS_SET_CURSOR), "D"((uint64_t)x), "S"((uint64_t)y)
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

int bos_seek(int fd, uint64_t offset, int whence) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_SEEK), "D"(fd), "S"(offset), "d"((uint64_t)whence)
        : "rcx", "r11", "memory"
    );
    return res;
}

void bos_surface_present(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h) {
    uint32_t res;
    register uint64_t r10 asm("r10") = (uint64_t)h; // h goes into arg4 (R10)
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_SURFACE_PRESENT), "D"(window_id), "S"(pixels), "d"((uint64_t)w), "r"(r10)
        : "rcx", "r11", "memory", "r8"
    );
}

int bos_gl_init_context(uint32_t window_id) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_GL_INIT_CONTEXT), "D"((uint64_t)window_id)
        : "rcx", "r11", "memory"
    );
    return res;
}

int bos_gl_present_frame(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h) {
    int res;
    register uint64_t r10 asm("r10") = (uint64_t)h;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_GL_PRESENT_FRAME), "D"((uint64_t)window_id), "S"(pixels), "d"((uint64_t)w), "r"(r10)
        : "rcx", "r11", "memory", "r8"
    );
    return res;
}

int bos_gl_destroy_context(uint32_t window_id) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_GL_DESTROY_CONTEXT), "D"((uint64_t)window_id)
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

int bos_get_input_event(void* event) {
    int res;
    __asm__ volatile (
        "syscall"
        : "=a"(res)
        : "a"(SYS_GET_INPUT_EVENT), "D"(event)
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
