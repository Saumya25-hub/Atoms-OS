#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/pmm/include/bitmap.h"
#include "kernel/debug/abde/abde.h"

extern void com1_puts(const char *s);
extern uint8_t _kernel_end;

static uint8_t  *g_pmm_bitmap = 0;
static uint64_t g_pmm_bitmap_size = 0;
static uint64_t g_pmm_total_memory = 0;
static uint64_t g_pmm_usable_memory = 0;
static uint64_t g_pmm_reserved_memory = 0;
static uint64_t g_pmm_free_memory = 0;
static uint64_t g_pmm_total_frames = 0;
static uint64_t g_pmm_last_alloc = 0;
static uint64_t g_pmm_last_free = 0;

static uint64_t g_pmm_free_pages = 0;
static uint64_t g_pmm_used_pages = 0;
static uint64_t g_pmm_reserved_pages = 0;

/* Reserve a physical memory region in the bitmap */
static void pmm_reserve_region(uint64_t base, uint64_t size) {
    uint64_t align_base = base / PAGE_SIZE;
    uint64_t align_size = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = 0; i < align_size && (align_base + i) < g_pmm_total_frames; i++) {
        if (!bitmap_test(g_pmm_bitmap, align_base + i)) {
            bitmap_set(g_pmm_bitmap, align_base + i);
            g_pmm_reserved_pages++;
        }
    }
}

/* Unreserve usable RAM regions in the bitmap */
static void pmm_unreserve_region(uint64_t base, uint64_t size) {
    uint64_t align_base = base / PAGE_SIZE;
    uint64_t align_size = size / PAGE_SIZE;

    for (uint64_t i = 0; i < align_size && (align_base + i) < g_pmm_total_frames; i++) {
        if (bitmap_test(g_pmm_bitmap, align_base + i)) {
            bitmap_clear(g_pmm_bitmap, align_base + i);
            if (g_pmm_reserved_pages > 0) g_pmm_reserved_pages--;
        }
    }
}

