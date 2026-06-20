#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/memory/pmm/include/bitmap.h"
#include "kernel/display/display.h"

extern uint8_t _kernel_end;

static uint8_t* pmm_bitmap;
static uint64_t pmm_bitmap_size;
static uint64_t pmm_total_memory;
static uint64_t pmm_used_memory;
static uint64_t pmm_free_memory;
static uint64_t pmm_total_frames;

// Identity map ceiling (bootloader maps 2MB)
#define IDENTITY_MAP_END 0x200000

static void pmm_verify_write(uint64_t start, uint64_t size) {
    if (start + size > IDENTITY_MAP_END) {
        display_print("FATAL: Write outside identity map!\n");
        display_print("  Start: "); display_print_hex(start); display_print("\n");
        display_print("  End:   "); display_print_hex(start + size); display_print("\n");
        display_print("  Limit: "); display_print_hex(IDENTITY_MAP_END); display_print("\n");
        while(1) { __asm__ volatile("hlt"); }
    }
}

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
    display_print("PMM A\n");

    // Validate boot_info pointer
    display_print("  bi_ptr: "); display_print_hex((uint64_t)boot_info); display_print("\n");
    pmm_verify_write((uint64_t)boot_info, sizeof(boot_info_t));

    pmm_total_memory = 0;
    uint64_t highest_address = 0;

    // Find the highest memory address to size the bitmap
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t* entry = &boot_info->entries[i];
        
        // Validate entry pointer
        pmm_verify_write((uint64_t)entry, sizeof(memory_map_entry_t));

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

    display_print("PMM B\n");

    // Place the bitmap just after the kernel
    pmm_bitmap = (uint8_t*)&_kernel_end;

    display_print("  KEnd:  "); display_print_hex((uint64_t)&_kernel_end); display_print("\n");
    display_print("  Bmp:   "); display_print_hex((uint64_t)pmm_bitmap); display_print("\n");
    display_print("  BmpSz: "); display_print_dec(pmm_bitmap_size); display_print("\n");
    display_print("  BmpEnd:"); display_print_hex((uint64_t)pmm_bitmap + pmm_bitmap_size); display_print("\n");
    display_print("  High:  "); display_print_hex(highest_address); display_print("\n");
    display_print("  Frames:"); display_print_dec(pmm_total_frames); display_print("\n");

    // VERIFY: Bitmap write range within identity map
    pmm_verify_write((uint64_t)pmm_bitmap, pmm_bitmap_size);

    display_print("PMM C\n");

    // Initially, mark ALL memory as reserved/used
    // This writes pmm_bitmap_size bytes starting at pmm_bitmap
    for (uint64_t i = 0; i < pmm_bitmap_size; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    display_print("PMM D\n");

    // Unreserve only the usable regions
    // bitmap_clear only writes within pmm_bitmap[0..pmm_bitmap_size-1]
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t* entry = &boot_info->entries[i];
        if (entry->type == MEMORY_TYPE_USABLE) {
            pmm_unreserve_region(entry->base_address, entry->length);
        }
    }

    display_print("PMM E\n");

    // Re-reserve memory used by the kernel and the PMM bitmap itself
    uint64_t kernel_start = 0x0;
    uint64_t kernel_end = (uint64_t)pmm_bitmap + pmm_bitmap_size;
    pmm_reserve_region(kernel_start, kernel_end - kernel_start);

    display_print("PMM F\n");

    // Verify bootloader page table frames are reserved
    // PML4=0x10000(frame 16) PDP=0x11000(17) PD=0x12000(18) PT=0x13000(19)
    display_print("  PT: ");
    display_print(bitmap_test(pmm_bitmap, 16) ? "R" : "F");
    display_print(bitmap_test(pmm_bitmap, 17) ? "R" : "F");
    display_print(bitmap_test(pmm_bitmap, 18) ? "R" : "F");
    display_print(bitmap_test(pmm_bitmap, 19) ? "R" : "F");
    display_print("\n");

    pmm_used_memory = 0;
    pmm_free_memory = 0;

    // Recalculate accurate free/used memory after reserves
    for (uint64_t i = 0; i < pmm_total_frames; i++) {
        if (bitmap_test(pmm_bitmap, i)) {
            pmm_used_memory += PAGE_SIZE;
        } else {
            pmm_free_memory += PAGE_SIZE;
        }
    }
    display_print("PMM G\n");
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
    
    while(1) { __asm__ volatile("hlt"); }
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
    
    while(1) { __asm__ volatile("hlt"); }
    return NULL;
}

void pmm_free_page(void* phys_addr) {
    if ((uint64_t)phys_addr % PAGE_SIZE != 0) {
        while(1) { __asm__ volatile("hlt"); }
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
