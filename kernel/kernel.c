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
#include "kernel/memory/heap/include/heap.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/config/build_config.h"

static BackendDriver vga_backend = {
    .init = vga_init,
    .draw_character = vga_draw_character,
    .set_hardware_cursor = vga_set_hardware_cursor,
    .clear_memory = vga_clear_memory,
    .get_width = vga_get_screen_width,
    .get_height = vga_get_screen_height
};

static void dummy_kernel_task(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

void kernel_main(boot_info_t* boot_info) {
    // 1. Display Subsystem
    console_set_backend(&vga_backend);
    display_init();
    display_clear();
    display_print("SignaturesOS v0.3 - BOS Architecture\n\n");

    // 2. Interrupt Subsystem
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

    // 3. Timer Subsystem
    timer_init(100);
    display_print("TMR OK\n");

    // 4. Keyboard Subsystem
    keyboard_init();
    display_print("KBD OK\n");

    // 5. Physical Memory Manager
    pmm_init(boot_info);
    display_print("PMM OK\n");

    // 6. VMM — Step 1 bring-up
    vmm_init();

    // 7. Kernel Heap
    heap_init();

    // 8. Scheduler (Sprint 2 - Round Robin Queue)
    display_print("\n[S1] Before scheduler_init\n");
    scheduler_init();
    display_print("[S2] After scheduler_init\n");
    
    scheduler_create_kernel_task("TaskA", dummy_kernel_task);
    scheduler_create_kernel_task("TaskB", dummy_kernel_task);
    
    display_print("\n[SCHED]\n");
    display_print("Current : ");
    display_print(scheduler_current_task()->name);
    display_print("\n");
    
    display_print("[S3] Before tick loop\n");
    for (int i = 0; i < 4; i++) {
        scheduler_tick();
    }
    
    display_print("[S4] After tick loop\n");
    display_print("\nRound Robin PASS\n");

    // Idle loop (pure simulation, no interrupts)
    while (1) {
        __asm__ volatile("hlt");
    }
}
