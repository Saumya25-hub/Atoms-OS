#pragma once
#include <stdint.h>

// Initializes the PIC and remaps IRQ0-IRQ15 to vectors 32-47
void pic_init(void);

// Sends the End of Interrupt (EOI) command to the PIC(s)
void pic_send_eoi(uint8_t irq);

// Masks an IRQ line so it stops generating interrupts
void pic_set_mask(uint8_t irq);

// Unmasks an IRQ line so it can generate interrupts
void pic_clear_mask(uint8_t irq);
