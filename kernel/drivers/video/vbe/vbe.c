#include "kernel/drivers/video/vbe/vbe.h"

#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/lib/include/crash_log.h"
#include <stddef.h>

static BVFramebuffer current_fb;
static uint32_t current_bpp;

// Double-buffer page-flip state
static uint32_t fb_page_height = 0;     // Height of one page
static uint32_t fb_current_page = 0;    // 0 or 1
static uint64_t fb_phys_base = 0;       // Physical base of VRAM
static uint64_t fb_page_size = 0;       // Bytes per page

// Bochs VGA Display Interface registers
#define VBE_DISPI_IOPORT_INDEX  0x01CE
#define VBE_DISPI_IOPORT_DATA   0x01CF
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x07
#define VBE_DISPI_INDEX_Y_OFFSET    0x09

static inline void bochs_vga_write(uint16_t index, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"((uint16_t)VBE_DISPI_IOPORT_INDEX));
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"((uint16_t)VBE_DISPI_IOPORT_DATA));
}

static inline void bochs_write_index(uint16_t index) {
    __asm__ volatile("outw %0, %1" : : "a"(index), "Nd"((uint16_t)VBE_DISPI_IOPORT_INDEX));
}

static inline void bochs_write_data(uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"((uint16_t)VBE_DISPI_IOPORT_DATA));
}

static inline uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void write_msr(uint32_t msr, uint64_t val) {
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32;
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

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
    
    fb_phys_base = boot_info->vbe_framebuffer;
    fb_page_height = current_fb.height;
    fb_page_size = (uint64_t)current_fb.height * current_fb.pitch;
    
    // Configure IA32_PAT MSR (0x277) to set PAT3 (bits 24-31) to Write-Combining (0x01)
    uint64_t pat = read_msr(0x277);
    pat = (pat & ~((uint64_t)0xFF << 24)) | ((uint64_t)0x01 << 24);
    write_msr(0x277, pat);

    // Map VRAM for TWO full pages (double buffer)
    uint64_t total_vram_size = fb_page_size * 2;
    uint64_t num_pages = (total_vram_size + 4095) / 4096;

    void* pml4 = vmm_get_active_pml4();
    if (!pml4) {
        crash_log_add("[VBE] FATAL: VMM PML4 is NULL");
        return;
    }
    
    // Identity map the full double-buffer VRAM region with Write-Combining enabled (PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLE -> PAT3)
    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t addr = fb_phys_base + (i * 4096);
        vmm_map_page(pml4, addr, addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLE);
    }
    
    // Tell Bochs VGA the virtual height is 2x physical (enables Y-offset paging)
    bochs_write_index(VBE_DISPI_INDEX_VIRT_HEIGHT);
    bochs_write_data((uint16_t)(fb_page_height * 2));
    
    // Start displaying page 0
    bochs_write_index(VBE_DISPI_INDEX_Y_OFFSET);
    bochs_write_data(0);
    fb_current_page = 0;
    
    // Point current_fb at page 0 (the display page)
    current_fb.buffer = (BOVISUAL_Color*)fb_phys_base;
    
    crash_log_add("[BOOT] VBE Driver Ready (Double-Buffer)");
}

BVFramebuffer* vbe_get_framebuffer(void) {
    return &current_fb;
}

// Get the BACK page (the one NOT currently displayed) for rendering
BVFramebuffer vbe_get_back_page(void) {
    BVFramebuffer back;
    back.width = current_fb.width;
    back.height = current_fb.height;
    back.pitch = current_fb.pitch;
    
    // Back page is the opposite of the currently displayed page
    uint32_t back_page = 1 - fb_current_page;
    back.buffer = (BOVISUAL_Color*)(fb_phys_base + (back_page * fb_page_size));
    
    return back;
}

// Atomically flip display to show the back page (ZERO tearing!)
void vbe_swap_page(void) {
    // The back page becomes the front page
    fb_current_page = 1 - fb_current_page;
    
    // Single register write atomically changes what the display controller reads
    uint16_t y_offset = (uint16_t)(fb_current_page * fb_page_height);
    bochs_write_index(VBE_DISPI_INDEX_Y_OFFSET);
    bochs_write_data(y_offset);
    
    // Update current_fb to point to the new front page
    current_fb.buffer = (BOVISUAL_Color*)(fb_phys_base + (fb_current_page * fb_page_size));
}
