#include "xhci.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"

// Allocates contiguous physical pages and maps them as uncacheable.
// Returns the virtual address and outputs the physical address in `phys_out`.
void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name) {
    // Round size up to nearest page
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    void* phys = pmm_alloc_pages(num_pages);
    if (!phys) return NULL;

    if (phys_out) {
        *phys_out = (uint64_t)phys;
    }

    void* pml4 = vmm_get_active_pml4();
    // We identity map it for DMA, or if there is a higher-half mapping, we map it there.
    // Looking at xhci.c, it seems they identity mapped MMIO. We'll do the same.
    // Map with PAGE_CACHE_DISABLE so hardware reads fresh data.
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);

    for (size_t i = 0; i < num_pages; i++) {
        uint64_t page_addr = (uint64_t)phys + i * PAGE_SIZE;
        if (!name || strcmp(name, "ScratchPage") != 0) {
            display_print("[XHCI DMA] NAME = "); display_print(name ? name : "UNKNOWN"); display_print("\n");
            display_print("[XHCI DMA] PHYS = "); display_print_hex(page_addr); display_print("\n");
            display_print("[XHCI DMA] VIRT = "); display_print_hex(page_addr); display_print("\n");
            display_print("[XHCI DMA] SIZE = "); display_print_dec(size); display_print("\n");
            display_print("[XHCI DMA] PAGE = "); display_print_dec(i); display_print("\n");
            display_print("[XHCI DMA] FLAGS = PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE\n");
        }

        // The first 1GB of physical memory is already identity mapped using HUGE pages by the kernel bootloader.
        // vmm_get_pt_entry() does not support splitting HUGE pages and returns NULL, causing vmm_map_page() to PANIC.
        // Since x86 guarantees cache coherency for PCI DMA, we can safely use the existing cacheable identity mapping.
        // We only need to map if it's outside the 1GB identity map (e.g. MMIO > 4GB).
        if (page_addr >= 0x40000000ULL) {
            vmm_map_page(pml4, page_addr, page_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }

    memset(phys, 0, num_pages * PAGE_SIZE);
    return phys;
}
