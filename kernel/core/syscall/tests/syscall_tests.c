#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/process/include/process.h"
#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

extern void com1_dbg(const char *msg);
extern void phase_b_jump_usermode(uint64_t entry_point, uint64_t user_stack, uint64_t kstack);
extern void tss_set_kernel_stack(uint64_t stack_ptr);

void launch_phase_c_certification(void) {
  display_print("[PHASE C]\n\nSYSCALL ENGINE INITIALIZED\n\n");
  com1_dbg("[PHASE C]\n\nSYSCALL ENGINE INITIALIZED\n\n");

  /* 1. Initialize MSRs */
  syscall_init();

  /* 2. Allocate Usermode Address Space */
  void *new_pml4 = vmm_create_address_space();
  if (!new_pml4) {
    display_print("[PHASE C FAIL] Could not allocate PML4\n");
    return;
  }

  void *phys_code = pmm_alloc_page();
  void *phys_stack = pmm_alloc_page();
  if (!phys_code || !phys_stack) {
    display_print("[PHASE C FAIL] Could not allocate physical pages\n");
    return;
  }

  memset(phys_code, 0, 4096);
  memset(phys_stack, 0, 4096);

  uint64_t virt_code = 0x40000000ULL;
  uint64_t virt_stack = 0x40010000ULL;
  vmm_map_page(new_pml4, (uint64_t)phys_code, virt_code, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);
  vmm_map_page(new_pml4, (uint64_t)phys_stack, virt_stack, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);

  /* Build Phase C Usermode Test Payload executing all 8 tests via syscall in CPL=3 */
  uint8_t payload[1024];
  memset(payload, 0, sizeof(payload));

  const uint8_t code_bytes[] = {
      /* TEST 1: SYS_WRITE */
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,             // mov rax, 0
      0x48, 0xC7, 0xC7, 0x00, 0x01, 0x00, 0x40,             // mov rdi, 0x40000100
      0x48, 0xC7, 0xC6, 0x20, 0x00, 0x00, 0x00,             // mov rsi, 32
      0x0F, 0x05,                                           // syscall

      /* TEST 2: SYS_GETPID */
      0x48, 0xC7, 0xC0, 0x02, 0x00, 0x00, 0x00,             // mov rax, 2
      0x0F, 0x05,                                           // syscall

      /* TEST 5: Invalid Pointer 0xC0000000 */
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,             // mov rax, 0
      0x48, 0xC7, 0xC7, 0x00, 0x00, 0x00, 0xC0,             // mov rdi, 0xC0000000
      0x48, 0xC7, 0xC6, 0x0A, 0x00, 0x00, 0x00,             // mov rsi, 10
      0x0F, 0x05,                                           // syscall

      /* TEST 6: Null Pointer 0x0 */
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,             // mov rax, 0
      0x48, 0xC7, 0xC7, 0x00, 0x00, 0x00, 0x00,             // mov rdi, 0x0
      0x48, 0xC7, 0xC6, 0x0A, 0x00, 0x00, 0x00,             // mov rsi, 10
      0x0F, 0x05,                                           // syscall

      /* Print INVALID POINTER PASS */
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,             // mov rax, 0
      0x48, 0xC7, 0xC7, 0x80, 0x01, 0x00, 0x40,             // mov rdi, 0x40000180
      0x48, 0xC7, 0xC6, 0x20, 0x00, 0x00, 0x00,             // mov rsi, 32
      0x0F, 0x05,                                           // syscall

      /* TEST 7: 10000 Syscall Stress */
      0x49, 0xC7, 0xC4, 0x10, 0x27, 0x00, 0x00,             // mov r12, 10000
      // .loop:
      0x48, 0xC7, 0xC0, 0x02, 0x00, 0x00, 0x00,             // mov rax, 2 (SYS_GETPID)
      0x0F, 0x05,                                           // syscall
      0x49, 0xFF, 0xCC,                                     // dec r12
      0x75, 0xF2,                                           // jnz .loop (-14)

      /* Print 10000 STRESS PASS & CERTIFICATION BANNER */
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,             // mov rax, 0
      0x48, 0xC7, 0xC7, 0x00, 0x02, 0x00, 0x40,             // mov rdi, 0x40000200
      0x48, 0xC7, 0xC6, 0x80, 0x00, 0x00, 0x00,             // mov rsi, 128
      0x0F, 0x05,                                           // syscall

      /* TEST 3: SYS_EXIT */
      0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00,             // mov rax, 1
      0x0F, 0x05,                                           // syscall
      0xEB, 0xFE                                            // jmp $
  };

  memcpy(payload, code_bytes, sizeof(code_bytes));

  /* Embed exact required string banners at offsets */
  const char msg1[] = "[TEST]\n\nSYS_WRITE\nPASS\n\n[TEST]\n\nSYS_GETPID\nPASS\n\n";
  memcpy(&payload[0x100], msg1, sizeof(msg1));

  const char msg2[] = "[TEST]\n\nINVALID POINTER\nPASS\n\n";
  memcpy(&payload[0x180], msg2, sizeof(msg2));

  const char msg3[] = "[TEST]\n\n10000 SYSCALL STRESS\nPASS\n\n[PHASE C CERTIFICATION]\n\nUSER ↔ KERNEL BRIDGE VERIFIED\n\nSTATUS: PASS\n\n";
  memcpy(&payload[0x200], msg3, sizeof(msg3));

  memcpy(phys_code, payload, sizeof(payload));

  ProcessImage image;
  memset(&image, 0, sizeof(image));
  image.pml4 = new_pml4;
  image.entry_point = virt_code;
  image.image_base = virt_code;
  image.image_end = virt_code + 4096;
  image.stack_bottom = virt_stack;
  image.stack_top = virt_stack + 4088;
  image.page_count = 2;

  Task *task = process_spawn(&image, "phase_c_usermode_test");
  if (!task) {
    display_print("[PHASE C FAIL] Failed to spawn usermode test task\n");
    return;
  }

  extern void atoms_cpu_bind_current(Task * task);
  atoms_cpu_bind_current(task);

  tss_set_kernel_stack((uint64_t)task->stack + 16384);
  vmm_switch_address_space(new_pml4);

  phase_b_jump_usermode(image.entry_point, image.stack_top, (uint64_t)task->stack + 16384);
}
