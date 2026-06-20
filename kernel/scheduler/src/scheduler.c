#include "kernel/scheduler/include/scheduler.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"

static Task* runnable_queue_head = NULL;
static Task* runnable_queue_tail = NULL;
static Task* current_task = NULL;
static Task* idle_task_ptr = NULL;

extern uint64_t task_generate_id(void);

static Task* scheduler_pick_next(void) {
    // Sprint 1: No actual switching
    return NULL;
}

static void scheduler_switch(Task* current, Task* next) {
    // Sprint 1: No actual switching
}

static void idle_task(void) {
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
    idle_task_ptr->state = TASK_READY;
    idle_task_ptr->rip = (uint64_t)idle_task;
    idle_task_ptr->next = NULL;
    
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

Task* scheduler_create_kernel_task(void (*entry)(void)) {
    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) return NULL;
    
    task->id = task_generate_id();
    task->state = TASK_READY;
    task->rip = (uint64_t)entry;
    task->next = NULL;
    
    // Allocate stack (e.g. 4KB)
    task->stack = kmalloc(4096);
    task->rsp = (uint64_t)task->stack + 4096;
    
    scheduler_add_task(task);
    
    display_print("Kernel Task Created\n");
    
    return task;
}

Task* scheduler_current_task(void) {
    return current_task;
}

void scheduler_tick(void) {
    // Sprint 1: Stub
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
