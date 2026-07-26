#include "gdt.h"
#include <stddef.h>

// Extern functions defined in gdt_flush.asm
extern void gdt_flush(uint64_t gdtr_ptr);
extern void tss_flush(void);

// 5 standard entries + 1 TSS (which takes 2 entries in 64-bit mode)
// Wait! In 64-bit mode, the TSS descriptor is 16 bytes. A standard GDT entry is
// 8 bytes. So the total size of the GDT is: Null (1) + KCode (1) + KData (1) +
// UData (1) + UCode (1) + TSS (2) = 7 entries.
#define ATOMS_MAX_CPUS_GDT 8U

static gdt_entry_t gdt_cpus[ATOMS_MAX_CPUS_GDT][7];
static gdtr_t gdtr_cpus[ATOMS_MAX_CPUS_GDT];
tss_t tss_cpus[ATOMS_MAX_CPUS_GDT];
tss_t tss;

static void set_gdt_entry_cpu(uint32_t cpu, int num, uint32_t base, uint32_t limit,
                              uint8_t access, uint8_t gran) {
  gdt_cpus[cpu][num].base_low = (base & 0xFFFF);
  gdt_cpus[cpu][num].base_middle = (base >> 16) & 0xFF;
  gdt_cpus[cpu][num].base_high = (base >> 24) & 0xFF;

  gdt_cpus[cpu][num].limit_low = (limit & 0xFFFF);
  gdt_cpus[cpu][num].granularity = ((limit >> 16) & 0x0F);
  gdt_cpus[cpu][num].granularity |= (gran & 0xF0);
  gdt_cpus[cpu][num].access = access;
}

static void set_tss_entry_cpu(uint32_t cpu, int num, uint64_t base, uint32_t limit) {
  tss_descriptor_t *tss_desc = (tss_descriptor_t *)&gdt_cpus[cpu][num];

  tss_desc->limit_low = limit & 0xFFFF;
  tss_desc->base_low = base & 0xFFFF;
  tss_desc->base_middle = (base >> 16) & 0xFF;
  tss_desc->access = 0x89;
  tss_desc->granularity = ((limit >> 16) & 0x0F) | 0x00;
  tss_desc->base_high = (base >> 24) & 0xFF;
  tss_desc->base_upper = (base >> 32) & 0xFFFFFFFF;
  tss_desc->reserved = 0;
}

void gdt_init_cpu(uint32_t logical_id) {
  if (logical_id >= ATOMS_MAX_CPUS_GDT)
    return;

  gdtr_cpus[logical_id].limit = sizeof(gdt_cpus[logical_id]) - 1;
  gdtr_cpus[logical_id].base = (uint64_t)&gdt_cpus[logical_id];

  set_gdt_entry_cpu(logical_id, 0, 0, 0, 0, 0);
  set_gdt_entry_cpu(logical_id, 1, 0, 0xFFFFF, 0x9A, 0xA0);
  set_gdt_entry_cpu(logical_id, 2, 0, 0xFFFFF, 0x92, 0xA0);
  set_gdt_entry_cpu(logical_id, 3, 0, 0xFFFFF, 0xF2, 0xA0);
  set_gdt_entry_cpu(logical_id, 4, 0, 0xFFFFF, 0xFA, 0xA0);

  tss_t *target_tss = &tss_cpus[logical_id];
  for (uint32_t i = 0; i < sizeof(tss_t); i++) {
    ((uint8_t *)target_tss)[i] = 0;
  }
  target_tss->iopb_offset = TSS_ARCHITECTURAL_SIZE;
  set_tss_entry_cpu(logical_id, 5, (uint64_t)target_tss, TSS_ARCHITECTURAL_SIZE - 1);

  if (logical_id == 0) {
    tss = *target_tss;
  }

  gdt_flush((uint64_t)&gdtr_cpus[logical_id]);
  tss_flush();
}

void gdt_init(void) {
  gdt_init_cpu(0);
}

void tss_set_kernel_stack_cpu(uint32_t logical_id, uint64_t stack_ptr) {
  if (logical_id < ATOMS_MAX_CPUS_GDT) {
    tss_cpus[logical_id].rsp0 = stack_ptr;
    if (logical_id == 0) {
      tss.rsp0 = stack_ptr;
    }
  }
}

void tss_set_kernel_stack(uint64_t stack_ptr) {
  tss_set_kernel_stack_cpu(0, stack_ptr);
}

tss_t *gdt_get_tss_cpu(uint32_t logical_id) {
  if (logical_id < ATOMS_MAX_CPUS_GDT) {
    return &tss_cpus[logical_id];
  }
  return &tss_cpus[0];
}

