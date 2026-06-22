#include "kernel/memory/heap/include/heap.h"
#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/display/display.h"

#define ALIGN_UP(val, align) (((val) + (align) - 1) & ~((align) - 1))

// Fixed initial heap region (e.g. at 256MB)
#define HEAP_START_VADDR 0x10000000ULL
#define KERNEL_HEAP_INITIAL_SIZE (256 * 1024)

static uint64_t heap_current;
static uint64_t heap_end;

void heap_init(void) {
    void* active_pml4 = vmm_get_active_pml4();

    size_t pages = ALIGN_UP(KERNEL_HEAP_INITIAL_SIZE, 4096) / 4096;

    // Request initial pages from VMM
    for (size_t i = 0; i < pages; i++) {
        uint64_t vaddr = HEAP_START_VADDR + (i * 4096);
        void* frame = vmm_alloc_mapped_page(active_pml4, vaddr, PAGE_WRITABLE | PAGE_USER);
        if (!frame) {
            display_print("[HEAP] PANIC: Failed to allocate initial heap pages!\n");
            while (1) { __asm__ volatile("hlt"); }
        }
    }

    heap_current = HEAP_START_VADDR;
    heap_end = HEAP_START_VADDR + (pages * 4096);

    display_print("\n[HEAP] Init OK\n");
    display_print("Heap Start: "); display_print_hex(heap_current); display_print("\n");
    display_print("Heap End:   "); display_print_hex(heap_end); display_print("\n\n");
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Align size to 8 bytes to ensure safely aligned pointers
    size_t aligned_size = (size + 7) & ~7ULL;

    if (heap_current + aligned_size > heap_end) {
        display_print("[HEAP] PANIC: Out of Memory in simple allocator!\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    void* allocated_ptr = (void*)heap_current;
    heap_current += aligned_size;

    return allocated_ptr;
}

void kfree(void* ptr) {
    (void)ptr; // Bump allocator doesn't support free
}
