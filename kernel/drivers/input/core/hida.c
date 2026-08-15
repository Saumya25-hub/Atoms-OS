#include "hida.h"
#include "input_core.h"
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
        g_hida_registry[i].is_enumerated = false;
        g_hida_registry[i].is_receiving_events = false;
        g_hida_registry[i].is_eligible = false;
        g_hida_registry[i].valid_packet_count = 0;
        g_hida_registry[i].invalid_packet_count = 0;
        g_hida_registry[i].parse_errors = 0;
        g_hida_registry[i].sync_errors = 0;
        g_hida_registry[i].health_score = 100;
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
            if (!connected) {
                g_hida_registry[i].is_eligible = false;
                g_hida_registry[i].is_receiving_events = false;
                if (g_hida_owner == backend_id) {
                    g_hida_owner = HIDA_BACKEND_NONE;
                }
            }
            return;
        }
    }
}

void hida_report_event_parsed(uint32_t backend_id, bool is_valid) {
    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        if (g_hida_registry[i].backend_id == backend_id) {
            if (is_valid) {
                g_hida_registry[i].valid_packet_count++;
                g_hida_registry[i].is_receiving_events = true;
                if (g_hida_registry[i].health_score < 100) {
                    g_hida_registry[i].health_score++;
                }
            } else {
                g_hida_registry[i].invalid_packet_count++;
                g_hida_registry[i].parse_errors++;
                if (g_hida_registry[i].health_score > 5) {
                    g_hida_registry[i].health_score -= 5;
                }
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

// PHASE 4 & 5: Eligibility & Health Evaluation Engine
static bool hida_eval_eligibility(InputDeviceDescriptor* dev, uint64_t now_ticks) {
    if (!dev) return false;

    // Rule 1: Driver must be initialized, connected, and enumerated
    if (!dev->is_initialized || !dev->is_connected) {
        dev->is_eligible = false;
        dev->status = HIDA_STATE_FAILED;
        return false;
    }

    // Rule 2: Must have parsed at least 1 valid packet (NEVER trust init alone!)
    if (dev->valid_packet_count == 0) {
        dev->is_eligible = false;
        dev->status = HIDA_STATE_DETECTING;
        return false;
    }

    // Rule 3: Health score must be above threshold (>= 50)
    if (dev->health_score < 50) {
        dev->is_eligible = false;
        dev->status = HIDA_STATE_DEGRADED;
        return false;
    }

    // Rule 4: Silence timeout check (If backend hasn't emitted events in > 3 seconds, mark inactive)
    if (dev->last_event_tick > 0 && (now_ticks - dev->last_event_tick) >= HIDA_SILENCE_TIMEOUT_TICKS) {
        dev->is_eligible = false;
        dev->is_receiving_events = false;
        dev->status = HIDA_STATE_FALLBACK;
        return false;
    }

    dev->is_eligible = true;
    dev->is_receiving_events = true;
    dev->status = HIDA_STATE_ACTIVE;
    return true;
}

// PHASE 6 & 7: Priority & Auto-Fallback Arbitration Engine
static void hida_arbitrate(uint32_t incoming_backend_id) {
    uint64_t now = timer_get_ticks();
    
    // Evaluate eligibility for all registered backends
    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        hida_eval_eligibility(&g_hida_registry[i], now);
    }

    // Find the ELIGIBLE backend with the HIGHEST priority score
    uint32_t best_backend = HIDA_BACKEND_NONE;
    uint32_t highest_score = 0;

    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        InputDeviceDescriptor* dev = &g_hida_registry[i];
        if (dev->is_eligible && dev->priority_score > highest_score) {
            highest_score = dev->priority_score;
            best_backend = dev->backend_id;
        }
    }

    // Dynamic Migration & Fallback
    if (best_backend != HIDA_BACKEND_NONE && best_backend != g_hida_owner) {
        if (g_hida_owner != HIDA_BACKEND_NONE) {
            g_hida_fallback_count++;
            display_print("[HIDA HEALTH ENGINE] Auto-Fallback: Input owner migrated from ");
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
        dev->valid_packet_count++;
        dev->is_receiving_events = true;
        dev->is_connected = true;
        dev->is_initialized = true;
        dev->is_enumerated = true;
    } else {
        // Dynamic registration for hardware hot-plug
        InputDeviceDescriptor auto_desc = {0};
        auto_desc.backend_id = backend_id;
        auto_desc.type = (backend_id == HIDA_BACKEND_VMMOUSE) ? INPUT_DEV_TYPE_VMMOUSE : INPUT_DEV_TYPE_USB_TABLET;
        auto_desc.device_name = (backend_id == HIDA_BACKEND_VMMOUSE) ? "VMware VMMouse Absolute" : "USB HID Absolute Device";
        auto_desc.driver_name = "auto_hida";
        auto_desc.is_supported = true;
        auto_desc.is_initialized = true;
        auto_desc.is_connected = true;
        auto_desc.is_enumerated = true;
        auto_desc.is_receiving_events = true;
        auto_desc.is_absolute = true;
        auto_desc.valid_packet_count = 1;
        auto_desc.priority_score = (backend_id == HIDA_BACKEND_VMMOUSE) ? 100 : 90;
        auto_desc.health_score = 100;
        auto_desc.status = HIDA_STATE_ACTIVE;
        auto_desc.last_event_tick = now;
        auto_desc.total_events = 1;
        hida_register_device(&auto_desc);
    }

    g_hida_owner = backend_id;
    ccte_push_absolute(backend_id, x, y, max_x, max_y, buttons, scroll);
}

void hida_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll) {
    extern volatile uint64_t g_hida_events;
    g_hida_events++;
    extern void usb_forensic_mark_stage(int stage, bool success);
    usb_forensic_mark_stage(17, true); // USB_STAGE_HIDA_ROUTER_ACTIVE
    uint64_t now = timer_get_ticks();
    g_hida_last_event_tick = now;

    InputDeviceDescriptor* dev = hida_find_descriptor(backend_id);
    if (dev) {
        dev->last_event_tick = now;
        dev->total_events++;
        dev->valid_packet_count++;
        dev->is_receiving_events = true;
        dev->is_connected = true;
        dev->is_initialized = true;
        dev->is_enumerated = true;
    } else {
        // Dynamic registration for hardware hot-plug
        InputDeviceDescriptor auto_desc = {0};
        auto_desc.backend_id = backend_id;
        auto_desc.type = (backend_id == HIDA_BACKEND_PS2) ? INPUT_DEV_TYPE_PS2_MOUSE : INPUT_DEV_TYPE_USB_HID_MOUSE;
        auto_desc.device_name = (backend_id == HIDA_BACKEND_PS2) ? "8042 PS/2 Relative Mouse" : "USB HID Relative Mouse";
        auto_desc.driver_name = "auto_hida";
        auto_desc.is_supported = true;
        auto_desc.is_initialized = true;
        auto_desc.is_connected = true;
        auto_desc.is_enumerated = true;
        auto_desc.is_receiving_events = true;
        auto_desc.is_absolute = false;
        auto_desc.valid_packet_count = 1;
        auto_desc.priority_score = (backend_id == HIDA_BACKEND_USB) ? 80 : 60;
        auto_desc.health_score = 100;
        auto_desc.status = HIDA_STATE_ACTIVE;
        auto_desc.last_event_tick = now;
        auto_desc.total_events = 1;
        hida_register_device(&auto_desc);
    }

    g_hida_owner = backend_id;

    // Real OS Standard (Linux libinput / Windows NT style): Direct Relative Event Dispatch
    InputCoreEvent ev = {0};
    ev.device_id = backend_id;
    ev.device_type = (backend_id == HIDA_BACKEND_PS2) ? INPUT_DEVICE_TYPE_PS2_MOUSE : INPUT_DEVICE_TYPE_USB_MOUSE;
    ev.type = INPUT_EVENT_TYPE_MOTION_RELATIVE;
    ev.timestamp_us = timer_get_ticks() * 1000;
    ev.data.motion_rel.dx = dx;
    ev.data.motion_rel.dy = dy;
    ev.data.motion_rel.buttons = buttons;
    
    input_core_push_event(&ev);
    input_core_dispatch_events();

    if (scroll != 0) {
        InputCoreEvent sev = {0};
        sev.device_id = backend_id;
        sev.device_type = ev.device_type;
        sev.type = INPUT_EVENT_TYPE_SCROLL;
        sev.timestamp_us = ev.timestamp_us;
        sev.data.scroll.delta_y = scroll;
        input_core_push_event(&sev);
        input_core_dispatch_events();
    }
}

void hida_push_keyboard_event(uint32_t backend_id, const void* kevt) {
    if (!kevt) return;
    extern volatile uint64_t g_hida_events;
    g_hida_events++;
    uint64_t now = timer_get_ticks();
    g_hida_last_event_tick = now;

    InputDeviceDescriptor* dev = hida_find_descriptor(backend_id);
    if (dev) {
        dev->last_event_tick = now;
        dev->total_events++;
        dev->valid_packet_count++;
        dev->is_receiving_events = true;
        dev->is_connected = true;
        dev->is_initialized = true;
        dev->is_enumerated = true;
    } else {
        InputDeviceDescriptor auto_desc = {0};
        auto_desc.backend_id = backend_id;
        auto_desc.type = (backend_id == HIDA_BACKEND_PS2_KBD) ? INPUT_DEV_TYPE_PS2_KEYBOARD : INPUT_DEV_TYPE_USB_HID_KEYBOARD;
        auto_desc.device_name = (backend_id == HIDA_BACKEND_PS2_KBD) ? "8042 PS/2 Keyboard" : "USB HID Keyboard";
        auto_desc.driver_name = "auto_hida_kbd";
        auto_desc.is_supported = true;
        auto_desc.is_initialized = true;
        auto_desc.is_connected = true;
        auto_desc.is_enumerated = true;
        auto_desc.is_receiving_events = true;
        auto_desc.is_absolute = false;
        auto_desc.valid_packet_count = 1;
        auto_desc.priority_score = (backend_id == HIDA_BACKEND_USB_KBD) ? 85 : 65;
        auto_desc.health_score = 100;
        auto_desc.status = HIDA_STATE_ACTIVE;
        auto_desc.last_event_tick = now;
        auto_desc.total_events = 1;
        hida_register_device(&auto_desc);
    }

    extern void kernel_input_push_key_event(const void* kevt);
    extern void keyboard_push_event(const void* kevt);
    keyboard_push_event(kevt);
    kernel_input_push_key_event(kevt);
}

// PHASE 10: Runtime Diagnostics & Health Audit Reporting
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
    else display_print("NONE (Waiting for runtime input verification)\n");

    for (uint32_t i = 0; i < g_hida_device_count; i++) {
        InputDeviceDescriptor* dev = &g_hida_registry[i];
        display_print("--------------------------------------------------------\n");
        display_print("Device: "); display_print(dev->device_name);
        display_print("\n  ID: "); display_print_dec(dev->backend_id);
        display_print(" | Priority: "); display_print_dec(dev->priority_score);
        display_print(" | Health: "); display_print_dec(dev->health_score);
        display_print("\n  Init: "); display_print(dev->is_initialized ? "YES" : "NO");
        display_print(" | Conn: "); display_print(dev->is_connected ? "YES" : "NO");
        display_print(" | Valid Packets: "); display_print_dec(dev->valid_packet_count);
        display_print("\n  Eligible: "); display_print(dev->is_eligible ? "YES" : "NO");
        display_print(" | Status: ");
        if (dev->status == HIDA_STATE_ACTIVE) display_print("ACTIVE\n");
        else if (dev->status == HIDA_STATE_FALLBACK) display_print("FALLBACK\n");
        else if (dev->status == HIDA_STATE_DEGRADED) display_print("DEGRADED\n");
        else display_print("DETECTING\n");
    }
    display_print("========================================================\n\n");
}
