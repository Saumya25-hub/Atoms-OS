#pragma once
#include <stdint.h>

// Registers state passed to ISR handlers
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

// Function pointer type for ISR handlers
typedef uint64_t (*isr_t)(registers_t* regs);

// Initialize the ISR Manager
void isr_init(void);

// Register a handler for a specific interrupt vector
void isr_register_handler(uint8_t vector, isr_t handler);
