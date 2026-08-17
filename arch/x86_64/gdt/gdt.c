#include "gdt.h"
#include "kernel/debug/abde/abde.h"
#include <stddef.h>

// Extern functions defined in gdt_flush.asm
extern void gdt_flush(uint64_t gdtr_ptr);
extern void tss_flush(void);

#define ATOMS_MAX_CPUS_GDT 8U

static gdt_entry_t gdt_cpus[ATOMS_MAX_CPUS_GDT][7] __attribute__((aligned(16)));
static gdtr_t gdtr_cpus[ATOMS_MAX_CPUS_GDT] __attribute__((aligned(16)));
tss_t tss_cpus[ATOMS_MAX_CPUS_GDT] __attribute__((aligned(16)));
tss_t tss __attribute__((aligned(16)));

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
  tss_desc->access = 0x89; // Present, 64-bit Available TSS
  tss_desc->granularity = ((limit >> 16) & 0x0F);
  tss_desc->base_high = (base >> 24) & 0xFF;
  tss_desc->base_upper = (base >> 32) & 0xFFFFFFFF;
  tss_desc->reserved = 0;
}

void gdt_init_cpu(uint32_t logical_id) {
  if (logical_id >= ATOMS_MAX_CPUS_GDT)
    return;

  // Step 1: Build GDT Segment Entries
  if (logical_id == 0) diag_set_step("BUILD GDT ENTRIES");
  set_gdt_entry_cpu(logical_id, 0, 0, 0, 0, 0);
  set_gdt_entry_cpu(logical_id, 1, 0, 0xFFFFF, 0x9A, 0xA0); // 64-bit Kernel Code (L=1, G=1)
  set_gdt_entry_cpu(logical_id, 2, 0, 0xFFFFF, 0x92, 0xC0); // Kernel Data (L=0, D/B=1, G=1)
  set_gdt_entry_cpu(logical_id, 3, 0, 0xFFFFF, 0xF2, 0xC0); // User Data (L=0, D/B=1, G=1)
  set_gdt_entry_cpu(logical_id, 4, 0, 0xFFFFF, 0xFA, 0xA0); // 64-bit User Code (L=1, G=1)

  // Step 2: Build TSS Descriptor
  if (logical_id == 0) diag_set_step("BUILD TSS DESCRIPTOR");
  tss_t *target_tss = &tss_cpus[logical_id];
  for (uint32_t i = 0; i < sizeof(tss_t); i++) {
    ((uint8_t *)target_tss)[i] = 0;
  }
  target_tss->iopb_offset = TSS_ARCHITECTURAL_SIZE;
  set_tss_entry_cpu(logical_id, 5, (uint64_t)target_tss, TSS_ARCHITECTURAL_SIZE - 1);

  if (logical_id == 0) {
    tss = *target_tss;
  }

  // Step 3: Setup GDTR Structure
  if (logical_id == 0) diag_set_step("SETUP GDTR");
  gdtr_cpus[logical_id].limit = sizeof(gdt_cpus[logical_id]) - 1;
  gdtr_cpus[logical_id].base = (uint64_t)&gdt_cpus[logical_id];

  // Step 4: Execute LGDT and Segment Reload
  if (logical_id == 0) diag_set_step("BEFORE LGDT");
  gdt_flush((uint64_t)&gdtr_cpus[logical_id]);
  if (logical_id == 0) diag_set_step("AFTER LGDT");

  if (logical_id == 0) diag_set_step("BEFORE SEGMENT RELOAD");
  // Segment reload completed cleanly in gdt_flush
  if (logical_id == 0) diag_set_step("AFTER SEGMENT RELOAD");

  // Step 5: Execute LTR (Task Register Load)
  if (logical_id == 0) diag_set_step("BEFORE LTR");
  tss_flush();
  if (logical_id == 0) diag_set_step("AFTER LTR");
}

void gdt_init(void) {
  diag_set_step("ENTER GDT_INIT");
  gdt_init_cpu(0);
  diag_set_step("GDT CERTIFIED");
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
