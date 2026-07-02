#pragma once
#include <stdint.h>
#include "kernel/core/interrupt/include/isr.h"

// Function pointer type for hardware IRQ handlers
typedef uint64_t (*irq_handler_t)(registers_t* regs);

// Initialize the IRQ Subsystem
void irq_init(void);

// Register a handler for a specific IRQ line (0-15)
void irq_register_handler(uint8_t irq, irq_handler_t handler);

// Unregister an IRQ handler
void irq_unregister_handler(uint8_t irq);

// The main dispatcher called by ISR Manager for all IRQs
uint64_t irq_dispatch(registers_t* regs);
