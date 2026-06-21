#include "kernel/interrupt/include/isr.h"
#include "arch/x86_64/interrupt/idt.h"
#include <stddef.h>

// Array of registered handlers
static isr_t interrupt_handlers[256];

// The array of stubs defined in assembly
extern uint64_t isr_stub_table[];

void isr_init(void) {
    // Clear all handlers
    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = NULL;
    }

    // Register all stubs in the IDT
    for (int i = 0; i < 256; i++) {
        uint8_t flags = 0x8E; // 0x8E: Present (0x80) | DPL0 (0x00) | Interrupt Gate (0x0E)
        if (i == 0x80) {
            flags = 0xEE; // 0xEE: Present (0x80) | DPL3 (0x60) | Interrupt Gate (0x0E)
        }
        idt_set_gate(i, (void*)isr_stub_table[i], flags);
    }
}

void isr_register_handler(uint8_t vector, isr_t handler) {
    interrupt_handlers[vector] = handler;
}

// This is called from the assembly stubs
uint64_t isr_common_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        return handler(regs);
    }
    return 0;
}
