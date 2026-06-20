#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include "kernel/scheduler/include/task.h"

// Context state structure (future-proofing for Sprint 3 context switching)
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

#endif
