#ifndef SIGNATURES_USB_DMA_H
#define SIGNATURES_USB_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void*    virt_addr;
    uint64_t phys_addr;
    size_t   size;
    size_t   alignment;
    bool     is_bounce;
    const char* name;
} usb_dma_buffer_t;

// DMA Engine APIs
void* usb_dma_alloc(size_t size, size_t alignment, uint64_t* phys_out, const char* name);
void  usb_dma_free(void* virt_addr, size_t size);
bool  usb_dma_validate_alignment(uint64_t phys_addr, size_t alignment);
void  usb_dma_cache_flush(void* virt_addr, size_t size);
void  usb_dma_cache_invalidate(void* virt_addr, size_t size);

// Bounce Buffer APIs
void* usb_dma_create_bounce_buffer(const void* src, size_t len, uint64_t* phys_out);
void  usb_dma_free_bounce_buffer(void* bounce_virt, void* dst, size_t len, bool copy_back);

#endif // SIGNATURES_USB_DMA_H
