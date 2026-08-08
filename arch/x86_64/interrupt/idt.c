#include "idt.h"
#include "kernel/debug/abde/abde.h"

// IDT Entry Structure (64-bit Interrupt Gate)
typedef struct {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

// IDTR Structure
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

// Array of 256 IDT entries
static idt_entry_t idt[256];
static idtr_t idtr;

void idt_set_gate(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t addr = (uint64_t)isr;
    
    idt[vector].isr_low = (uint16_t)(addr & 0xFFFF);
    idt[vector].kernel_cs = 0x08; // 0x08 is the kernel code segment
    idt[vector].ist = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].isr_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].reserved = 0;
}

void idt_init(void) {
    diag_set_step("BUILD IDT DESCRIPTORS");
    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt) - 1;

    diag_set_step("LOAD IDTR (LIDT)");
    // Load the IDT using LIDT
    __asm__ volatile("lidt %0" : : "m"(idtr));

    diag_set_idt_telemetry(256, idtr.base, 256, true, "NONE", 0);
    diag_set_step("IDTR LOADED 100%");
}
