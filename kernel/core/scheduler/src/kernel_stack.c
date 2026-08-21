#include "../include/kernel_stack.h"
#include "../include/task.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/lib/include/string.h"

void *kernel_stack_alloc(size_t size) {
  if (size == 0) return NULL;
  size_t page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  if (page_count == 0) page_count = 1;

  void *stack = pmm_alloc_pages(page_count);
  if (!stack) return NULL;

  memset(stack, 0, page_count * PAGE_SIZE);

  // Plant 64-bit hardware stack canary at bottom-most boundary
  *(uint64_t *)stack = STACK_CANARY_BOTTOM_MAGIC;

  return stack;
}

void kernel_stack_free(void *stack, size_t size) {
  if (!stack || size == 0) return;
  size_t page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  if (page_count == 0) page_count = 1;

  // Clear bottom canary before free
  *(uint64_t *)stack = 0;
  pmm_free_pages(stack, page_count);
}

bool kernel_stack_validate(const struct Task *task) {
  if (!task || !task->stack) return false;

  // Verify bottom boundary canary
  if (*(const uint64_t *)task->stack != STACK_CANARY_BOTTOM_MAGIC) {
    return false;
  }

  // Verify RSP resides within allocated stack envelope
  uint64_t stack_base = (uint64_t)task->stack;
  uint64_t stack_top  = stack_base + KERNEL_TASK_STACK_SIZE;

  if (task->rsp < stack_base + 8 || task->rsp > stack_top) {
    return false;
  }

  return true;
}
