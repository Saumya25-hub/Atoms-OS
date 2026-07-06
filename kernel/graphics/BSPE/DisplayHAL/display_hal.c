/**
 * @file display_hal.c
 * @brief BSPE Display Hardware Abstraction Layer Production Implementation
 * @status Step 4 Production Implementation
 * 
 * @section PURPOSE
 * Implements driver registration, probing, selection, initialization, capability reporting,
 * and high-level VRAM/Page-flip delegation to physical video driver backends.
 */

#include "display_hal.h"
#include <stddef.h>

#define BSPE_MAX_REGISTERED_DRIVERS 8

typedef struct BSPE_DisplayDriver_T {
    char name[32];
    const BSPE_DisplayDriverInterface* interface;
    BSPE_DisplayDriverCaps caps;
    bool is_registered;
    bool is_active;
} BSPE_DisplayDriverInstance;

static BSPE_DisplayDriverInstance g_drivers[BSPE_MAX_REGISTERED_DRIVERS];
static BSPE_DisplayDriverInstance* g_active_driver = NULL;
static bool g_hal_initialized = false;

/* Helper to copy strings without depending on string.h */
static void hal_strcpy_safe(char* dest, const char* src, uint32_t max_len) {
    uint32_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* --- Public Driver Management Implementations --- */

BSPE_Error BSPE_DisplayHAL_RegisterDriver(const char* name, const BSPE_DisplayDriverInterface* interface, BSPE_DisplayDriverHandle* out_handle) {
    if (!name || !interface) {
        return BSPE_ERR_NULL_POINTER;
    }
    
    for (uint32_t i = 0; i < BSPE_MAX_REGISTERED_DRIVERS; i++) {
        if (!g_drivers[i].is_registered) {
            g_drivers[i].is_registered = true;
            g_drivers[i].is_active = false;
            g_drivers[i].interface = interface;
            hal_strcpy_safe(g_drivers[i].name, name, sizeof(g_drivers[i].name));
            
            /* Default capabilities if driver doesn't report them yet */
            g_drivers[i].caps.max_width = 1920;
            g_drivers[i].caps.max_height = 1080;
            g_drivers[i].caps.vram_size_bytes = 16 * 1024 * 1024;
            g_drivers[i].caps.supports_hw_cursor = true;
            g_drivers[i].caps.supports_page_flip = true;
            
            if (out_handle) {
                *out_handle = (BSPE_DisplayDriverHandle)&g_drivers[i];
            }
            return BSPE_OK;
        }
    }
    return BSPE_ERR_OUT_OF_MEMORY;
}

void BSPE_DisplayHAL_UnregisterDriver(BSPE_DisplayDriverHandle handle) {
    if (!handle) return;
    BSPE_DisplayDriverInstance* drv = (BSPE_DisplayDriverInstance*)handle;
    if (drv == g_active_driver) {
        if (drv->interface && drv->interface->shutdown) {
            drv->interface->shutdown();
        }
        g_active_driver = NULL;
    }
    drv->is_registered = false;
    drv->is_active = false;
    drv->interface = NULL;
}

BSPE_Error BSPE_DisplayHAL_SetActiveDriver(BSPE_DisplayDriverHandle handle) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_DisplayDriverInstance* drv = (BSPE_DisplayDriverInstance*)handle;
    if (!drv->is_registered) return BSPE_ERR_INVALID_STATE;
    
    if (g_active_driver && g_active_driver != drv) {
        if (g_active_driver->interface && g_active_driver->interface->shutdown) {
            g_active_driver->interface->shutdown();
        }
        g_active_driver->is_active = false;
    }
    
    g_active_driver = drv;
    g_active_driver->is_active = true;
    return BSPE_OK;
}

BSPE_DisplayDriverHandle BSPE_DisplayHAL_GetActiveDriver(void) {
    return (BSPE_DisplayDriverHandle)g_active_driver;
}

BSPE_Error BSPE_DisplayHAL_GetCaps(BSPE_DisplayDriverHandle handle, BSPE_DisplayDriverCaps* out_caps) {
    if (!out_caps) return BSPE_ERR_NULL_POINTER;
    BSPE_DisplayDriverInstance* drv = handle ? (BSPE_DisplayDriverInstance*)handle : g_active_driver;
    if (!drv || !drv->is_registered) return BSPE_ERR_DRIVER_NOT_FOUND;
    *out_caps = drv->caps;
    return BSPE_OK;
}

BSPE_Error BSPE_DisplayHAL_ProbeAndSelectDriver(void) {
    /* Probe registered drivers and select the first active valid driver */
    for (uint32_t i = 0; i < BSPE_MAX_REGISTERED_DRIVERS; i++) {
        if (g_drivers[i].is_registered && g_drivers[i].interface) {
            g_active_driver = &g_drivers[i];
            g_active_driver->is_active = true;
            return BSPE_OK;
        }
    }
    return BSPE_ERR_DRIVER_NOT_FOUND;
}

/* --- High-Level HAL Delegation Implementations --- */

BSPE_Error BSPE_DisplayHAL_InitDisplay(uint32_t width, uint32_t height, uint32_t bpp) {
    if (!g_active_driver || !g_active_driver->interface) {
        BSPE_Error probe_err = BSPE_DisplayHAL_ProbeAndSelectDriver();
        if (probe_err != BSPE_OK) return probe_err;
    }
    if (g_active_driver->interface->init) {
        BSPE_Error err = g_active_driver->interface->init(width, height, bpp);
        if (err == BSPE_OK) g_hal_initialized = true;
        return err;
    }
    return BSPE_ERR_UNSUPPORTED;
}

void BSPE_DisplayHAL_ShutdownDisplay(void) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->shutdown) {
        g_active_driver->interface->shutdown();
    }
    g_hal_initialized = false;
}

void* BSPE_DisplayHAL_GetFramebufferBase(void) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->get_framebuffer_base) {
        return g_active_driver->interface->get_framebuffer_base();
    }
    return NULL;
}

BSPE_Error BSPE_DisplayHAL_SwapPage(uint32_t y_offset) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->swap_page) {
        return g_active_driver->interface->swap_page(y_offset);
    }
    return BSPE_ERR_UNSUPPORTED;
}

void BSPE_DisplayHAL_WaitForVSync(void) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->wait_vsync) {
        g_active_driver->interface->wait_vsync();
    }
}

BSPE_Error BSPE_DisplayHAL_CopyRectToVRAM(const void* src_ram, uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->copy_rect_to_vram) {
        return g_active_driver->interface->copy_rect_to_vram(src_ram, dest_x, dest_y, width, height);
    }
    return BSPE_ERR_UNSUPPORTED;
}

BSPE_Error BSPE_DisplayHAL_CursorSetPosition(int32_t x, int32_t y) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->cursor_set_position) {
        return g_active_driver->interface->cursor_set_position(x, y);
    }
    return BSPE_ERR_UNSUPPORTED;
}

BSPE_Error BSPE_DisplayHAL_CursorSetImage(const uint32_t* argb_32x32) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->cursor_set_image) {
        return g_active_driver->interface->cursor_set_image(argb_32x32);
    }
    return BSPE_ERR_UNSUPPORTED;
}

void BSPE_DisplayHAL_CursorEnable(bool enable) {
    if (g_active_driver && g_active_driver->interface && g_active_driver->interface->cursor_enable) {
        g_active_driver->interface->cursor_enable(enable);
    }
}
