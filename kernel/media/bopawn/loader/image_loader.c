#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/heap/include/heap.h"
#include <stdint.h>
#include <stddef.h>

uint8_t* image_loader_read(const char* path, uint32_t* out_size) {
    if (!path || !out_size) return NULL;
    
    int fd = vfs_open(path);
    if (fd < 0) return NULL;
    
    // Hacky file size fetch if stat is not exposed:
    // We assume file size is at most a few MB for icons/wallpapers.
    // In a real VFS, we'd use `stat`. For now, we allocate a large buffer, read, and realloc.
    // However, VFS might have `vfs_get_size`? We'll just read chunk by chunk.
    
    uint32_t capacity = 1024 * 1024; // Start with 1MB
    uint8_t* buffer = (uint8_t*)kmalloc(capacity);
    if (!buffer) {
        vfs_close(fd);
        return NULL;
    }
    
    uint32_t total_read = 0;
    while (1) {
        if (total_read >= capacity) {
            // Expand buffer
            uint32_t new_cap = capacity * 2;
            uint8_t* new_buf = (uint8_t*)kmalloc(new_cap);
            if (!new_buf) break;
            
            // memcpy
            extern void* memcpy(void*, const void*, size_t);
            memcpy(new_buf, buffer, total_read);
            kfree(buffer);
            buffer = new_buf;
            capacity = new_cap;
        }
        
        int bytes_read = vfs_read(fd, buffer + total_read, capacity - total_read);
        if (bytes_read <= 0) break;
        total_read += bytes_read;
    }
    
    vfs_close(fd);
    *out_size = total_read;
    return buffer;
}

void image_loader_free(uint8_t* buffer) {
    if (buffer) kfree(buffer);
}
