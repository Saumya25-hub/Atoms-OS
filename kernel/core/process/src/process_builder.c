#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/crash_log.h"

// Sprint 6: Map a user stack at 0x00007FFFFFFFE000
bool process_build_user_stack(ProcessImage* image, void* pml4) {
    uint64_t stack_size = USER_STACK_PAGES * 4096;
    uint64_t stack_bottom = USER_STACK_TOP - stack_size;
    
    for (int i = 0; i < USER_STACK_PAGES; i++) {
        uint64_t virt_addr = stack_bottom + (i * 4096);
        // Flags: 0x7 -> Present (1) | Writable (2) | User Access (4)
        void* mapped = vmm_alloc_mapped_page(pml4, virt_addr, 0x07);
        if (!mapped) {
            display_print("[FAIL] Failed to allocate user stack page\n");
            return false;
        }
    }
    
    image->stack_bottom = stack_bottom;
    
    // Setting up argc=0, argv=NULL, envp=NULL for now
    // A proper implementation would write these to the top of the stack and adjust stack_top downwards
    image->stack_top = USER_STACK_TOP;
    
    
    // Increment page count in the process image
    image->page_count += USER_STACK_PAGES;
    
    crash_log_add("[ELF] Executable Stack Mapped");
    
    return true;
}
