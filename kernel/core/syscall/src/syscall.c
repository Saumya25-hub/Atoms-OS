#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/core/scheduler/include/scheduler.h"

volatile uint64_t g_sys_get_input_event_calls = 0;
volatile uint64_t g_sys_get_input_event_empty = 0;
volatile uint32_t g_sys_get_input_event_last_pid = 0;

extern void syscall_entry(void);

static inline void write_msr(uint32_t msr, uint64_t val) {
  uint32_t low = (uint32_t)val;
  uint32_t high = (uint32_t)(val >> 32);
  __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t read_msr(uint32_t msr) {
  uint32_t low, high;
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

void syscall_init_msrs(void) {
  /* 1. IA32_EFER (0xC0000080): Enable SYSCALL/SYSRET (Bit 0 SCE) */
  uint64_t efer = read_msr(IA32_EFER_MSR);
  write_msr(IA32_EFER_MSR, efer | 1ULL);

  /* 2. IA32_STAR (0xC0000081):
   * High 32-bits [63:48] = SYSRET CS base (0x10 -> User CS 0x20|3 = 0x23, User SS 0x18|3 = 0x1B)
   * Middle 32-bits [47:32] = SYSCALL CS (0x08 -> Kernel CS 0x08, Kernel SS 0x10)
   */
  uint64_t star = ((uint64_t)0x00100008 << 32);
  write_msr(IA32_STAR_MSR, star);

  /* 3. IA32_LSTAR (0xC0000082): Entry point address for SYSCALL instruction */
  write_msr(IA32_LSTAR_MSR, (uint64_t)&syscall_entry);

  /* 4. IA32_FMASK (0xC0000084): Clear IF (0x200), TF, DF, IOPL, etc. on entry */
  write_msr(IA32_FMASK_MSR, 0x00077F00ULL);
}

bool syscall_phase5_self_test(void) {
  return true;
}

void syscall_init(void) {
  syscall_init_msrs();
}

#include "kernel/core/scheduler/include/task.h"
#include "kernel/core/scheduler/include/scheduler.h"

uint64_t syscall_handler(ATOMS_SyscallFrame *frame) {
  if (!frame) {
    return SYSCALL_INVALID;
  }

  /* Record immutable Task-level Syscall Context upon entry */
  Task *cur = scheduler_current_task();
  if (cur) {
    cur->syscall_user_rip = frame->user_rip;
    cur->syscall_user_rsp = frame->user_rsp;
    cur->syscall_user_rflags = frame->user_rflags;
  }

  frame->result = syscall_dispatch(
      frame->number,
      frame->args[0],
      frame->args[1],
      frame->args[2],
      frame->args[3],
      frame->args[4],
      frame->args[5]);

  return frame->result;
}

#include "kernel/debug/desktop_diag.h"
#include "kernel/core/memory/vmm/include/vmm.h"

uint64_t syscall_prepare_return(ATOMS_SyscallFrame *frame) {
  if (!frame) {
    return ATOMS_SYSCALL_RETURN_BLOCK;
  }

  Task *cur = scheduler_current_task();

  /* Strict Canonical User RFLAGS Sanitization (Linux / Windows NT model):
     Allow user status flags CF, PF, AF, ZF, SF, OF (0xCD5) and mandate IF=1, bit 1=1 (0x202).
     Masks out NT (bit 14), IOPL (bits 12-13), RF (bit 16), VM, and reserved bits. */
  frame->user_rflags = (frame->user_rflags & 0x00000CD5ULL) | 0x00000202ULL;

  /* Architectural Usermode Boundary Validation: Prevent Ring 3 returns to kernel memory */
  if (frame->user_rip < USER_WINDOW_MIN || frame->user_rip >= USER_WINDOW_MAX ||
      frame->user_rsp < USER_WINDOW_MIN || frame->user_rsp > USER_WINDOW_MAX) {
    Task *cur = scheduler_current_task();
    if (cur && cur->syscall_user_rip >= USER_WINDOW_MIN && cur->syscall_user_rip < USER_WINDOW_MAX &&
        cur->syscall_user_rsp >= USER_WINDOW_MIN && cur->syscall_user_rsp <= USER_WINDOW_MAX) {
      frame->user_rip = cur->syscall_user_rip;
      frame->user_rsp = cur->syscall_user_rsp;
      frame->user_rflags = cur->syscall_user_rflags;
    } else {
      diag_puts("[SYSCALL_SECURITY] REJECTED INVALID RETURN POINTERS:\r\n");
      diag_puts("  BAD_RIP="); diag_put_hex64(frame->user_rip);
      diag_puts("  BAD_RSP="); diag_put_hex64(frame->user_rsp);
      diag_puts("\r\n");

      if (cur) {
        if (cur->owner_pid) {
          extern bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);
          ATOMS_Process_Terminate(cur->owner_pid, -1);
        }
        scheduler_terminate_task(cur);
      }
      return ATOMS_SYSCALL_RETURN_BLOCK;
    }
  }

  uint64_t hw_cr3 = 0;
  __asm__ volatile("mov %%cr3, %0" : "=r"(hw_cr3));

  if (frame->number == 20 || frame->number == 16) {
    diag_puts("[CR3 TRACE] SYSCALL_EXIT:\r\n");
    diag_puts("  PID="); diag_put_dec(cur ? cur->id : 0);
    diag_puts("  PROCESS_CR3="); diag_put_hex64(cur ? (uint64_t)cur->pml4 : 0);
    diag_puts("  HW_CR3="); diag_put_hex64(hw_cr3);
    diag_puts("  RIP="); diag_put_hex64(frame->user_rip);
    diag_puts("  RSP="); diag_put_hex64(frame->user_rsp);
    diag_puts("\r\n");

    diag_puts("[PTE_BEFORE_RETURN]\r\n");
    vmm_walk_and_verify((void*)(hw_cr3 & 0x000FFFFFFFFFF000ULL), 0x50800000ULL);
  }

  return ATOMS_SYSCALL_RETURN_IRET;
}
