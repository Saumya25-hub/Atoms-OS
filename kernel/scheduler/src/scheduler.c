#include "kernel/scheduler/include/scheduler.h"
#include "kernel/scheduler/include/context.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"
static Task* runnable_queue_head = NULL;
static Task* runnable_queue_tail = NULL;
static Task* current_task = NULL;
static Task* idle_task_ptr = NULL;
static uint64_t scheduler_tick_count = 0;

extern uint64_t task_generate_id(void);

static Task* scheduler_pick_next(void) {
    if (!current_task || !current_task->next) {
        return runnable_queue_head;
    }
    return current_task->next;
}

static void scheduler_switch(Task* current, Task* next) {
    // Sprint 1: No actual switching
}

static void idle_task(void) {
    // The Idle Task is immortal. It can never enter TASK_TERMINATED 
    // and is never removed from the runnable queue.
    while (1) {
        __asm__ volatile("hlt");
    }
}

void scheduler_init(void) {
    display_print("\n[SCHED]\nInit OK\n");
    
    // Create idle task
    idle_task_ptr = (Task*)kmalloc(sizeof(Task));
    if (!idle_task_ptr) {
        display_print("[SCHED] PANIC: Failed to allocate idle task!\n");
        while(1) { __asm__ volatile("hlt"); }
    }
    
    idle_task_ptr->id = task_generate_id();
    idle_task_ptr->name = "Idle";
    idle_task_ptr->state = TASK_READY;
    idle_task_ptr->next = NULL;
    
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
    
    task->next = NULL;
    if (!runnable_queue_head) {
        runnable_queue_head = task;
        runnable_queue_tail = task;
    } else {
        runnable_queue_tail->next = task;
        runnable_queue_tail = task;
    }
}

Task* scheduler_create_kernel_task(const char* name, void (*entry)(void)) {
    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) return NULL;
    
    task->id = task_generate_id();
    task->name = name;
    task->state = TASK_READY;
    task->next = NULL;
    
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
    if (!runnable_queue_head) return;
    
    current_task = scheduler_pick_next();
    
    display_print("Next : ");
    display_print(current_task->name);
    display_print("\n");
}

void scheduler_on_tick(void) {
    scheduler_tick_count++;
    
    display_print("Tick : ");
    char num_str[2] = {(char)('0' + scheduler_tick_count), '\0'};
    display_print(num_str);
    display_print("\n\n");
    
    if (scheduler_tick_count >= 5) {
        display_print("Timer PASS\n\nSystem Halted\n");
        __asm__ volatile("cli");
        while (1) { __asm__ volatile("hlt"); }
    }
}

void scheduler_start(void) {
    // Sprint 1: Stub
}

uint32_t scheduler_get_task_count(void) {
    uint32_t count = 0;
    Task* curr = runnable_queue_head;
    while (curr) {
        count++;
        curr = curr->next;
    }
    return count;
}

Task* scheduler_get_idle_task(void) {
    return idle_task_ptr;
}
