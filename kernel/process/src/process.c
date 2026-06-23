#include "kernel/process/include/process.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/scheduler/include/context.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/lib/include/string.h"

// Defined in enter_usermode.asm
extern void enter_usermode(uint64_t rip, uint64_t rsp);

Task* process_spawn(ProcessImage* image, const char* name) {
    if (!image || !image->pml4) return NULL;

    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) return NULL;

    // Use scheduler's internal ID generator
    extern uint64_t task_generate_id(void);
    task->id = task_generate_id();
    image->pid = (uint32_t)task->id;

    // We strdup the name or just point to it. Assuming 'name' is statically allocated or we copy it.
    // For now, just copy pointer.
    task->name = name;
    task->state = TASK_READY;
    task->quantum = 0;
    task->default_quantum = 5;
    task->is_user_task = 1;
    task->pml4 = image->pml4;
    task->user_stack = (void*)image->stack_bottom;
    list_node_init(&task->queue_node);

    // Allocate kernel stack for the task
    task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
    if (!task->stack) {
        kfree(task);
        return NULL;
    }

    // Set up the interrupt frame for enter_usermode
    uint64_t* stack = (uint64_t*)((uint64_t)task->stack + KERNEL_TASK_STACK_SIZE);

    // 1. Interrupt Frame for iretq (5 items)
    *(--stack) = 0x1B; // SS: User Data Segment (Selector 0x18 | RPL 3)
    *(--stack) = image->stack_top; // RSP: User Stack Pointer
    *(--stack) = 0x202; // RFLAGS (Interrupts Enabled)
    *(--stack) = 0x23; // CS: User Code Segment (Selector 0x20 | RPL 3)
    *(--stack) = image->entry_point; // RIP: User Instruction Pointer

    // 2. Dummy Error Code & Int No (2 items)
    // context_switch_first does `add rsp, 16` before iretq
    *(--stack) = 0; // dummy err_code
    *(--stack) = 0; // dummy int_no

    // 3. General Purpose Registers (15 items)
    // context_switch_first pops 15 items in reverse order:
    // r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax
    for (int i = 0; i < 15; i++) {
        *(--stack) = 0;
    }

    task->rsp = (uint64_t)stack;

    // Add to scheduler
    scheduler_add_task(task);

    return task;
}
