#include "kernel/core/syscall/include/syscall.h"

bool syscall_validate_user_ptr(const void *ptr, size_t size) {
  if (!ptr || size == 0) {
    return false;
  }

  uintptr_t addr = (uintptr_t)ptr;

  /* Check address wrap/overflow */
  if (addr + size < addr) {
    return false;
  }

  /* Reject NULL and addresses below Usermode window min (0x40000000) */
  if (addr < USER_WINDOW_MIN) {
    return false;
  }

  /* Reject addresses exceeding Usermode window max (0x80000000) */
  if (addr >= USER_WINDOW_MAX || (addr + size) > USER_WINDOW_MAX) {
    return false;
  }

  /* Explicitly reject Framebuffer (0x90000000+) and Kernel Heap / Kernel Space (0xC0000000+) */
  if (addr >= 0x90000000ULL || (addr + size) >= 0x90000000ULL) {
    return false;
  }

  return true;
}
