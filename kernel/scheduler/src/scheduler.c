#include "kernel/scheduler/include/scheduler.h"
#include "kernel/scheduler/include/context.h"
#include "kernel/scheduler/include/runqueue.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/display/display.h"
#include "arch/x86_64/gdt/gdt.h"
#include "kernel/lib/include/crash_log.h"
#include <stddef.h>

static RunQueue ready_queue;
static RunQueue sleep_queue;
static RunQueue terminated_queue;
Task* current_task = NULL;
Task* idle_task_ptr = NULL;
static uint64_t scheduler_tick_count = 0;
static bool scheduler_running = false;

extern uint64_t task_generate_id(void);

static void scheduler_switch(Task* current, Task* next) {
    // Sprint 1: No actual switching
}

#include "kernel/timer/include/timer.h"

static void idle_task(void) {
    while (1) {
        // Phase 26: Process Cleanup
        // While idle, we check if any task is in the terminated queue and free its resources.
        while (!runqueue_is_empty(&terminated_queue)) {
            __asm__ volatile("cli");
            Task* t = runqueue_pop(&terminated_queue);
            __asm__ volatile("sti");
            
            if (t) {
                // Free kernel stack
                if (t->stack) kfree(t->stack);
                
                // TODO: In the future, we will also free the PML4 here if it's a separate process space
                
                // Free the task struct
                kfree(t);
            }
        }

        // Wait for next interrupt
        __asm__ volatile("hlt" : : : "memory");
    }
}

void scheduler_init(void) {
    display_print("\n[SCHED]\nInit OK\n");
    runqueue_init(&ready_queue);
    runqueue_init(&sleep_queue);
    runqueue_init(&terminated_queue);
    
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
    idle_task_ptr->pml4 = vmm_get_active_pml4();
    list_node_init(&idle_task_ptr->queue_node);
    
    // Allocate stack and prepare context for Idle Task
    idle_task_ptr->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    context_prepare_kernel_task(idle_task_ptr, idle_task);
    
    display_print("Idle Task Created\n");
    
    // Phase 14: Keep current_task NULL until scheduler_start() is invoked
    current_task = NULL;
}

// Register the currently executing code (kernel_main / GUI system task) as a
// proper scheduler task. This allows scheduler_on_tick() to preempt it and
// give CPU time to user processes like SHELL.BOSX.
// Must be called from kernel_main BEFORE the GUI loop.
void scheduler_register_boot_task(void) {
    Task* boot_task = (Task*)kmalloc(sizeof(Task));
    if (!boot_task) {
        display_print("[SCHED] PANIC: Failed to allocate boot task!\n");
        while(1) { __asm__ volatile("hlt"); }
    }
    
    boot_task->id = task_generate_id();
    boot_task->name = "GUI_System";
    boot_task->state = TASK_RUNNING;
    boot_task->quantum = 5;
    boot_task->default_quantum = 5;
    boot_task->is_user_task = 0;
    boot_task->pml4 = vmm_get_active_pml4();
    list_node_init(&boot_task->queue_node);
    
    // Allocate a kernel stack for this task. Even though we're already running
    // on kernel_main's stack, we need a valid stack pointer for TSS.RSP0 
    // when switching back from user tasks.
    boot_task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    boot_task->rsp = 0;           // Will be filled by context_save_state on first tick
    
    current_task = boot_task;
    scheduler_running = true;
    
    display_print("[SCHED] Boot task registered (PID ");
    display_print_dec(boot_task->id);
    display_print("), scheduler enabled\n");
}

void scheduler_add_task(Task* task) {
    if (!task) return;
    
    runqueue_push(&ready_queue, task);
}

void scheduler_terminate_task(Task* task) {
    if (!task) return;
    __asm__ volatile("cli");
    
    char log_buf[64] = "[TASK EXIT] Terminated: ";
    int j = 0;
    while (log_buf[j] != '\0') j++;
    int k = 0;
    while (task->name[k] != '\0' && j < 63) {
        log_buf[j++] = task->name[k++];
    }
    log_buf[j] = '\0';
    crash_log_add(log_buf);

    task->state = TASK_TERMINATED;
    runqueue_push(&terminated_queue, task);
    __asm__ volatile("sti");
}

