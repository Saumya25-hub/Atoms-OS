#include "hida.h"
#include "kernel/core/vizier/include/vizier.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/timer/include/timer.h"
#include "ccte.h"

#define MAX_HIDA_DEVICES 8
#define HIDA_SILENCE_TIMEOUT_TICKS 3000 // 3 seconds timeout for active fallback

static InputDeviceDescriptor g_hida_registry[MAX_HIDA_DEVICES];
static uint32_t g_hida_device_count = 0;
static uint32_t g_hida_owner = HIDA_BACKEND_NONE;
static HidaState g_hida_state = HIDA_STATE_DETECTING;
static uint64_t g_hida_last_event_tick = 0;
static uint32_t g_hida_fallback_count = 0;
static uint32_t g_hida_conflict_count = 0;

void hida_init(void) {
    g_hida_device_count = 0;
    g_hida_owner = HIDA_BACKEND_NONE;
    g_hida_state = HIDA_STATE_DETECTING;
    g_hida_last_event_tick = 0;
    g_hida_fallback_count = 0;
    g_hida_conflict_count = 0;

    for (int i = 0; i < MAX_HIDA_DEVICES; i++) {
        g_hida_registry[i].backend_id = HIDA_BACKEND_NONE;
        g_hida_registry[i].is_connected = false;
        g_hida_registry[i].is_initialized = false;
    }

    VizierContract c = {0};
    c.subsystem_name = "HIDA";
    c.subsystem_id = 110;
    vizier_register_subsystem(&c);
}

void hida_register_device(const InputDeviceDescriptor* desc) {
    if (!desc || desc->backend_id == HIDA_BACKEND_NONE) return;

    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        if (g_hida_registry[i].backend_id == desc->backend_id) {
            g_hida_registry[i] = *desc;
            return;
        }
    }

    if (g_hida_device_count < MAX_HIDA_DEVICES) {
        g_hida_registry[g_hida_device_count++] = *desc;
    }
}

void hida_set_device_connected(uint32_t backend_id, bool connected) {
    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        if (g_hida_registry[i].backend_id == backend_id) {
            g_hida_registry[i].is_connected = connected;
            if (!connected && g_hida_owner == backend_id) {
                g_hida_owner = HIDA_BACKEND_NONE;
            }
            return;
        }
    }
}

static InputDeviceDescriptor* hida_find_descriptor(uint32_t backend_id) {
    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        if (g_hida_registry[i].backend_id == backend_id) {
            return &g_hida_registry[i];
        }
    }
    return NULL;
}

const InputDeviceDescriptor* hida_get_device_descriptor(uint32_t backend_id) {
    return hida_find_descriptor(backend_id);
}

uint32_t hida_get_active_owner(void) {
    return g_hida_owner;
}

static void hida_arbitrate(uint32_t incoming_backend_id) {
    uint64_t now = timer_get_ticks();
    
    // Find best available connected device based on priority scoring
    uint32_t best_backend = HIDA_BACKEND_NONE;
    uint32_t highest_score = 0;

    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        InputDeviceDescriptor* dev = &g_hida_registry[i];
        if (!dev->is_connected || !dev->is_initialized) continue;

        // Check if device is active or recently emitted an event
        bool is_recent = (dev->last_event_tick > 0) && ((now - dev->last_event_tick) < HIDA_SILENCE_TIMEOUT_TICKS);
        
        // If this is the incoming event emitter, it is active right now
        if (dev->backend_id == incoming_backend_id) {
            is_recent = true;
        }

        if (is_recent || dev->last_event_tick == 0) {
            if (dev->priority_score > highest_score) {
                highest_score = dev->priority_score;
                best_backend = dev->backend_id;
            }
        }
    }

    if (best_backend != HIDA_BACKEND_NONE && best_backend != g_hida_owner) {
        if (g_hida_owner != HIDA_BACKEND_NONE) {
            g_hida_fallback_count++;
            display_print("[HIDA] Auto-Switch: Input owner changed from ");
            display_print_dec(g_hida_owner);
            display_print(" to ");
            display_print_dec(best_backend);
            display_print(" (Score: ");
            display_print_dec(highest_score);
            display_print(")\n");
        }
        g_hida_owner = best_backend;
        g_hida_state = HIDA_STATE_ACTIVE;
    }
}

