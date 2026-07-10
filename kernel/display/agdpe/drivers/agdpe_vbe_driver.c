/**
 * @file agdpe_vbe_driver.c
 * @brief AGDPE VBE Fallback Driver Wrapper
 */

#include "../agdpe.h"
#include "kernel/drivers/video/vbe/vbe.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/lib/include/string.h"

// External diagnostics
extern void display_print(const char* str);

static void vbe_drv_swap_buffers(AGDPE_DisplayDevice* dev, const BVFramebuffer* backbuffer) {
    if (!dev || !backbuffer) return;
    
    // We could use vbe_swap_page() if we wanted hardware flipping,
    // but the BSPE engine currently handles software copying via BOVISUAL_Graphics_SwapBuffers.
    // For AGDPE, we will just delegate to the legacy BVFramebuffer.
    // Wait, BSPE does memory copy. If the driver is asked to swap buffers,
    // the driver should copy the backbuffer to the frontbuffer.
    
    // Copy the whole buffer
    uint32_t size = backbuffer->pitch * backbuffer->height;
    memcpy((void*)dev->framebuffer.buffer, (void*)backbuffer->buffer, size);
}

static bool vbe_drv_set_mode(AGDPE_DisplayDevice* dev, uint32_t width, uint32_t height, uint32_t bpp) {
    // VBE driver cannot change modes at runtime without dropping back to Real Mode (BIOS).
    // So this is unsupported.
    return false;
}

static void vbe_drv_wait_for_vsync(AGDPE_DisplayDevice* dev) {
    // Standard VGA vertical retrace port check
    while ((io_in8(0x3DA) & 0x08));
    while (!(io_in8(0x3DA) & 0x08));
}

void AGDPE_VBE_Driver_Initialize(void* boot_info_ptr) {
    boot_info_t* boot_info = (boot_info_t*)boot_info_ptr;
    
    // Initialize the legacy driver
    vbe_init(boot_info);
    
    BVFramebuffer* raw_fb = vbe_get_framebuffer();
    if (!raw_fb) {
        display_print("[AGDPE-VBE] FATAL: VBE Framebuffer not found!\n");
        return;
    }
    
    AGDPE_DisplayDevice dev;
    memset(&dev, 0, sizeof(dev));
    
    dev.state = AGDPE_DISPLAY_STATE_ACTIVE;
    dev.driver_type = AGDPE_DRIVER_TYPE_VBE;
    strncpy(dev.name, "Generic VESA BIOS Display", sizeof(dev.name));
    
    dev.width = raw_fb->width;
    dev.height = raw_fb->height;
    dev.pitch = raw_fb->pitch;
    dev.bpp = 32; // ATOMS OS standardizes on 32bpp
    
    dev.supports_hardware_cursor = false;
    dev.supports_vsync = true;
    dev.supports_page_flipping = false; // VBE supports it via banks, but we abstract it for now
    
    dev.framebuffer = *raw_fb;
    
    dev.SwapBuffers = vbe_drv_swap_buffers;
    dev.SetMode = vbe_drv_set_mode;
    dev.WaitForVSync = vbe_drv_wait_for_vsync;
    
    AGDPE_RegisterDevice(&dev);
}