Task* scheduler_create_kernel_task(const char* name, void (*entry)(void)) {
    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) return NULL;
    
    task->id = task_generate_id();
    task->name = name;
    
    char log_buf[64] = "[SPAWN] Kernel Task: ";
    int j = 0;
    while (log_buf[j] != '\0') j++;
    int k = 0;
    while (name[k] != '\0' && j < 63) {
        log_buf[j++] = name[k++];
    }
    log_buf[j] = '\0';
    crash_log_add(log_buf);

    task->state = TASK_READY;
    task->quantum = 0;
    task->default_quantum = 5;
    task->pml4 = vmm_get_active_pml4();
    list_node_init(&task->queue_node);
    
    // Allocate stack dynamically based on the architecture define
    task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    task->is_user_task = 0;
    
    // Prepare the CPU Context on the task's stack
    context_prepare_kernel_task(task, entry);
    
    scheduler_add_task(task);
    
    return task;
}

Task* scheduler_create_user_task(const char* name, void (*entry)(void)) {
    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) return NULL;
    
    task->id = task_generate_id();
    task->name = name;
    
    char log_buf2[64] = "[SPAWN] User Task: ";
    int j2 = 0;
    while (log_buf2[j2] != '\0') j2++;
    int k2 = 0;
    while (name[k2] != '\0' && j2 < 63) {
        log_buf2[j2++] = name[k2++];
    }
    log_buf2[j2] = '\0';
    crash_log_add(log_buf2);

    task->state = TASK_READY;
    task->quantum = 0;
    task->default_quantum = 5;
    task->pml4 = vmm_get_active_pml4();
    list_node_init(&task->queue_node);
    
    // Allocate Ring 0 Stack
    task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    
    // Allocate Ring 3 Stack
    task->user_stack = kmalloc(KERNEL_TASK_STACK_SIZE); // 8KB for user stack
    task->is_user_task = 1;
    
    // User task starts in kernel mode initially via a wrapper that calls enter_usermode
    extern void enter_usermode(uint64_t entry_point, uint64_t user_stack);
    
    // We will prepare the kernel context to jump to a trampoline that calls enter_usermode
    // But since context_prepare_kernel_task takes a void(*)(void), we need a custom wrapper or
    // we can manually setup the context for enter_usermode!
    // Wait, enter_usermode takes RDI and RSI. The System V ABI passes RDI=arg1, RSI=arg2.
    // context_prepare_kernel_task doesn't set RDI/RSI.
    // Let's manually prepare the context for the user task:
    
    uint64_t* stack_ptr = (uint64_t*)((uint8_t*)task->stack + KERNEL_TASK_STACK_SIZE);
    
    // 1. Interrupt Frame (5 items: RIP, CS, RFLAGS, RSP, SS)
    *(--stack_ptr) = 0x10; // SS (Kernel Data)
    *(--stack_ptr) = (uint64_t)task->stack + KERNEL_TASK_STACK_SIZE; // RSP
    *(--stack_ptr) = 0x202; // RFLAGS (IF=1)
    *(--stack_ptr) = 0x08; // CS (Kernel Code)
    *(--stack_ptr) = (uint64_t)enter_usermode; // RIP
    
    // 2. Error Code & Int No (2 items)
    *(--stack_ptr) = 0;
    *(--stack_ptr) = 0;
    
    // 3. General Purpose Registers (15 items)
    *(--stack_ptr) = 0; // RAX
    *(--stack_ptr) = 0; // RBX
    *(--stack_ptr) = 0; // RCX
    *(--stack_ptr) = 0; // RDX
    *(--stack_ptr) = (uint64_t)task->user_stack + KERNEL_TASK_STACK_SIZE; // RSI (user_stack)
    *(--stack_ptr) = (uint64_t)entry; // RDI (entry_point)
    *(--stack_ptr) = 0; // RBP
    *(--stack_ptr) = 0; // R8
    *(--stack_ptr) = 0; // R9
    *(--stack_ptr) = 0; // R10
    *(--stack_ptr) = 0; // R11
    *(--stack_ptr) = 0; // R12
    *(--stack_ptr) = 0; // R13
    *(--stack_ptr) = 0; // R14
    *(--stack_ptr) = 0; // R15
    
    task->rsp = (uint64_t)stack_ptr;
    
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

    // Ping-pong prevention: enforce minimum 1 tick gap
    if (current_task->last_run_tick == scheduler_tick_count) {
        return; // Reject immediate re-yield in the same tick
    }
    current_task->last_run_tick = scheduler_tick_count;

    // Voluntarily clear the quantum to force a switch on the next tick
    current_task->quantum = 0;
    
    // Wait for the hardware timer to perform the safe context switch
    __asm__ volatile("sti");
    __asm__ volatile("hlt" : : : "memory");
}