void pmm_init(boot_info_t *boot_info) {
    com1_puts("[PMM] PMM_INIT ENTERED\n");
    g_abde.pmm_active = true;
    diag_set_running("PMM");
    diag_set_step("PMM UEFI MAP SCAN");

    if (!boot_info) {
        com1_puts("[PMM] NULL BOOT INFO!\n");
        diag_panic_reason("PMM", "PARSE_BOOT_INFO", "NULL_BOOT_INFO", "boot_info structure is NULL");
        return;
    }

    g_pmm_total_memory = 0;
    g_pmm_usable_memory = 0;
    g_pmm_reserved_memory = 0;
    uint64_t highest_address = 0;

    // 1. Enumerate UEFI Memory Map Entries
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t *entry = &boot_info->entries[i];
        g_pmm_total_memory += entry->length;

        if (entry->type == MEMORY_TYPE_USABLE) {
            g_pmm_usable_memory += entry->length;
            uint64_t top = entry->base_address + entry->length;
            if (top > highest_address) {
                highest_address = top;
            }
        } else {
            g_pmm_reserved_memory += entry->length;
        }
    }

    // Cap highest_address to physical RAM range (max 32GB) to prevent bogus UEFI MMIO regions from inflating bitmap
    if (highest_address > 0x800000000ULL) {
        highest_address = 0x800000000ULL;
    }
    g_pmm_total_frames = highest_address / PAGE_SIZE;
    g_pmm_bitmap_size = (g_pmm_total_frames + 7) / 8;

    // 2. Dynamic Bitmap Allocation: Place bitmap immediately after kernel image
    uint64_t kernel_end_addr = (uint64_t)&_kernel_end;
    uint64_t bitmap_addr = (kernel_end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    g_pmm_bitmap = (uint8_t*)bitmap_addr;

    diag_set_step("PMM INIT BITMAP");

    // Initially mark ALL memory as reserved/used
    for (uint64_t i = 0; i < g_pmm_bitmap_size; i++) {
        g_pmm_bitmap[i] = 0xFF;
    }
    g_pmm_reserved_pages = g_pmm_total_frames;

    // Unreserve usable memory regions
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t *entry = &boot_info->entries[i];
        if (entry->type == MEMORY_TYPE_USABLE) {
            pmm_unreserve_region(entry->base_address, entry->length);
        }
    }

    // 3. Re-reserve Critical Kernel & Hardware Regions
    pmm_reserve_region(0x0, 0x200000);
    uint64_t kernel_reserve_end = bitmap_addr + g_pmm_bitmap_size;
    pmm_reserve_region(0x100000, kernel_reserve_end - 0x100000);

    if (boot_info->vbe_framebuffer > 0) {
        uint64_t fb_size = (uint64_t)boot_info->vbe_pitch * boot_info->vbe_height;
        pmm_reserve_region(boot_info->vbe_framebuffer, fb_size);
    }

    // Recalculate accurate free/used counts
    g_pmm_free_pages = 0;
    g_pmm_used_pages = 0;
    for (uint64_t i = 0; i < g_pmm_total_frames; i++) {
        if (!bitmap_test(g_pmm_bitmap, i)) {
            g_pmm_free_pages++;
        } else {
            g_pmm_used_pages++;
        }
    }

    g_pmm_free_memory = g_pmm_free_pages * PAGE_SIZE;

    uint64_t total_mb    = g_pmm_total_memory / (1024 * 1024);
    uint64_t usable_mb   = g_pmm_usable_memory / (1024 * 1024);
    uint64_t reserved_mb = g_pmm_reserved_memory / (1024 * 1024);

    diag_set_pmm_telemetry(total_mb, usable_mb, reserved_mb, g_pmm_free_pages, g_pmm_used_pages, g_pmm_reserved_pages, 0, 0);

    // 4. PHASE 5: PMM STRESS & VERIFICATION TEST SUITE
    com1_puts("[PMM] RUNNING STRESS TESTS\n");
    diag_set_step("PMM STRESS TEST A (1 PAGE)");
    void *p1 = pmm_alloc_page();
    if (!p1 || ((uint64_t)p1 % PAGE_SIZE) != 0) {
        diag_panic_reason("PMM", "STRESS_TEST_A", "ALLOC_FAIL_1P", "Single page allocation failed or unaligned");
        return;
    }

    diag_set_step("PMM STRESS TEST B (10 PAGES)");
    void *p10 = pmm_alloc_pages(10);
    if (!p10 || ((uint64_t)p10 % PAGE_SIZE) != 0) {
        diag_panic_reason("PMM", "STRESS_TEST_B", "ALLOC_FAIL_10P", "10-page allocation failed or unaligned");
        return;
    }

    diag_set_step("PMM STRESS TEST C (100 PAGES)");
    void *p100 = pmm_alloc_pages(100);
    if (!p100 || ((uint64_t)p100 % PAGE_SIZE) != 0) {
        diag_panic_reason("PMM", "STRESS_TEST_C", "ALLOC_FAIL_100P", "100-page allocation failed or unaligned");
        return;
    }

    diag_set_step("PMM STRESS TEST D (1000 PAGES)");
    void *p1000 = pmm_alloc_pages(1000);
    if (!p1000 || ((uint64_t)p1000 % PAGE_SIZE) != 0) {
        diag_panic_reason("PMM", "STRESS_TEST_D", "ALLOC_FAIL_1000P", "1000-page allocation failed or unaligned");
        return;
    }

    diag_set_step("PMM STRESS FREE ALL");
    pmm_free_page(p1);
    pmm_free_pages(p10, 10);
    pmm_free_pages(p100, 100);
    pmm_free_pages(p1000, 1000);

    diag_set_pmm_telemetry(total_mb, usable_mb, reserved_mb, g_pmm_free_pages, g_pmm_used_pages, g_pmm_reserved_pages, (uint64_t)p1000, (uint64_t)p1000);
    diag_set_step("PMM STRESS CERTIFIED");
    com1_puts("[PMM] PMM_INIT COMPLETE\n");
}

