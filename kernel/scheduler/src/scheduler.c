#include "kernel/scheduler/include/scheduler.h"
#include "kernel/scheduler/include/context.h"
#include "kernel/scheduler/include/runqueue.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"

static RunQueue ready_queue;
static Task* current_task = NULL;
static Task* idle_task_ptr = NULL;
static uint64_t scheduler_tick_count = 0;

extern uint64_t task_generate_id(void);

static void scheduler_switch(Task* current, Task* next) {
    // Sprint 1: No actual switching
}

#include "kernel/timer/include/timer.h"

static void idle_task(void) {
    while (1) {
        display_print("Idle Running\n");
        __asm__ volatile("hlt");
    }
}

void scheduler_init(void) {
    display_print("\n[SCHED]\nInit OK\n");
    runqueue_init(&ready_queue);
    
    // Create idle task
    idle_task_ptr = (Task*)kmalloc(sizeof(Task));
    if (!idle_task_ptr) {
        display_print("[SCHED] PANIC: Failed to allocate idle task!\n");
        while(1) { __asm__ volatile("hlt"); }
    }
    
    idle_task_ptr->id = task_generate_id();
    idle_task_ptr->name = "Idle";
    idle_task_ptr->state = TASK_RUNNING;
    list_node_init(&idle_task_ptr->queue_node);
    
    // Allocate stack and prepare context for Idle Task
    idle_task_ptr->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    context_prepare_kernel_task(idle_task_ptr, idle_task);
    
    scheduler_add_task(idle_task_ptr);
    
    display_print("Idle Task Created\n");
    
    // For Sprint 1, we don't start it, just set it as current
    current_task = idle_task_ptr;
}

void scheduler_add_task(Task* task) {
    if (!task) return;
    
    runqueue_push(&ready_queue, task);
}

Task* scheduler_create_kernel_task(const char* name, void (*entry)(void)) {
    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) return NULL;
    
    task->id = task_generate_id();
    task->name = name;
    task->state = TASK_READY;
    list_node_init(&task->queue_node);
    
    // Allocate stack dynamically based on the architecture define
    task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    
    // Prepare the CPU Context on the task's stack
    context_prepare_kernel_task(task, entry);
    
    scheduler_add_task(task);
    
    return task;
}

Task* scheduler_current_task(void) {
    return current_task;
}

void scheduler_tick(void) {
    // Sprint 1 Manual Next Call
}

void scheduler_on_tick(void) {
    scheduler_tick_count++;

    if (ready_queue.magic != RUNQUEUE_MAGIC) {
        display_print("PANIC: RUNQUEUE CORRUPTED!\n");
        __asm__ volatile("cli; hlt");
    }

    if (runqueue_is_empty(&ready_queue)) {
        goto check_halt;
    }

    Task* old_task = current_task;
    Task* new_task = runqueue_pop(&ready_queue);

    if (new_task) {
        task_transition(new_task, TASK_RUNNING);
        task_transition(old_task, TASK_READY);
        
        runqueue_push(&ready_queue, old_task);
        
        current_task = new_task;
    }

check_halt:

    if (scheduler_tick_count > 990) {
        // We do not have printf yet, so we just print a simple line
        // to show we are still alive and switching correctly
        display_print("Tick > 990... Queue Size Checked\n");
        if (runqueue_get_size(&ready_queue) != 2) {
            display_print("PANIC: QUEUE SIZE MISMATCH!\n");
            __asm__ volatile("cli; hlt");
        }
    }

    // Stop at 1000 ticks to prove stability
    if (scheduler_tick_count >= 1000) {
        display_print("\n1000-Tick Validation Passed.\n");
        display_print("System Halted for Verification.\n");
        __asm__ volatile("cli");
        while (1) { __asm__ volatile("hlt"); }
    }
}

void scheduler_start(void) {
    if (current_task) {
        context_switch_first(current_task);
    }
}

uint32_t scheduler_get_task_count(void) {
    return runqueue_get_size(&ready_queue);
}

Task* scheduler_get_idle_task(void) {
    return idle_task_ptr;
}
