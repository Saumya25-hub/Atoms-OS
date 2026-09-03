#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/scheduler/include/scheduler.h"

bool syscall_validate_user_ptr(const void *ptr, size_t size) {
  if (!ptr || size == 0) {
    return false;
  }

  uintptr_t addr = (uintptr_t)ptr;

  /* Check address wrap/overflow */
  if (addr + size < addr) {
    return false;
  }

  /* Reject canonical violation */
  if (!vmm_address_canonical(addr) || !vmm_address_canonical(addr + size - 1)) {
    return false;
  }

  /* Reject NULL and addresses outside Usermode window [0x40000000, 0x80000000) */
  if (addr < USER_WINDOW_MIN || addr >= USER_WINDOW_MAX || (addr + size) > USER_WINDOW_MAX) {
    return false;
  }

  /* Reject Framebuffer (0x90000000+) and Kernel Space */
  if (addr >= 0x90000000ULL || (addr + size) >= 0x90000000ULL) {
    return false;
  }

  /* Query active hardware page table to verify all pages in range are mapped with PAGE_USER */
  Task *cur = scheduler_current_task();
  void *pml4 = (cur && cur->pml4) ? cur->pml4 : vmm_get_active_pml4();
  if (!pml4) {
    return false;
  }

  return vmm_validate_user_range(pml4, addr, size, VMM_ACCESS_READ);
}

bool syscall_validate_user_ptr_writable(const void *ptr, size_t size) {
  if (!ptr || size == 0) {
    return false;
  }

  uintptr_t addr = (uintptr_t)ptr;

  if (addr + size < addr) {
    return false;
  }

  if (!vmm_address_canonical(addr) || !vmm_address_canonical(addr + size - 1)) {
    return false;
  }

  if (addr < USER_WINDOW_MIN || addr >= USER_WINDOW_MAX || (addr + size) > USER_WINDOW_MAX) {
    return false;
  }

  if (addr >= 0x90000000ULL || (addr + size) >= 0x90000000ULL) {
    return false;
  }

  Task *cur = scheduler_current_task();
  void *pml4 = (cur && cur->pml4) ? cur->pml4 : vmm_get_active_pml4();
  if (!pml4) {
    return false;
  }

  return vmm_validate_user_range(pml4, addr, size, VMM_ACCESS_READ | VMM_ACCESS_WRITE);
}

bool syscall_validate_user_string(const char *str, size_t max_len) {
  if (!str || max_len == 0) return false;
  uintptr_t addr = (uintptr_t)str;

  if (!vmm_address_canonical(addr)) return false;
  if (addr < USER_WINDOW_MIN || addr >= USER_WINDOW_MAX) return false;

  Task *cur = scheduler_current_task();
  void *pml4 = (cur && cur->pml4) ? cur->pml4 : vmm_get_active_pml4();
  if (!pml4) return false;

  uint64_t last_checked_page = 0;
  bool page_is_valid = false;

  for (size_t i = 0; i < max_len; i++) {
    uintptr_t cur_addr = addr + i;
    if (cur_addr >= USER_WINDOW_MAX) return false;

    uint64_t page_base = cur_addr & ~0xFFFULL;
    if (i == 0 || page_base != last_checked_page) {
      last_checked_page = page_base;
      VMMPageInfo info;
      if (!vmm_query_page(pml4, page_base, &info) || !info.user || !info.present) {
        page_is_valid = false;
        return false; /* Stop before dereferencing unmapped memory */
      }
      page_is_valid = true;
    }

    if (!page_is_valid) return false;

    if (str[i] == '\0') {
      return true;
    }
  }

  return false; /* Exceeded max_len without finding null terminator */
}