void scheduler_on_tick(void) {
    if (!scheduler_running) return;
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
            if (old_task->state == TASK_RUNNING && old_task->quantum > 0) {
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
        if (new_task != old_task) {
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
        }
        
        if (new_task->state != TASK_RUNNING) {
            task_transition(new_task, TASK_RUNNING);
        }
        new_task->quantum = new_task->default_quantum; // Reload quantum
        
        current_task = new_task;
        
        // Ensure TSS.RSP0 is updated for the new task to receive Ring 3 interrupts!
        tss_set_kernel_stack((uint64_t)current_task->stack + KERNEL_TASK_STACK_SIZE);

        // Phase 27: Switch Address Space
        if (old_task->pml4 != current_task->pml4) {
            vmm_switch_address_space(current_task->pml4);
        }
    }
}

void scheduler_start(void) {
    if (!runqueue_is_empty(&ready_queue)) {
        Task* first_task = runqueue_pop(&ready_queue);
        task_transition(first_task, TASK_RUNNING);
        first_task->quantum = first_task->default_quantum;
        current_task = first_task;
        tss_set_kernel_stack((uint64_t)current_task->stack + KERNEL_TASK_STACK_SIZE);
        vmm_switch_address_space(current_task->pml4);
    }
    if (current_task) {
        scheduler_running = true;
        context_switch_first(current_task);
    }
}

uint32_t scheduler_get_task_count(void) {
    return runqueue_get_size(&ready_queue);
}

Task* scheduler_get_idle_task(void) {
    return idle_task_ptr;
}

static const char* state_to_str(TaskState state) {
    switch(state) {
        case TASK_READY: return "READY";
        case TASK_RUNNING: return "RUNNING";
        case TASK_BLOCKED: return "BLOCKED";
        case TASK_SLEEPING: return "SLEEPING";
        case TASK_TERMINATED: return "DEAD";
        default: return "UNKNOWN";
    }
}

void scheduler_dump_tasks(void) {
    display_print("\n--- Task Diagnostics ---\n");
    display_print("PID   STATE       NAME\n");
    display_print("------------------------\n");
    
    if (current_task) {
        display_print_dec(current_task->id);
        display_print("     ");
        display_print(state_to_str(current_task->state));
        display_print("     ");
        display_print(current_task->name);
        display_print("\n");
    }

    list_node_t* node = ready_queue.ready_list.head;
    while(node) {
        Task* t = (Task*)((uint8_t*)node - offsetof(Task, queue_node));
        if (t != current_task) {
            display_print_dec(t->id); display_print("     ");
            display_print(state_to_str(t->state)); display_print("     ");
            display_print(t->name); display_print("\n");
        }
        node = node->next;
    }

    node = sleep_queue.ready_list.head;
    while(node) {
        Task* t = (Task*)((uint8_t*)node - offsetof(Task, queue_node));
        display_print_dec(t->id); display_print("     ");
        display_print(state_to_str(t->state)); display_print("     ");
        display_print(t->name); display_print("\n");
        node = node->next;
    }
    display_print("------------------------\n");
}

void scheduler_dump_task_info(uint64_t pid) {
    display_print("\n--- Task Info (PID: "); display_print_dec(pid); display_print(") ---\n");
    
    Task* target = NULL;
    if (current_task && current_task->id == pid) target = current_task;
    
    list_node_t* node = ready_queue.ready_list.head;
    while(node && !target) {
        Task* t = (Task*)((uint8_t*)node - offsetof(Task, queue_node));
        if (t->id == pid) target = t;
        node = node->next;
    }
    
    node = sleep_queue.ready_list.head;
    while(node && !target) {
        Task* t = (Task*)((uint8_t*)node - offsetof(Task, queue_node));
        if (t->id == pid) target = t;
        node = node->next;
    }

    if (!target) {
        display_print("Task not found.\n");
        display_print("------------------------\n");
        return;
    }

    display_print("Name     : "); display_print(target->name); display_print("\n");
    display_print("State    : "); display_print(state_to_str(target->state)); display_print("\n");
    display_print("Priority : Default\n");
    display_print("RSP Base : "); display_print_hex((uint64_t)target->stack); display_print("\n");
    if (target->is_user_task) {
        display_print("RSP User : "); display_print_hex((uint64_t)target->user_stack); display_print("\n");
    }
    display_print("CR3      : "); display_print_hex((uint64_t)target->pml4); display_print("\n");
    display_print("------------------------\n");
}
