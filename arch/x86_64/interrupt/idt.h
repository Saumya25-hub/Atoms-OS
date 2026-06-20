#pragma once
#include <stdint.h>

void idt_init(void);
void idt_set_gate(uint8_t vector, void* isr, uint8_t flags);
