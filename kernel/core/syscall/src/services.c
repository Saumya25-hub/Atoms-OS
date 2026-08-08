#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/lib/include/string.h"

extern void com1_dbg(const char *msg);

uint64_t sys_service_write(const char *user_str, size_t len) {
  if (!syscall_validate_user_ptr(user_str, len > 0 ? len : 1)) {
    return SYSCALL_BAD_ADDRESS;
  }

  char buf[256];
  size_t copy_len = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
  memcpy(buf, user_str, copy_len);
  buf[copy_len] = '\0';

  display_print(buf);
  com1_dbg(buf);
  return SYSCALL_OK;
}

uint64_t sys_service_exit(int code) {
  (void)code;
  Task *current = scheduler_current_task();
  if (current && current != scheduler_get_idle_task()) {
    scheduler_terminate_task(current);
  }
  return SYSCALL_OK;
}

uint64_t sys_service_getpid(void) {
  Task *current = scheduler_current_task();
  return current ? current->id : 1;
}

uint64_t sys_service_yield(void) {
  scheduler_yield();
  return SYSCALL_OK;
}

uint64_t sys_service_uptime(void) {
  return timer_get_ticks();
}

uint64_t sys_service_alloc(size_t size) {
  if (size == 0 || size > 4096 * 16) {
    return SYSCALL_FAIL;
  }

  Task *current = scheduler_current_task();
  if (!current || !current->pml4) {
    return SYSCALL_FAIL;
  }

  void *phys = pmm_alloc_page();
  if (!phys) {
    return SYSCALL_FAIL;
  }
  memset(phys, 0, 4096);

  static uint64_t s_alloc_addr = 0x40020000ULL;
  uint64_t virt = s_alloc_addr;
  s_alloc_addr += 4096;
  if (s_alloc_addr >= 0x7FFF0000ULL) {
    s_alloc_addr = 0x40020000ULL;
  }

  vmm_map_page(current->pml4, (uint64_t)phys, virt, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);
  return virt;
}

uint64_t sys_service_free(void *ptr) {
  if (!syscall_validate_user_ptr(ptr, 1)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return SYSCALL_OK;
}

uint64_t sys_service_debug_print(const char *msg) {
  if (!syscall_validate_user_ptr(msg, 1)) {
    return SYSCALL_BAD_ADDRESS;
  }

  display_print(msg);
  com1_dbg(msg);
  return SYSCALL_OK;
}
