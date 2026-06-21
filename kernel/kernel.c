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
#include "kernel/scheduler/include/context.h"
#include "kernel/scheduler/include/runqueue.h"
#include "kernel/config/build_config.h"
#include "kernel/lib/include/list.h"
#include <stddef.h>

_Static_assert(sizeof(Task) == 64, "Task struct size mismatch!");
_Static_assert(offsetof(Task, rsp) == 32, "Task rsp offset mismatch!");
_Static_assert(sizeof(Context) == 176, "Context struct size mismatch!");

static BackendDriver vga_backend = {
    .init = vga_init,
    .draw_character = vga_draw_character,
    .set_hardware_cursor = vga_set_hardware_cursor,
    .clear_memory = vga_clear_memory,
    .get_width = vga_get_screen_width,
    .get_height = vga_get_screen_height
};

typedef struct {
    uint32_t magic;
    list_node_t queue_node;
} TestNode;

static void test_intrusive_list(void) {
    list_t my_list;
    list_init(&my_list);

    TestNode n1, n2;
    n1.magic = 111;
    n2.magic = 222;
    
    list_node_init(&n1.queue_node);
    list_node_init(&n2.queue_node);

    list_insert_tail(&my_list, &n1.queue_node);
    list_insert_tail(&my_list, &n2.queue_node);

    if (my_list.size != 2) {
        display_print("[TEST] Intrusive List: SIZE FAIL\n");
        while(1) __asm__ volatile("hlt");
    }

    list_node_t* popped = list_remove_head(&my_list);
    TestNode* popped_node = LIST_ENTRY(popped, TestNode, queue_node);

    if (popped_node->magic != 111 || my_list.size != 1) {
        display_print("[TEST] Intrusive List: POP FAIL\n");
        while(1) __asm__ volatile("hlt");
    }

    display_print("[TEST] Intrusive List Validation: PASS\n");
}

static void task_a_entry(void) {
    for(volatile uint64_t i=0;;i++) {
        // Real CPU workload
    }
}

static void task_b_entry(void) {
    for(volatile uint64_t i=0;;i++) {
        // Real CPU workload
    }
}

void kernel_main(boot_info_t* boot_info) {
    // 1. Display Subsystem
    console_set_backend(&vga_backend);
    display_init();
    display_clear();
    display_print("SignaturesOS v0.3 - BOS Architecture\n\n");
    
    test_intrusive_list();

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

    // 5. Physical Memory Manager
    pmm_init(boot_info);
    display_print("PMM OK\n");

    // 6. VMM — Step 1 bring-up
    vmm_init();

    // 7. Kernel Heap
    heap_init();

    // 8. Scheduler (Sprint 5 - Timer Driven Entry)
    context_init();
    scheduler_init();
    
    // 3. Timer Subsystem (Must be after Scheduler)
    timer_init(100);
    display_print("TMR OK\n");

    // 4. Keyboard Subsystem
    keyboard_init();
    display_print("KBD OK\n");
    display_print("\n4. Multitasking Subsystem\n");
    scheduler_create_kernel_task("TaskA", task_a_entry);
    scheduler_create_kernel_task("TaskB", task_b_entry);
    
    display_clear();
    display_print("Soak Test Initializing...\n");
    
    scheduler_start();
}
