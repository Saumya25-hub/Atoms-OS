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
#include "kernel/memory/vmm/include/vmm.h"
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
    
    display_print("SignaturesOS v0.3\n");

    // ==========================================
    // BOOT_INFO POINTER VERIFICATION
    // ==========================================
    display_print("boot_info: "); display_print_hex((uint64_t)boot_info); display_print("\n");
    display_print("entry_cnt: "); display_print_dec(boot_info->memory_entry_count); display_print("\n");

    // Validate boot_info pointer range
    if ((uint64_t)boot_info < 0x1000 || (uint64_t)boot_info > 0x90000) {
        display_print("FATAL: boot_info pointer invalid!\n");
        while(1) { __asm__ volatile("hlt"); }
    }

    // Validate entry count is sane
    if (boot_info->memory_entry_count == 0 || boot_info->memory_entry_count > 32) {
        display_print("FATAL: entry_count invalid!\n");
        while(1) { __asm__ volatile("hlt"); }
    }

    // Print first E820 entry to verify data integrity
    display_print("E0 B:"); display_print_hex(boot_info->entries[0].base_address);
    display_print(" L:"); display_print_hex(boot_info->entries[0].length);
    display_print(" T:"); display_print_dec(boot_info->entries[0].type);
    display_print("\n");

    // ==========================================
    // E820 MEMORY MAP (compact)
    // ==========================================
    display_print("\n[E820]\n");
    for (uint32_t i = 0; i < boot_info->memory_entry_count; i++) {
        memory_map_entry_t* entry = &boot_info->entries[i];

        // Validate each entry pointer is within identity map
        if ((uint64_t)entry > 0x200000) {
            display_print("FATAL: E820 entry ptr outside identity map!\n");
            while(1) { __asm__ volatile("hlt"); }
        }

        display_print("R"); display_print_dec(i);
        display_print(" B:"); display_print_hex(entry->base_address);
        display_print(" L:"); display_print_hex(entry->length);
        display_print(" T:"); display_print_dec(entry->type);
        display_print("\n");
    }

    // ==========================================
    // PMM
    // ==========================================
    display_print("\n[K] Before PMM\n");
    pmm_init(boot_info);
    display_print("[K] After PMM\n");

    display_print("Total: "); display_print_dec(pmm_get_total_memory() / 1024); display_print("KB\n");
    display_print("Free:  "); display_print_dec(pmm_get_free_memory() / 1024); display_print("KB\n");
    display_print("Bmp:   "); display_print_hex((uint64_t)pmm_get_bitmap_address()); display_print("\n");
    display_print("Frames:"); display_print_dec(pmm_get_total_frames()); display_print("\n");

    // ==========================================
    // CR3 DUMP (before VMM)
    // ==========================================
    uint64_t cr3_val;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3_val));
    display_print("\n[K] CR3: "); display_print_hex(cr3_val); display_print("\n");

    // ==========================================
    // VMM
    // ==========================================
    display_print("[K] Before VMM\n");
    vmm_init();
    display_print("[K] After VMM\n");

    // ==========================================
    // REMAINING SUBSYSTEMS
    // ==========================================
    idt_init();
    display_print("IDT OK\n");

    isr_init();
    display_print("ISR OK\n");

    exception_init();
    display_print("EXC OK\n");

    pic_init();
    display_print("PIC OK\n");

    irq_init();
    display_print("IRQ OK\n");

    timer_init(100);
    display_print("TMR OK\n");

    keyboard_init();
    display_print("KBD OK\n");

    __asm__ volatile("sti");

    while (1) {
        __asm__ volatile("hlt");
    }
}
