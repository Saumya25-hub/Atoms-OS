#include "kernel/drivers/video/vbe/vbe.h"
#include "kernel/graphics/gpu/include/gpu.h"

#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/crash_log.h"
#include <stddef.h>

static BVFramebuffer current_fb;
static uint32_t current_bpp;

// Double-buffer page-flip state
static uint32_t fb_page_height = 0;     // Height of one page
static uint32_t fb_current_page = 0;    // 0 or 1
static uint64_t fb_phys_base = 0;       // Physical base of VRAM
static uint64_t fb_page_size = 0;       // Bytes per page
static BVFramebuffer s_pages[2];

// Bochs VGA Display Interface registers
#define VBE_DISPI_IOPORT_INDEX  0x01CE
#define VBE_DISPI_IOPORT_DATA   0x01CF
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x07
#define VBE_DISPI_INDEX_Y_OFFSET    0x09

static inline void bochs_vga_write(uint16_t index, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(index), "Nd"((uint16_t)VBE_DISPI_IOPORT_INDEX));
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

    current_fb.width  = boot_info->vbe_width;
    current_fb.height = boot_info->vbe_height;
    current_fb.pitch  = boot_info->vbe_pitch;
    current_bpp       = boot_info->vbe_bpp;

    fb_phys_base    = boot_info->vbe_framebuffer;
    fb_page_height  = current_fb.height;
    fb_page_size    = (uint64_t)current_fb.height * current_fb.pitch;

    // In UEFI/GOP mode the framebuffer is already identity-mapped by our 4GB
    // kernel page table (vmm_init built a 0-4GB RWX identity map).
    // DO NOT re-map individual pages here — that would split the 2MB huge pages
    // and could corrupt the mapping.
    // DO NOT write Bochs VBE I/O registers — OVMF already configured the display
    // controller; touching VBE_DISPI_INDEX_VIRT_HEIGHT breaks the GOP framebuffer.

    // Point current_fb at page 0 (the display page) — write directly to VRAM.
    current_fb.buffer = (BOVISUAL_Color*)fb_phys_base;

    console_set_backend(vbe_get_console_backend());
    console_clear_all(0x00);   // Paints entire VRAM black — confirms framebuffer works.

    crash_log_add("[BOOT] VBE Driver Ready (GOP Direct)");
}

BVFramebuffer* vbe_get_framebuffer(void) {
    return &current_fb;
}

BVFramebuffer vbe_get_back_page(void) {
    BVFramebuffer back;
    back.width = current_fb.width;
    back.height = current_fb.height;
    back.pitch = current_fb.pitch;
    
    uint32_t back_page = 1 - fb_current_page;
    back.buffer = (BOVISUAL_Color*)(fb_phys_base + (back_page * fb_page_size));
    
    return back;
}

BVFramebuffer* vbe_get_back_page_ptr(void) {
    extern bos_gpu_device_t* bos_gpu_get_primary(void);
    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    bool is_vmware = (gpu && gpu->vendor_id == 0x15AD);

    uint32_t back_page = 0;
    
    s_pages[back_page].width = current_fb.width;
    s_pages[back_page].height = current_fb.height;
    s_pages[back_page].pitch = current_fb.pitch;
    s_pages[back_page].buffer = (BOVISUAL_Color*)(fb_phys_base + (back_page * fb_page_size));
    
    return &s_pages[back_page];
}

BVFramebuffer* vbe_get_front_page_ptr(void) {
    uint32_t front_page = fb_current_page;
    
    s_pages[front_page].width = current_fb.width;
    s_pages[front_page].height = current_fb.height;
    s_pages[front_page].pitch = current_fb.pitch;
    s_pages[front_page].buffer = (BOVISUAL_Color*)(fb_phys_base + (front_page * fb_page_size));
    
    return &s_pages[front_page];
}

void vbe_swap_page(void) {
    extern bos_gpu_device_t* bos_gpu_get_primary(void);
    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    bool is_vmware = (gpu && gpu->vendor_id == 0x15AD);

    if (is_vmware && gpu->ops && gpu->ops->present) {
        bos_gpu_surface_t surf;
        surf.width = current_fb.width;
        surf.height = current_fb.height;
        surf.pitch = current_fb.pitch;
        surf.virt_addr = (void*)fb_phys_base;
        gpu->ops->present(gpu, &surf);
        return;
    }

    fb_current_page = 1 - fb_current_page;
    
    uint16_t y_offset = (uint16_t)(fb_current_page * fb_page_height);
    bochs_write_index(VBE_DISPI_INDEX_Y_OFFSET);
    bochs_write_data(y_offset);
    
    current_fb.buffer = (BOVISUAL_Color*)(fb_phys_base + (fb_current_page * fb_page_size));
}

#include "bovisual/Text/font8x16.h"

static void vbe_console_init(void) {
}

static uint16_t vbe_console_get_width(void) {
    return current_fb.width ? (current_fb.width / 8) : 80;
}

static uint16_t vbe_console_get_height(void) {
    return current_fb.height ? (current_fb.height / 16) : 25;
}

static void vbe_console_clear_memory(uint8_t bg_color) {
    if (!current_fb.buffer || current_fb.width == 0 || current_fb.height == 0) return;
    uint32_t color = (bg_color == 0) ? 0xFF000000 : 0xFF111111;
    uint32_t count = current_fb.width * current_fb.height;
    uint32_t* buf = (uint32_t*)current_fb.buffer;
    for (uint32_t i = 0; i < count; i++) {
        buf[i] = color;
    }
}

static void vbe_console_draw_character(uint16_t x, uint16_t y, char c, uint8_t color) {
    if (!current_fb.buffer || current_fb.width == 0 || current_fb.height == 0) return;
    uint32_t px = (uint32_t)x * 8;
    uint32_t py = (uint32_t)y * 16;
    if (px + 8 > current_fb.width || py + 16 > current_fb.height) return;

    uint32_t fg = 0xFFFFFFFF; // White default
    if (color == 0x0C) fg = 0xFFFF5555; // Light Red
    else if (color == 0x0A) fg = 0xFF55FF55; // Light Green
    else if (color == 0x0E) fg = 0xFFFFFF55; // Yellow
    else if (color == 0x09) fg = 0xFF5555FF; // Light Blue
    else if (color == 0x0D) fg = 0xFFFF55FF; // Magenta
    else if (color == 0x0B) fg = 0xFF55FFFF; // Light Cyan

    const uint8_t* glyph = g_font8x16_stub[(uint8_t)c];
    for (int row = 0; row < 16; row++) {
        uint8_t row_data = glyph[row];
        uint32_t* row_ptr = (uint32_t*)((uint8_t*)current_fb.buffer + (py + row) * current_fb.pitch);
        for (int col = 0; col < 8; col++) {
            if (row_data & (1 << col)) {
                row_ptr[px + col] = fg;
            } else {
                row_ptr[px + col] = 0xFF000000;
            }
        }
    }
}

static void vbe_console_set_hardware_cursor(uint16_t x, uint16_t y) {
    (void)x; (void)y;
}

static BackendDriver vbe_console_backend = {
    .init = vbe_console_init,
    .draw_character = vbe_console_draw_character,
    .set_hardware_cursor = vbe_console_set_hardware_cursor,
    .clear_memory = vbe_console_clear_memory,
    .get_width = vbe_console_get_width,
    .get_height = vbe_console_get_height
};

BackendDriver* vbe_get_console_backend(void) {
    return &vbe_console_backend;
}
