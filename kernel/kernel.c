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
#include "kernel/syscall/include/syscall.h"
#include <stddef.h>

_Static_assert(sizeof(Task) == 80, "Task struct size mismatch!");
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
    while (1) {
        display_print("[PID ");
        display_print_dec(sys_getpid());
        display_print("] Tick ");
        display_print_dec(sys_uptime());
        display_print(" : A\n");
        sys_sleep(50); // Sleep via syscall
    }
}

static void task_b_entry(void) {
    while (1) {
        display_print("[PID ");
        display_print_dec(sys_getpid());
        display_print("] Tick ");
        display_print_dec(sys_uptime());
        display_print(" : B\n");
        sys_sleep(200); // Sleep via syscall
    }
}

static void task_c_entry(void) {
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] Stress Test Started\n");
    
    // 1. Stress test SYS_UPTIME (100,000 calls)
    for (volatile int i = 0; i < 100000; i++) {
        volatile uint64_t uptime = sys_uptime();
        (void)uptime; // Prevent optimization
    }
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] 100K SYS_UPTIME: PASS\n");
    
    // 2. Test SYS_YIELD
    sys_yield();
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] SYS_YIELD: PASS\n");
    
    // 3. Test Invalid Syscall
    uint64_t err;
    __asm__ volatile("mov $999, %%rax; int $0x80; mov %%rax, %0" : "=r"(err) : : "rax", "memory");
    if (err == (uint64_t)-1) {
        display_print("[PID ");
        display_print_dec(sys_getpid());
        display_print("] SYS_INVALID: PASS\n");
    }

    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] Stress Test Complete. Sleeping forever.\n");
    
    while(1) {
        sys_sleep(100000);
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
    scheduler_create_kernel_task("TaskStress", task_c_entry);
    
    display_clear();
    display_print("Phase 18 Validation Initializing...\n");
    
    syscall_init();
    display_print("Syscalls OK\n");
    
    // Validation Test: Invalid Syscall
    uint64_t err;
    __asm__ volatile("mov $999, %%rax; int $0x80; mov %%rax, %0" : "=r"(err) : : "rax", "memory");
    if (err == (uint64_t)-1) {
        display_print("[VALIDATION] SYS_INVALID handled safely\n");
    } else {
        display_print("[VALIDATION] SYS_INVALID failed\n");
    }

    scheduler_start();
}
