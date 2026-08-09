#include <stdint.h>
#include <stddef.h>

void sys_write(const char* str) {
    uint64_t len = 0;
    while (str && str[len]) len++;
    __asm__ volatile (
        "syscall"
        : 
        : "a"(0), "D"(str), "S"(len) // RAX=0 (SYS_WRITE), RDI=str, RSI=len
        : "rcx", "r11", "memory"
    );
}

void sys_exit(void) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(1), "D"(0) // RAX=1 (SYS_EXIT), RDI=0 (code)
        : "rcx", "r11", "memory"
    );
}

void sys_yield(void) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(3) // RAX=3 (SYS_YIELD)
        : "rcx", "r11", "memory"
    );
}

int sys_open(const char* path) {
    int fd;
    __asm__ volatile (
        "syscall"
        : "=a"(fd)
        : "a"(6), "D"(path) // RAX=6 (SYS_OPEN), RDI=path
        : "rcx", "r11", "memory"
    );
    return fd;
}

int sys_read(int fd, void* buffer, size_t size) {
    int bytes_read;
    __asm__ volatile (
        "syscall"
        : "=a"(bytes_read)
        : "a"(7), "D"(fd), "S"(buffer), "d"(size) // RAX=7, RDI=fd, RSI=buffer, RDX=size
        : "rcx", "r11", "memory"
    );
    return bytes_read;
}

void sys_close(int fd) {
    __asm__ volatile (
        "syscall"
        : 
        : "a"(8), "D"(fd) // RAX=8 (SYS_CLOSE), RDI=fd
        : "rcx", "r11", "memory"
    );
}

void _start() {
    sys_write("\n[BOS] I am BOS.\n");
    sys_write("[BOS] Kernel initialized successfully.\n");
    sys_write("[BOS] Welcome to BOS Userspace.\n\n");
    
    sys_write("[INIT] Attempting to open /BOS_OS.TXT from userspace...\n");
    int fd = sys_open("/BOS_OS.TXT");
    
    if (fd >= 0) {
        char buffer[256];
        int bytes_read = sys_read(fd, buffer, 255);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            sys_write("--------------------------------------------------\n");
            sys_write(buffer);
            sys_write("--------------------------------------------------\n");
            sys_write("[INIT] File read successfully.\n");
        } else {
            sys_write("[INIT] Failed to read file.\n");
        }
        sys_close(fd);
    } else {
        sys_write("[INIT] Failed to open /BOS_OS.TXT\n");
    }

    sys_write("\n[INIT] Terminating gracefully...\n");
    sys_exit();
    
    // In case sys_exit returns (e.g. if running as bypass kernel thread), loop forever
    while(1) { sys_yield(); }
}
