#include "kernel/drivers/video/vbe/vbe.h"

#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/lib/include/crash_log.h"
#include <stddef.h>

static BVFramebuffer current_fb;
static uint32_t current_bpp;

void vbe_init(boot_info_t* boot_info) {
    if (!boot_info) {
        crash_log_add("[VBE] FATAL: boot_info is NULL");
        return;
    }

    if (boot_info->vbe_width == 0 || boot_info->vbe_framebuffer == 0) {
        crash_log_add("[VBE] FATAL: VBE not initialized by bootloader");
        return;
    }

    current_fb.width = boot_info->vbe_width;
    current_fb.height = boot_info->vbe_height;
    current_fb.pitch = boot_info->vbe_pitch;
    current_bpp = boot_info->vbe_bpp;
    
    uint64_t fb_phys = boot_info->vbe_framebuffer;
    uint64_t fb_size = (current_fb.height * current_fb.pitch);
    uint64_t num_pages = (fb_size + 4095) / 4096; // PAGE_SIZE is 4096

    void* pml4 = vmm_get_active_pml4();
    if (!pml4) {
        crash_log_add("[VBE] FATAL: VMM PML4 is NULL");
        return;
    }
    
    // Identity map the framebuffer
    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t addr = fb_phys + (i * 4096);
        vmm_map_page(pml4, addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    current_fb.buffer = (BOVISUAL_Color*)fb_phys;
    
    crash_log_add("[BOOT] VBE Driver Ready");
}

BVFramebuffer* vbe_get_framebuffer(void) {
    return &current_fb;
}
