#include "kernel/core/process/include/process.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/scheduler/include/context.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

// Defined in enter_usermode.asm
extern void enter_usermode(uint64_t rip, uint64_t rsp);

Task* process_spawn(ProcessImage* image, const char* name) {
    if (!image || !image->pml4) return NULL;

    ATOMS_PCB* pcb = NULL;
    if (image->pid == 0) {
        // Automatically allocate PCB if not already set (e.g. boot time / horse engine launches)
        Task* curr = scheduler_current_task();
        uint32_t ppid = curr ? (uint32_t)curr->id : 0;
        pcb = ATOMS_Process_Create(name, name, ppid, 0);
        if (!pcb) return NULL;
        pcb->pml4_phys = (uint64_t)image->pml4;
        image->pid = pcb->pid;
    } else {
        pcb = ATOMS_Process_GetByPID(image->pid);
    }

    Task* task = (Task*)kmalloc(sizeof(Task));
    if (!task) {
        if (pcb) ATOMS_Process_Terminate(pcb->pid, -1);
        return NULL;
    }
    memset(task, 0, sizeof(Task));

    task->id = image->pid;
    task->owner_pid = image->pid;
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
        if (pcb) ATOMS_Process_Terminate(pcb->pid, -1);
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
    *(--stack) = 0; // dummy err_code
    *(--stack) = 0; // dummy int_no

    // 3. General Purpose Registers (15 items)
    for (int i = 0; i < 15; i++) {
        *(--stack) = 0;
    }

    task->rsp = (uint64_t)stack;

    // Register thread in the PCB
    if (pcb) {
        if (pcb->thread_count < ATOMS_MAX_THREADS_PER_PROC) {
            pcb->thread_ids[pcb->thread_count] = (uint32_t)task->id;
            pcb->thread_count++;
        }
        pcb->state = ATOMS_PROC_STATE_RUNNING;
    }

    // Add to scheduler
    scheduler_add_task(task);

    return task;
}