void hida_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint32_t max_x, uint32_t max_y, uint8_t buttons, int32_t scroll) {
    uint64_t now = timer_get_ticks();
    g_hida_last_event_tick = now;

    InputDeviceDescriptor* dev = hida_find_descriptor(backend_id);
    if (dev) {
        dev->last_event_tick = now;
        dev->total_events++;
        dev->status = HIDA_STATE_ACTIVE;
        dev->is_connected = true;
    } else {
        // Auto-register dynamically if backend pushes events without prior registration
        InputDeviceDescriptor auto_desc = {0};
        auto_desc.backend_id = backend_id;
        auto_desc.type = (backend_id == HIDA_BACKEND_VMMOUSE) ? INPUT_DEV_TYPE_VMMOUSE : INPUT_DEV_TYPE_USB_TABLET;
        auto_desc.device_name = (backend_id == HIDA_BACKEND_VMMOUSE) ? "VMMouse Absolute" : "USB HID Absolute Device";
        auto_desc.driver_name = "auto_hida";
        auto_desc.is_supported = true;
        auto_desc.is_initialized = true;
        auto_desc.is_connected = true;
        auto_desc.is_absolute = true;
        auto_desc.priority_score = (backend_id == HIDA_BACKEND_VMMOUSE) ? 100 : 90;
        auto_desc.health_score = 100;
        auto_desc.status = HIDA_STATE_ACTIVE;
        auto_desc.last_event_tick = now;
        auto_desc.total_events = 1;
        hida_register_device(&auto_desc);
    }

    hida_arbitrate(backend_id);

    uint32_t auth_owner = vizier_get_authoritative_owner(VIZIER_CAP_INPUT_POINTER_RAW);
    if (auth_owner != 0 && auth_owner != 110) {
        return; // Suppressed by Vizier Governance
    }

    // Duplicate-event suppression: drop events from non-owner backends
    if (g_hida_owner != HIDA_BACKEND_NONE && g_hida_owner != backend_id) {
        g_hida_conflict_count++;
        return;
    }

    ccte_push_absolute(backend_id, x, y, max_x, max_y, buttons, scroll);
}

void hida_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll) {
    uint64_t now = timer_get_ticks();
    g_hida_last_event_tick = now;

    InputDeviceDescriptor* dev = hida_find_descriptor(backend_id);
    if (dev) {
        dev->last_event_tick = now;
        dev->total_events++;
        dev->status = HIDA_STATE_ACTIVE;
        dev->is_connected = true;
    } else {
        // Auto-register dynamically if backend pushes events without prior registration
        InputDeviceDescriptor auto_desc = {0};
        auto_desc.backend_id = backend_id;
        auto_desc.type = (backend_id == HIDA_BACKEND_PS2) ? INPUT_DEV_TYPE_PS2_MOUSE : INPUT_DEV_TYPE_USB_HID_MOUSE;
        auto_desc.device_name = (backend_id == HIDA_BACKEND_PS2) ? "PS/2 Relative Mouse" : "USB HID Relative Mouse";
        auto_desc.driver_name = "auto_hida";
        auto_desc.is_supported = true;
        auto_desc.is_initialized = true;
        auto_desc.is_connected = true;
        auto_desc.is_absolute = false;
        auto_desc.priority_score = (backend_id == HIDA_BACKEND_USB) ? 80 : 60;
        auto_desc.health_score = 100;
        auto_desc.status = HIDA_STATE_ACTIVE;
        auto_desc.last_event_tick = now;
        auto_desc.total_events = 1;
        hida_register_device(&auto_desc);
    }

    hida_arbitrate(backend_id);

    uint32_t auth_owner = vizier_get_authoritative_owner(VIZIER_CAP_INPUT_POINTER_RAW);
    if (auth_owner != 0 && auth_owner != 110) {
        return; // Suppressed by Vizier Governance
    }

    // Duplicate-event suppression: drop events from non-owner backends
    if (g_hida_owner != HIDA_BACKEND_NONE && g_hida_owner != backend_id) {
        g_hida_conflict_count++;
        return;
    }

    ccte_push_relative(backend_id, dx, dy, buttons, scroll);
}

void hida_dump_status(void) {
    display_print("\n========================================================\n");
    display_print("     BOS OS INPUT DEVICE MANAGER (HIDA ARBITER) REPORT  \n");
    display_print("========================================================\n");
    display_print("Registered Devices: ");
    display_print_dec(g_hida_device_count);
    display_print("\nActive Input Owner: ");
    if (g_hida_owner == HIDA_BACKEND_VMMOUSE) display_print("VMware VMMouse Absolute [Priority 100]\n");
    else if (g_hida_owner == HIDA_BACKEND_USB) display_print("USB HID Device [Priority 80/90]\n");
    else if (g_hida_owner == HIDA_BACKEND_PS2) display_print("8042 PS/2 Mouse [Priority 60]\n");
    else display_print("NONE (Waiting for input)\n");

    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        InputDeviceDescriptor* dev = &g_hida_registry[i];
        display_print(" - ");
        display_print(dev->device_name);
        display_print(" (ID:"); display_print_dec(dev->backend_id);
        display_print(") Score:"); display_print_dec(dev->priority_score);
        display_print(" Conn:"); display_print(dev->is_connected ? "YES" : "NO");
        display_print(" Events:"); display_print_dec((uint32_t)dev->total_events);
        display_print("\n");
    }
    display_print("========================================================\n\n");
}
