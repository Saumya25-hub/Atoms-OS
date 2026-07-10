/**
 * @file agdpe_core.c
 * @brief ATOMEGear Display Platform Engine (AGDPE) Core Implementation
 */

#include "agdpe.h"
#include "kernel/core/lib/include/string.h"

// External diagnostics
extern void display_print(const char* str);
extern void display_print_hex(uint32_t val);

static AGDPE_DisplayDevice s_devices[AGDPE_MAX_DISPLAYS];
static uint32_t s_device_count = 0;
static bool s_agdpe_initialized = false;

void AGDPE_Initialize(void) {
    if (s_agdpe_initialized) return;
    
    memset(s_devices, 0, sizeof(s_devices));
    s_device_count = 0;
    
    display_print("[AGDPE] Platform Engine Core Initialized.\n");
    s_agdpe_initialized = true;
}

bool AGDPE_RegisterDevice(AGDPE_DisplayDevice* device) {
    if (!device || s_device_count >= AGDPE_MAX_DISPLAYS) {
        return false;
    }
    
    device->display_id = s_device_count;
    s_devices[s_device_count] = *device;
    s_device_count++;
    
    display_print("[AGDPE] Registered Device: ");
    display_print(device->name);
    display_print(" (ID ");
    display_print_hex(device->display_id);
    display_print(")\n");
    
    return true;
}

uint32_t AGDPE_GetActiveDisplayCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < s_device_count; i++) {
        if (s_devices[i].state == AGDPE_DISPLAY_STATE_ACTIVE) {
            count++;
        }
    }
    return count;
}

AGDPE_DisplayDevice* AGDPE_GetPrimaryDisplay(void) {
    if (s_device_count == 0) return 0; // null
    return &s_devices[0]; // Device 0 is primary by default
}

AGDPE_DisplayDevice* AGDPE_GetDisplay(uint32_t display_id) {
    if (display_id >= s_device_count) return 0;
    return &s_devices[display_id];
}
