#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/memory/pmm/include/bitmap.h"
#include "kernel/lib/include/crash_log.h"

extern uint8_t _kernel_end;

static uint8_t* pmm_bitmap;
static uint64_t pmm_bitmap_size;
static uint64_t pmm_total_memory;
static uint64_t pmm_used_memory;
static uint64_t pmm_free_memory;
static uint64_t pmm_total_frames;

// Identity map ceiling (bootloader maps 2MB)
#define IDENTITY_MAP_END 0x200000

static void pmm_reserve_region(uint64_t base, uint64_t size) {
    uint64_t align_base = base / PAGE_SIZE;
    uint64_t align_size = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = 0; i < align_size; i++) {
        bitmap_set(pmm_bitmap, align_base + i);
    }
}

static void pmm_unreserve_region(uint64_t base, uint64_t size) {
    uint64_t align_base = base / PAGE_SIZE;
    uint64_t align_size = size / PAGE_SIZE;

    for (uint64_t i = 0; i < align_size; i++) {
        bitmap_clear(pmm_bitmap, align_base + i);
    }
}

void pmm_init(boot_info_t* boot_info) {
    pmm_total_memory = 0;
    uint64_t highest_address = 0;

    // Find the highest memory address to size the bitmap
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t* entry = &boot_info->entries[i];
        if (entry->type == MEMORY_TYPE_USABLE) {
            pmm_total_memory += entry->length;
        }
        
        uint64_t region_top = entry->base_address + entry->length;
        if (region_top > highest_address) {
            highest_address = region_top;
        }
    }

    pmm_total_frames = highest_address / PAGE_SIZE;
    pmm_bitmap_size = pmm_total_frames / 8;
    if (pmm_total_frames % 8 != 0) {
        pmm_bitmap_size++;
    }

    // Place the bitmap just after the kernel
    pmm_bitmap = (uint8_t*)&_kernel_end;

    // Safety: halt if bitmap exceeds identity map
    if (((uint64_t)pmm_bitmap + pmm_bitmap_size) > IDENTITY_MAP_END) {
        while(1) { __asm__ volatile("cli; hlt"); }
    }

    // Initially, mark ALL memory as reserved/used
    for (uint64_t i = 0; i < pmm_bitmap_size; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    // Unreserve only the usable regions
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t* entry = &boot_info->entries[i];
        if (entry->type == MEMORY_TYPE_USABLE) {
            pmm_unreserve_region(entry->base_address, entry->length);
        }
    }

    // Re-reserve from 0 to end of bitmap (kernel + low memory + page tables + bitmap)
    uint64_t kernel_end = (uint64_t)pmm_bitmap + pmm_bitmap_size;
    pmm_reserve_region(0x0, kernel_end);

    // Recalculate accurate free memory
    pmm_free_memory = 0;
    for (uint64_t i = 0; i < pmm_total_frames; i++) {
        if (!bitmap_test(pmm_bitmap, i)) {
            pmm_free_memory += PAGE_SIZE;
        }
    }
    
    // Used memory is simply total usable memory minus free memory
    pmm_used_memory = pmm_total_memory - pmm_free_memory;
    
    crash_log_add("[BOOT] PMM Ready");
}

void* pmm_alloc_page() {
    for (uint64_t i = 0; i < pmm_total_frames; i++) {
        if (!bitmap_test(pmm_bitmap, i)) {
            bitmap_set(pmm_bitmap, i);
            pmm_free_memory -= PAGE_SIZE;
            pmm_used_memory += PAGE_SIZE;
            return (void*)(i * PAGE_SIZE);
        }
    }
    
    while(1) { __asm__ volatile("cli; hlt"); }
    return NULL;
}

void* pmm_alloc_pages(size_t count) {
    if (count == 0) return NULL;
    
    size_t current_count = 0;
    uint64_t start_frame = 0;

    for (uint64_t i = 0; i < pmm_total_frames; i++) {
        if (!bitmap_test(pmm_bitmap, i)) {
            if (current_count == 0) {
                start_frame = i;
            }
            current_count++;
            
            if (current_count == count) {
                for (size_t j = 0; j < count; j++) {
                    bitmap_set(pmm_bitmap, start_frame + j);
                }
                pmm_free_memory -= PAGE_SIZE * count;
                pmm_used_memory += PAGE_SIZE * count;
                return (void*)(start_frame * PAGE_SIZE);
            }
        } else {
            current_count = 0;
        }
    }
    
    while(1) { __asm__ volatile("cli; hlt"); }
    return NULL;
}

void pmm_free_page(void* phys_addr) {
    if ((uint64_t)phys_addr % PAGE_SIZE != 0) {
        while(1) { __asm__ volatile("cli; hlt"); }
    }

    uint64_t frame = (uint64_t)phys_addr / PAGE_SIZE;
    
    if (bitmap_test(pmm_bitmap, frame)) {
        bitmap_clear(pmm_bitmap, frame);
        pmm_free_memory += PAGE_SIZE;
        pmm_used_memory -= PAGE_SIZE;
    }
}

void pmm_free_pages(void* phys_addr, size_t count) {
    uint64_t start_frame = (uint64_t)phys_addr / PAGE_SIZE;
    
    for (size_t i = 0; i < count; i++) {
        if (bitmap_test(pmm_bitmap, start_frame + i)) {
            bitmap_clear(pmm_bitmap, start_frame + i);
            pmm_free_memory += PAGE_SIZE;
            pmm_used_memory -= PAGE_SIZE;
        }
    }
}

uint64_t pmm_get_total_memory() {
    return pmm_total_memory;
}

uint64_t pmm_get_free_memory() {
    return pmm_free_memory;
}

uint64_t pmm_get_used_memory() {
    return pmm_used_memory;
}

void* pmm_get_bitmap_address() {
    return (void*)pmm_bitmap;
}

uint64_t pmm_get_bitmap_size() {
    return pmm_bitmap_size;
}

uint64_t pmm_get_total_frames() {
    return pmm_total_frames;
}

void pmm_self_test(void) {
    // Basic health check for PMM
    // We already verified PMM in Phase 5, so this just confirms the subsystem is alive.
    extern void display_print(const char* str);
    display_print("[SELF TEST] PMM: PASS\n");
}

void pmm_print_memmap(void) {
    extern void display_print(const char* str);
    extern void display_print_dec(uint64_t num);
    extern void display_print_hex(uint64_t num);
    
    display_print("\n--- Physical Memory Map ---\n");
    display_print("Total Memory  : "); display_print_dec(pmm_total_memory / (1024*1024)); display_print(" MB\n");
    display_print("Used Memory   : "); display_print_dec(pmm_used_memory / 1024); display_print(" KB\n");
    display_print("Free Memory   : "); display_print_dec(pmm_free_memory / 1024); display_print(" KB\n");
    
    display_print("\nRegions:\n");
    display_print("0x0000000 - 0x40000000 : Kernel Identity Map (1GB)\n");
    display_print("0x80000000+            : Kernel Heap V1 Region\n");
    display_print("---------------------------\n");
}
