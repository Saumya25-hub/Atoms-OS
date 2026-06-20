#include "kernel/scheduler/include/context.h"
#include "kernel/display/display.h"
#include <stddef.h>

void context_init(void) {
    // Context engine initialization (e.g., TSS setup in the future)
}

void context_prepare_kernel_task(Task* task, void (*entry)(void)) {
    if (!task || !task->stack) return;

    // Top of the stack is stack + size
    uint64_t top_of_stack = (uint64_t)task->stack + KERNEL_TASK_STACK_SIZE;
    
    // The CPU naturally aligns RSP to 16 bytes upon interrupt handling. 
    // However, System V AMD64 ABI requires that upon entry to a C function, 
    // the stack pointer (RSP) + 8 must be a multiple of 16 (i.e. RSP = 16N - 8).
    // This simulates the 8-byte return address pushed by a 'call' instruction.
    top_of_stack &= ~0xFULL;
    top_of_stack -= 8;
    *(uint64_t*)top_of_stack = 0; // Dummy return address

    // Allocate space for the Context structure
    top_of_stack -= sizeof(Context);
    
    // Get pointer to the context frame
    Context* ctx = (Context*)top_of_stack;
    
    // Zero out the entire context
    for (size_t i = 0; i < sizeof(Context); i++) {
        ((uint8_t*)ctx)[i] = 0;
    }

    // Set up the interrupt frame
    ctx->cs = 0x08; // Kernel code segment
    ctx->ss = 0x10; // Kernel data segment
    ctx->rip = (uint64_t)entry;
    
    // Default RFLAGS (Interrupts enabled (IF = 1 << 9) + Reserved bit 1)
    ctx->rflags = 0x202;
    
    // Provide RSP value at the interrupt boundary
    ctx->rsp = top_of_stack + sizeof(Context);

    // Save the context back into the task object
    task->rsp = top_of_stack;
    task->rip = (uint64_t)entry;
}

uint64_t context_get_initial_rsp(Task* task) {
    if (!task) return 0;
    return task->rsp;
}

uint64_t context_get_initial_rip(Task* task) {
    if (!task) return 0;
    return task->rip;
}
