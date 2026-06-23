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

static heap_block_t* heap_head = NULL;

void heap_init(void) {
    void* active_pml4 = vmm_get_active_pml4();

    size_t pages = ALIGN_UP(KERNEL_HEAP_INITIAL_SIZE, 4096) / 4096;

    // Request initial pages from VMM
    for (size_t i = 0; i < pages; i++) {
        uint64_t vaddr = HEAP_START_VADDR + (i * 4096);
        void* frame = vmm_alloc_mapped_page(active_pml4, vaddr, PAGE_WRITABLE | PAGE_USER);
        if (!frame) {
            display_print("[HEAP V1] PANIC: Failed to allocate initial heap pages!\n");
            while (1) { __asm__ volatile("hlt"); }
        }
    }

    heap_current = HEAP_START_VADDR;
    heap_end = HEAP_START_VADDR + (pages * 4096);

    // Initialize the first free block
    heap_head = (heap_block_t*)HEAP_START_VADDR;
    heap_head->magic = HEAP_MAGIC;
    heap_head->size = (heap_end - HEAP_START_VADDR) - sizeof(heap_block_t);
    heap_head->is_free = true;
    heap_head->next = NULL;
    heap_head->prev = NULL;

    display_print("\n[HEAP V1] Init OK\n");
    display_print("Heap Start: "); display_print_hex(heap_current); display_print("\n");
    display_print("Heap End:   "); display_print_hex(heap_end); display_print("\n\n");
}

static void split_block(heap_block_t* block, size_t size) {
    // Check if remaining size is enough for a new block header + at least 8 bytes of data
    if (block->size > size + sizeof(heap_block_t) + 8) {
        heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->is_free = true;
        
        new_block->next = block->next;
        new_block->prev = block;
        
        if (new_block->next) {
            new_block->next->prev = new_block;
        }
        
        block->next = new_block;
        block->size = size;
    }
}

static void coalesce_block(heap_block_t* block) {
    if (!block || !block->is_free) return;

    // Merge with next if free
    if (block->next && block->next->is_free && block->next->magic == HEAP_MAGIC) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Merge with prev if free
    if (block->prev && block->prev->is_free && block->prev->magic == HEAP_MAGIC) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Align size to 8 bytes to ensure safely aligned pointers
    size_t aligned_size = (size + 7) & ~7ULL;

    heap_block_t* current = heap_head;
    while (current) {
        if (current->magic != HEAP_MAGIC) {
            display_print("[HEAP V1] PANIC: Heap corruption detected during kmalloc!\n");
            while (1) { __asm__ volatile("hlt"); }
        }

        if (current->is_free && current->size >= aligned_size) {
            // Found a suitable block
            split_block(current, aligned_size);
            current->is_free = false;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }

    // Out of memory (no VMM expansion implemented yet)
    display_print("[HEAP V1] PANIC: Out of Memory! Failed to allocate ");
    display_print_dec(size);
    display_print(" bytes.\n");
    while (1) { __asm__ volatile("hlt"); }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) {
        display_print("[HEAP V1] PANIC: Corrupted heap block passed to kfree!\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    if (block->is_free) {
        display_print("[HEAP V1] PANIC: Double free detected!\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    block->is_free = true;
    coalesce_block(block);
}

void* kcalloc(size_t num, size_t size) {
    void* ptr = kmalloc(num * size);
    if (ptr) {
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < num * size; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (block->size >= new_size) {
        return ptr; // In-place optimization
    }

    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        uint8_t* src = (uint8_t*)ptr;
        uint8_t* dst = (uint8_t*)new_ptr;
        for (size_t i = 0; i < block->size; i++) {
            dst[i] = src[i];
        }
        kfree(ptr);
    }
    return new_ptr;
}

void heap_get_stats(HeapStats* stats) {
    if (!stats) return;

    stats->total_size = (uint32_t)(heap_end - HEAP_START_VADDR);
    stats->used_size = 0;
    stats->free_size = 0;
    stats->block_count = 0;
    stats->largest_free = 0;

    heap_block_t* current = heap_head;
    while (current) {
        stats->block_count++;
        if (current->is_free) {
            stats->free_size += current->size;
            if (current->size > stats->largest_free) {
                stats->largest_free = current->size;
            }
        } else {
            stats->used_size += current->size;
        }
        current = current->next;
    }
}
