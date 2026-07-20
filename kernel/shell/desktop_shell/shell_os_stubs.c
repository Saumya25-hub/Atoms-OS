#include <stdint.h>
#include <stddef.h>

// Forward declarations from kernel systems
extern void display_print(const char* str);
extern void display_print_dec(uint64_t num);
extern void terminal_add_line(void* terminal_ctx, const char* str);

// Stubs for userspace libbos functions called by shared shell files
// When compiling for the kernel, we map these to kernel equivalents.

void bos_print(const char* str) {
    display_print(str);
}

void bos_clear_screen(void) {
    // The kernel shell stubs don't need to clear the whole screen
}

void bos_set_cursor(uint16_t x, uint16_t y) {
    (void)x;
    (void)y;
}

uint64_t bos_spawn(const char* path) {
    (void)path;
    return (uint64_t)-1;
}

// Optional VFS wrappers for editing/diagnostic commands 
// (For this phase, commands using VFS will just return failure in kernel GUI)
int bos_open(const char* path) { (void)path; return -1; }
int bos_read(int fd, void* buffer, size_t size) { (void)fd; (void)buffer; (void)size; return -1; }
int bos_write(int fd, const void* buffer, size_t size) { (void)fd; (void)buffer; (void)size; return -1; }
int bos_close(int fd) { (void)fd; return -1; }
int bos_seek(int fd, uint64_t offset, int whence) { (void)fd; (void)offset; (void)whence; return -1; }
int bos_readdir(const char* path, int index, void* out_entry) { (void)path; (void)index; (void)out_entry; return -1; }

void bos_heapinfo(void) {}
void bos_ps(void) {}
void bos_heapdump(void) {}
void bos_memmap(void) {}
void bos_dmesg(void) {}
void bos_taskinfo(int pid) { (void)pid; }
void bos_stressheap(void) {}
void bos_heapvalidate(void) {}
void bos_heapwalk(void) {}
void bos_heaptrace_toggle(void) {}
