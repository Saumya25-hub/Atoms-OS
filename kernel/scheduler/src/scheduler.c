#include "kernel/scheduler/include/scheduler.h"
#include "kernel/scheduler/include/context.h"
#include "kernel/scheduler/include/runqueue.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"

static RunQueue ready_queue;
static RunQueue sleep_queue;
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
        // Silenced for timing test
        __asm__ volatile("hlt" : : : "memory");
    }
}

void scheduler_init(void) {
    display_print("\n[SCHED]\nInit OK\n");
    runqueue_init(&ready_queue);
    runqueue_init(&sleep_queue);
    
    // Create idle task
    idle_task_ptr = (Task*)kmalloc(sizeof(Task));
    if (!idle_task_ptr) {
        display_print("[SCHED] PANIC: Failed to allocate idle task!\n");
        while(1) { __asm__ volatile("hlt"); }
    }
    
    idle_task_ptr->id = task_generate_id();
    idle_task_ptr->name = "Idle";
    idle_task_ptr->state = TASK_RUNNING;
    idle_task_ptr->quantum = 0;
    idle_task_ptr->default_quantum = 5;
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
    task->quantum = 0;
    task->default_quantum = 5;
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

void scheduler_sleep(uint64_t ticks) {
    if (!current_task || current_task == idle_task_ptr) return;

    // We must disable interrupts to safely modify queues
    __asm__ volatile("cli");
    
    current_task->wake_tick = timer_get_ticks() + ticks;
    task_transition(current_task, TASK_SLEEPING);
    runqueue_push(&sleep_queue, current_task);
    
    __asm__ volatile("sti");

    // Block here. The next real hardware timer tick will cleanly preempt this task
    // and ignore it since its state is no longer TASK_RUNNING.
    while (current_task->state == TASK_SLEEPING) {
        __asm__ volatile("hlt" : : : "memory");
    }
}

void scheduler_yield(void) {
    if (!current_task || current_task == idle_task_ptr) return;

    // Voluntarily clear the quantum to force a switch on the next tick
    current_task->quantum = 0;
    
    // Wait for the hardware timer to perform the safe context switch
    __asm__ volatile("sti");
    __asm__ volatile("hlt" : : : "memory");
}

void scheduler_on_tick(void) {
    scheduler_tick_count++;

    // 1. Wakeup Phase: Check the sleep queue for expired timers
    uint64_t current_time = timer_get_ticks();
    
    // We must manually traverse the list.
    list_node_t* current_node = sleep_queue.ready_list.head;
    while (current_node != NULL) {
        list_node_t* next_node = current_node->next;
        
        Task* t = (Task*)((uint8_t*)current_node - offsetof(Task, queue_node));
        
        if (current_time >= t->wake_tick) {
            // Wake this task up!
            runqueue_remove(&sleep_queue, t);
            
            task_transition(t, TASK_READY);
            runqueue_push(&ready_queue, t);
        }
        
        current_node = next_node;
    }

    // 2. Quantum Phase: Process the currently running task
    if (current_task && current_task != idle_task_ptr) {
        current_task->quantum--;
        
        if (current_task->quantum > 0 && current_task->state == TASK_RUNNING) {
            // Keep running current task ONLY if it is still running (didn't sleep)
            return;
        }
    }

    // Quantum expired, or task yielded/slept, or we are running idle
    Task* old_task = current_task;
    Task* new_task = NULL;

    if (!runqueue_is_empty(&ready_queue)) {
        new_task = runqueue_pop(&ready_queue);
    } else {
        if (old_task != idle_task_ptr) {
            if (old_task->state == TASK_RUNNING) {
                old_task->quantum = old_task->default_quantum;
                return; // Keep running the only active task
            } else {
                // The current task is SLEEPING or blocked, we MUST switch to Idle!
                new_task = idle_task_ptr;
            }
        } else {
            // We are already running the Idle task and nothing is ready. Keep idling.
            return;
        }
    }

    if (new_task) {
        task_transition(new_task, TASK_RUNNING);
        new_task->quantum = new_task->default_quantum; // Reload quantum
        
        if (old_task != idle_task_ptr) {
            // If the task voluntarily slept, its state is already TASK_SLEEPING.
            // We ONLY push it back to the ready queue if it was preempted normally.
            if (old_task->state == TASK_RUNNING) {
                task_transition(old_task, TASK_READY);
                runqueue_push(&ready_queue, old_task);
            }
        } else {
            task_transition(old_task, TASK_READY);
        }
        
        current_task = new_task;
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
