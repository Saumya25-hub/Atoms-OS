#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/lib/include/crash_log.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/drivers/display/display.h"


// Map a user stack with an unmapped guard page immediately below it.
bool process_build_user_stack(ProcessImage *image, void *pml4) {
  if (!image || !pml4)
    return false;
  uint64_t stack_size = USER_STACK_PAGES * 4096;
  uint64_t stack_bottom = USER_STACK_TOP - stack_size;
  uint64_t guard_page = stack_bottom - 4096;

  if (!vmm_map_guard_page(pml4, guard_page))
    return false;
  for (int i = 0; i < USER_STACK_PAGES; i++) {
    uint64_t virt_addr = stack_bottom + (i * 4096);
    if (!vmm_map_user_page(pml4, virt_addr,
                           VMM_ACCESS_READ | VMM_ACCESS_WRITE)) {
      while (i-- > 0)
        vmm_free_mapped_page(pml4, stack_bottom + (i * 4096));
      display_print("[FAIL] Failed to allocate protected user stack page\n");
      return false;
    }
  }

  image->stack_bottom = stack_bottom;

  // Setting up argc=0, argv=NULL, envp=NULL for now
  // A proper implementation would write these to the top of the stack and
  // adjust stack_top downwards
  image->stack_top = USER_STACK_TOP - 8;

  // Increment page count in the process image
  image->page_count += USER_STACK_PAGES;

  crash_log_add("[ELF] Executable Stack Mapped");

  return true;
}
