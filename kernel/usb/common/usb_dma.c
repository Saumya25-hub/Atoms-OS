#include "usb_dma.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

void* usb_dma_alloc(size_t size, size_t alignment, uint64_t* phys_out, const char* name) {
    if (size == 0) return NULL;
    if (alignment < 16) alignment = 16;
    
    // Calculate required pages
    size_t pages = (size + 4095) / 4096;
    void* phys = pmm_alloc_pages(pages);
    if (!phys) {
        display_print("[USB DMA] Error: PMM Allocation failed for ");
        display_print(name ? name : "DMA");
        display_print("\n");
        return NULL;
    }
    
    uint64_t phys_addr = (uint64_t)phys;
    void* pml4 = vmm_get_active_pml4();
    
    // Map pages with cache disable for uncached DMA consistency
    for (size_t i = 0; i < pages; i++) {
        uint64_t p = phys_addr + (i * 4096);
        vmm_map_page(pml4, p, p, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }
    
    memset((void*)phys_addr, 0, size);
    
    if (phys_out) *phys_out = phys_addr;
    return (void*)phys_addr;
}

void usb_dma_free(void* virt_addr, size_t size) {
    if (!virt_addr || size == 0) return;
    size_t pages = (size + 4095) / 4096;
    pmm_free_pages(virt_addr, pages);
}

bool usb_dma_validate_alignment(uint64_t phys_addr, size_t alignment) {
    if (alignment == 0) return true;
    return (phys_addr % alignment) == 0;
}

void usb_dma_cache_flush(void* virt_addr, size_t size) {
    (void)virt_addr; (void)size;
    asm volatile ("mfence" ::: "memory");
}

void usb_dma_cache_invalidate(void* virt_addr, size_t size) {
    (void)virt_addr; (void)size;
    asm volatile ("mfence" ::: "memory");
}

void* usb_dma_create_bounce_buffer(const void* src, size_t len, uint64_t* phys_out) {
    if (len == 0) return NULL;
    void* bounce = usb_dma_alloc(len, 16, phys_out, "BounceBuf");
    if (bounce && src) {
        memcpy(bounce, src, len);
    }
    return bounce;
}

void usb_dma_free_bounce_buffer(void* bounce_virt, void* dst, size_t len, bool copy_back) {
    if (!bounce_virt) return;
    if (copy_back && dst && len > 0) {
        memcpy(dst, bounce_virt, len);
    }
    usb_dma_free(bounce_virt, len);
}
