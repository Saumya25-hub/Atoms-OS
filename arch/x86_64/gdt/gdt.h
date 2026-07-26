#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Standard GDT entry for 64-bit mode (8 bytes)
typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// TSS descriptor in 64-bit mode (16 bytes)
typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper;
  uint32_t reserved;
} __attribute__((packed)) tss_descriptor_t;

// Task State Segment for 64-bit mode
typedef struct {
  uint32_t reserved0;
  uint64_t rsp0; // The ring 0 stack pointer
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved1;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t iopb_offset;
  /* Software-owned BSP syscall entry state. This travels with the TSS and is
     intentionally outside the architectural 104-byte TSS limit. Phase 6 may
     replicate the containing GDT/TSS per CPU without changing entry ABI. */
  uint64_t syscall_user_rsp;
  uint64_t syscall_active_frame;
  uint32_t syscall_nesting;
  uint32_t syscall_reserved;
} __attribute__((packed)) tss_t;

#define TSS_ARCHITECTURAL_SIZE 104U
#define TSS_SYSCALL_USER_RSP_OFFSET 104U
#define TSS_SYSCALL_ACTIVE_FRAME_OFFSET 112U
#define TSS_SYSCALL_NESTING_OFFSET 120U

// GDTR
typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) gdtr_t;

// GDT Selectors
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_DATA 0x18
#define GDT_USER_CODE 0x20
#define GDT_TSS 0x28

// Important combinations
// User RPL = 3. Therefore, User Code selector is 0x20 | 3 = 0x23
// User Data selector is 0x18 | 3 = 0x1B
#define USER_CODE_SEGMENT (GDT_USER_CODE | 3)
#define USER_DATA_SEGMENT (GDT_USER_DATA | 3)

void gdt_init(void);
void gdt_init_cpu(uint32_t logical_id);
void tss_set_kernel_stack(uint64_t stack_ptr);
void tss_set_kernel_stack_cpu(uint32_t logical_id, uint64_t stack_ptr);
tss_t *gdt_get_tss_cpu(uint32_t logical_id);

#endif // GDT_H
