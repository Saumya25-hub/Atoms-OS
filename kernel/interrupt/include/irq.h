#pragma once
#include <stdint.h>
#include "kernel/interrupt/include/isr.h"

// Function pointer type for IRQ handlers
typedef void (*irq_handler_t)(registers_t* regs);

// Initialize the IRQ Manager
void irq_init(void);

// Register an IRQ handler for a specific hardware IRQ line (0-15)
void irq_register_handler(uint8_t irq, irq_handler_t handler);

// Unregister an IRQ handler
void irq_unregister_handler(uint8_t irq);

// Internal dispatch function called by the ISR routing layer
void irq_dispatch(registers_t* regs);
