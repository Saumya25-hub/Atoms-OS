#include "gdt.h"
#include <stddef.h>

// Extern functions defined in gdt_flush.asm
extern void gdt_flush(uint64_t gdtr_ptr);
extern void tss_flush(void);

// 5 standard entries + 1 TSS (which takes 2 entries in 64-bit mode)
// Wait! In 64-bit mode, the TSS descriptor is 16 bytes. A standard GDT entry is 8 bytes.
// So the total size of the GDT is: Null (1) + KCode (1) + KData (1) + UData (1) + UCode (1) + TSS (2) = 7 entries.
static gdt_entry_t gdt[7];
static gdtr_t gdtr;
static tss_t tss;

static void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

static void set_tss_entry(int num, uint64_t base, uint32_t limit) {
    // A TSS descriptor is 16 bytes (takes two 8-byte slots)
    tss_descriptor_t* tss_desc = (tss_descriptor_t*)&gdt[num];
    
    tss_desc->limit_low = limit & 0xFFFF;
    tss_desc->base_low = base & 0xFFFF;
    tss_desc->base_middle = (base >> 16) & 0xFF;
    tss_desc->access = 0x89; // Present, DPL0, Type=9 (Available 64-bit TSS)
    tss_desc->granularity = ((limit >> 16) & 0x0F) | 0x00; // No granularity, byte-granular
    tss_desc->base_high = (base >> 24) & 0xFF;
    tss_desc->base_upper = (base >> 32) & 0xFFFFFFFF;
    tss_desc->reserved = 0;
}

void gdt_init(void) {
    // 1. Setup GDTR
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;

    // 2. Setup Null Segment
    set_gdt_entry(0, 0, 0, 0, 0);

    // 3. Setup Kernel Segments (Ring 0)
    // Code: Execute/Read, DPL 0. Access = 0x9A. Flags = 0xA0 (Long Mode = 1, Size = 0)
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xA0); 
    // Data: Read/Write, DPL 0. Access = 0x92. Flags = 0xA0
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xA0); 

    // 4. Setup User Segments (Ring 3)
    // Note: The Syscall/Sysret instruction implicitly expects User Data to come BEFORE User Code.
    // Data: Read/Write, DPL 3. Access = 0xF2 (0x92 | 0x60). Flags = 0xA0
    set_gdt_entry(3, 0, 0xFFFFF, 0xF2, 0xA0); 
    // Code: Execute/Read, DPL 3. Access = 0xFA (0x9A | 0x60). Flags = 0xA0
    set_gdt_entry(4, 0, 0xFFFFF, 0xFA, 0xA0); 

    // 5. Setup TSS
    // Clear TSS
    for (uint32_t i = 0; i < sizeof(tss_t); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }
    
    // Set IOPB offset past the end of the TSS to prevent unprivileged IO
    tss.iopb_offset = sizeof(tss_t);
    
    set_tss_entry(5, (uint64_t)&tss, sizeof(tss_t) - 1);

    // 6. Flush the new GDT into the CPU and Load the Task Register (TR)
    gdt_flush((uint64_t)&gdtr);
    tss_flush();
}

void tss_set_kernel_stack(uint64_t stack_ptr) {
    tss.rsp0 = stack_ptr;
}