/* Allocate Single 4KB Physical Page */
void* pmm_alloc_page(void) {
    for (uint64_t i = 0; i < g_pmm_total_frames; i++) {
        if (!bitmap_test(g_pmm_bitmap, i)) {
            bitmap_set(g_pmm_bitmap, i);
            g_pmm_free_pages--;
            g_pmm_used_pages++;
            g_pmm_free_memory -= PAGE_SIZE;
            
            uint64_t phys_addr = i * PAGE_SIZE;
            g_pmm_last_alloc = phys_addr;
            return (void*)phys_addr;
        }
    }

    diag_panic_reason("PMM", "ALLOC_PAGE", "OUT_OF_MEMORY", "All physical memory exhausted");
    return 0;
}

/* Allocate Contiguous 4KB Physical Pages */
void* pmm_alloc_pages(size_t count) {
    if (count == 0) return 0;
    size_t current_count = 0;
    uint64_t start_frame = 0;

    for (uint64_t i = 0; i < g_pmm_total_frames; i++) {
        if (!bitmap_test(g_pmm_bitmap, i)) {
            if (current_count == 0) {
                start_frame = i;
            }
            current_count++;
            if (current_count == count) {
                for (size_t j = 0; j < count; j++) {
                    bitmap_set(g_pmm_bitmap, start_frame + j);
                }
                g_pmm_free_pages -= count;
                g_pmm_used_pages += count;
                g_pmm_free_memory -= count * PAGE_SIZE;

                uint64_t phys_addr = start_frame * PAGE_SIZE;
                g_pmm_last_alloc = phys_addr;
                return (void*)phys_addr;
            }
        } else {
            current_count = 0;
        }
    }

    diag_panic_reason("PMM", "ALLOC_PAGES", "OUT_OF_MEMORY", "Contiguous physical pages exhausted");
    return 0;
}

/* Free Single 4KB Physical Page */
void pmm_free_page(void *phys_addr) {
    if (!phys_addr || ((uint64_t)phys_addr % PAGE_SIZE) != 0) {
        diag_panic_reason("PMM", "FREE_PAGE", "INVALID_ALIGNMENT", "Physical address is not 4KB aligned");
        return;
    }

    uint64_t frame = (uint64_t)phys_addr / PAGE_SIZE;
    if (frame >= g_pmm_total_frames) {
        diag_panic_reason("PMM", "FREE_PAGE", "OUT_OF_BOUNDS", "Physical address exceeds physical RAM limits");
        return;
    }

    if (!bitmap_test(g_pmm_bitmap, frame)) {
        diag_panic_reason("PMM", "FREE_PAGE", "DOUBLE_FREE", "Freeing already free physical page");
        return;
    }

    bitmap_clear(g_pmm_bitmap, frame);
    g_pmm_free_pages++;
    if (g_pmm_used_pages > 0) g_pmm_used_pages--;
    g_pmm_free_memory += PAGE_SIZE;
    g_pmm_last_free = (uint64_t)phys_addr;
}

/* Free Contiguous 4KB Physical Pages */
void pmm_free_pages(void *phys_addr, size_t count) {
    if (!phys_addr || count == 0 || ((uint64_t)phys_addr % PAGE_SIZE) != 0) {
        diag_panic_reason("PMM", "FREE_PAGES", "INVALID_ARGUMENT", "Invalid address or count for free");
        return;
    }

    uint64_t start_frame = (uint64_t)phys_addr / PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        uint64_t frame = start_frame + i;
        if (frame < g_pmm_total_frames && bitmap_test(g_pmm_bitmap, frame)) {
            bitmap_clear(g_pmm_bitmap, frame);
            g_pmm_free_pages++;
            if (g_pmm_used_pages > 0) g_pmm_used_pages--;
            g_pmm_free_memory += PAGE_SIZE;
        }
    }
    g_pmm_last_free = (uint64_t)phys_addr;
}

uint64_t pmm_get_total_memory(void) { return g_pmm_total_memory; }
uint64_t pmm_get_free_memory(void)  { return g_pmm_free_memory; }
uint64_t pmm_get_used_memory(void)  { return g_pmm_usable_memory - g_pmm_free_memory; }
void* pmm_get_bitmap_address(void)   { return (void*)g_pmm_bitmap; }
uint64_t pmm_get_bitmap_size(void)   { return g_pmm_bitmap_size; }
uint64_t pmm_get_total_frames(void)  { return g_pmm_total_frames; }

void pmm_self_test(void) {}
void pmm_print_memmap(void) {}
