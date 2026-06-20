#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include "kernel/scheduler/include/task.h"

// Context state structure (Interrupt Frame + General Registers)
typedef struct Context {
    // General purpose registers to be saved/restored
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    
    // Interrupt frame
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} Context;

void context_init(void);
void context_prepare_kernel_task(Task* task, void (*entry)(void));
uint64_t context_get_initial_rsp(Task* task);
uint64_t context_get_initial_rip(Task* task);

void context_switch_first(Task* task);

#endif
