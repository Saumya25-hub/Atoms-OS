#include "kernel/display/display.h"
#include "kernel/console/console.h"
#include "drivers/video/vga/vga.h"
#include "arch/x86_64/interrupt/idt.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/interrupt/include/exception.h"
#include "drivers/interrupt/pic/pic.h"

static BackendDriver vga_backend = {
    .init = vga_init,
    .draw_character = vga_draw_character,
    .set_hardware_cursor = vga_set_hardware_cursor,
    .clear_memory = vga_clear_memory,
    .get_width = vga_get_screen_width,
    .get_height = vga_get_screen_height
};

void kernel_main() {
    // 1. Initialize and register the backend
    console_set_backend(&vga_backend);
    
    // 2. Initialize the display subsystem (Terminal Logic)
    display_init();
    display_clear();
    
    // 3. Print using the high-level API
    display_print("SignaturesOS v0.3 - BOS Architecture\n");
    display_print("Kernel OK\n");
    display_print("Display Subsystem V1 Initialized.\n");
    
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

    // Halt the system in an idle loop
    while (1) {
        __asm__ volatile("hlt");
    }
}
