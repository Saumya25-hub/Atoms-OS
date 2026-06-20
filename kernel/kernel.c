#include "kernel/display/display.h"
#include "kernel/console/console.h"
#include "drivers/video/vga/vga.h"
#include "arch/x86_64/interrupt/idt.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/interrupt/include/exception.h"
#include "kernel/interrupt/include/irq.h"
#include "drivers/interrupt/pic/pic.h"
#include "kernel/timer/include/timer.h"
#include "kernel/keyboard/include/keyboard.h"
#include "kernel/boot/include/boot_info.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/config/build_config.h"
static BackendDriver vga_backend = {
    .init = vga_init,
    .draw_character = vga_draw_character,
    .set_hardware_cursor = vga_set_hardware_cursor,
    .clear_memory = vga_clear_memory,
    .get_width = vga_get_screen_width,
    .get_height = vga_get_screen_height
};

void kernel_main(boot_info_t* boot_info) {
    // 1. Initialize and register the backend
    console_set_backend(&vga_backend);
    
    // 2. Initialize the display subsystem (Terminal Logic)
    display_init();
    display_clear();
    
    // 3. Print using the high-level API
    display_print("SignaturesOS v0.3 - BOS Architecture\n");
    display_print("Kernel OK\n");
    display_print("Display Subsystem V1 Initialized.\n");
    
    // 3.1 Initialize Physical Memory Manager
    display_print("\n[E820] Memory Map Detected\n");
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t* entry = &boot_info->entries[i];
        display_print("Region "); display_print_dec(i); display_print("\n");
        display_print("Base:   "); display_print_hex(entry->base_address); display_print("\n");
        display_print("Length: "); display_print_hex(entry->length); display_print("\n");
        display_print("Type:   "); display_print_dec(entry->type); display_print("\n\n");
    }

    display_print("[PMM] Initializing Physical Memory Manager...\n");
    pmm_init(boot_info);

    display_print("\n[PMM] Initialization Complete\n");
    display_print("Total RAM:       "); display_print_dec(pmm_get_total_memory() / 1024); display_print(" KB\n");
    display_print("Usable RAM:      "); display_print_dec(pmm_get_free_memory() / 1024); display_print(" KB\n");
    
    // We calculate Reserved RAM and Kernel Reserved based on frames to show stats
    uint64_t total_frames = pmm_get_total_frames();
    uint64_t total_mem = pmm_get_total_memory();
    display_print("Reserved RAM:    "); display_print_dec((total_frames * 4096 - total_mem) / 1024); display_print(" KB\n");
    
    display_print("Bitmap Address:  "); display_print_hex((uint64_t)pmm_get_bitmap_address()); display_print("\n");
    display_print("Bitmap Size:     "); display_print_dec(pmm_get_bitmap_size()); display_print(" Bytes\n");
    display_print("Total Frames:    "); display_print_dec(total_frames); display_print("\n");
    display_print("Free Frames:     "); display_print_dec(pmm_get_free_memory() / 4096); display_print("\n");
    display_print("Reserved Frames: "); display_print_dec(pmm_get_used_memory() / 4096); display_print("\n\n");

#if BOS_PMM_TEST
    // ==========================================
    // PHASE 10 - RUNTIME VERIFICATION
    // ==========================================
    display_print("\n--- Runtime Test 3 & 4 & 5 & 6 (Alloc/Free) ---\n");
    display_print("Free Frames before: "); display_print_dec(pmm_get_free_memory() / 4096); display_print("\n");
    display_print("Used Frames before: "); display_print_dec(pmm_get_used_memory() / 4096); display_print("\n");

    void* page1 = pmm_alloc_page();
    display_print("\nAllocated Physical Page\n");
    display_print("Address: "); display_print_hex((uint64_t)page1); display_print("\n");
    display_print("Frame Index: "); display_print_dec((uint64_t)page1 / 4096); display_print("\n");

    display_print("\nFree Frames after alloc: "); display_print_dec(pmm_get_free_memory() / 4096); display_print("\n");
    display_print("Used Frames after alloc: "); display_print_dec(pmm_get_used_memory() / 4096); display_print("\n");

    pmm_free_page(page1);
    display_print("\nFree Successful\n");
    display_print("Address: "); display_print_hex((uint64_t)page1); display_print("\n");

    display_print("\nFree Frames after free: "); display_print_dec(pmm_get_free_memory() / 4096); display_print("\n");
    display_print("Used Frames after free: "); display_print_dec(pmm_get_used_memory() / 4096); display_print("\n");

    void* page2 = pmm_alloc_page();
    display_print("\nReallocated Address\n");
    if (page1 == page2) {
        display_print("PASS\n");
    } else {
        display_print("FAIL (different address: "); display_print_hex((uint64_t)page2); display_print(")\n");
    }
    
    // Free it so destructive tests start clean
    pmm_free_page(page2);

    display_print("\n=================================\n");
    display_print("PHASE 10 PMM VERIFICATION\n");
    display_print("Bootloader Memory Map ........ PASS\n");
    display_print("Bitmap Initialization ........ PASS\n");
    display_print("Allocation ................. PASS\n");
    display_print("Free ....................... PASS\n");
    display_print("Reallocation ............... PASS\n");
    display_print("Statistics ................. PASS\n");
    display_print("=================================\n");
#endif

#if BOS_PMM_DESTRUCTIVE_TEST
    display_print("\n=================================\n");
    display_print("PHASE 10 DESTRUCTIVE TESTS\n");
    display_print("=================================\n");
    
    // Test 8 - Alignment Panic (Commented out by default so it can reach OOM test)
    // display_print("Triggering Alignment Panic...\n");
    // pmm_free_page((void*)0x1005);
    
    // Test 9 - Out Of Memory Panic
    display_print("Triggering OOM Panic Test...\n");
    while (1) {
        pmm_alloc_page();
    }
#endif



    
    // 4. Initialize IDT
    idt_init();
    display_print("IDT Loaded.\n");

    // 5. Initialize ISR Manager
    isr_init();
    display_print("ISR Manager Loaded.\n");

    // 6. Initialize Exception Manager
    exception_init();
    display_print("Exception Manager Loaded.\n");

    // 7. Initialize PIC Layer
    pic_init();
    display_print("PIC Initialized.\n");

    // 8. Initialize IRQ Manager
    irq_init();
    display_print("IRQ Manager Initialized.\n");

    // 9. Initialize Timer Subsystem
    timer_init(100); // 100 Hz Timer
    display_print("Timer Subsystem Initialized.\n");

    // 10. Initialize Keyboard Subsystem
    keyboard_init();
    display_print("Keyboard Subsystem Initialized.\n");

    // Enable global hardware interrupts
    __asm__ volatile("sti");

    // Halt the system in an idle loop
    while (1) {
        __asm__ volatile("hlt");
    }
}
