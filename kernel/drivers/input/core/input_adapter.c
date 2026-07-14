#include "input_adapter.h"
#include "kernel/drivers/input/pointer/pointer_consumers.h"
#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/drivers/input/dispatcher/dispatcher.h"
#include "kernel/drivers/display/display.h"

extern void BOHeart_InputCapture(const BVEvent* ev);
extern bool Desktop_Shell_IsLoginActive(void);
extern void Desktop_Shell_HandleLoginEvent(const BVEvent* ev);

// Phase 4 Event Dispatcher Tier 3 Consumer Callback (Authoritative Event Dispatcher -> Legacy BVEvent)
static DispatchResult input_adapter_dispatcher_cb(const DispatcherEvent* ev, void* context) {
    (void)context;
    if (!ev) return DISPATCH_CONTINUE;
    extern void display_print(const char*);
    display_print("[INPUT TRACE] InputAdapter\n");

    if (ev->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE || ev->type == INPUT_EVENT_TYPE_BUTTON) {
        BVEvent bv;
        bv.mouse_x = (ev->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) ? ev->data.motion_abs.x : pointer_state_get()->current_x;
        bv.mouse_y = (ev->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) ? ev->data.motion_abs.y : pointer_state_get()->current_y;
        bv.mouse_buttons = (ev->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) ? (uint8_t)ev->data.motion_abs.buttons : (uint8_t)ev->data.button.button_mask;
        bv.key_code = 0;
        bv.ascii = 0;
        bv.shift = false;
        bv.ctrl = false;
        bv.alt = false;
        bv.caps_lock = false;

        if (ev->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) {
            bv.type = BV_EVENT_MOUSE_MOVE;
        } else {
            bv.type = ev->data.button.pressed ? BV_EVENT_MOUSE_DOWN : BV_EVENT_MOUSE_UP;
        }

        if (Desktop_Shell_IsLoginActive()) {
            BOHeart_InputCapture(&bv);
            Desktop_Shell_HandleLoginEvent(&bv);
        } else {
            BOHeart_InputCapture(&bv);
        }
        return DISPATCH_CONSUME;
    } else if (ev->type == INPUT_EVENT_TYPE_KEY) {
        BVEvent bv;
        bv.mouse_x = 0;
        bv.mouse_y = 0;
        bv.mouse_buttons = 0;
        bv.key_code = (uint8_t)ev->data.key.keycode;
        bv.ascii = (char)ev->data.key.ascii;
        bv.shift = (ev->data.key.modifiers & (1 << 0)) != 0;
        bv.ctrl  = (ev->data.key.modifiers & (1 << 1)) != 0;
        bv.alt   = (ev->data.key.modifiers & (1 << 2)) != 0;
        bv.caps_lock = (ev->data.key.modifiers & (1 << 3)) != 0;
        bv.type = ev->data.key.pressed ? BV_EVENT_KEY_DOWN : BV_EVENT_KEY_UP;

        if (Desktop_Shell_IsLoginActive()) {
            Desktop_Shell_HandleLoginEvent(&bv);
        } else {
            BOHeart_InputCapture(&bv);
        }
        return DISPATCH_CONSUME;
    }

    return DISPATCH_CONTINUE;
}

void input_adapter_init(void) {
    input_core_init();
    display_print("[INPUT ADAPTER] Backward Compatibility Layer Initialized.\n");
}

void input_adapter_register_pointer_consumer(void) {
    // Register as Priority Tier 3 (Window Manager) in the Phase 4 Universal Event Dispatcher
    uint32_t mask = (1 << INPUT_EVENT_TYPE_KEY) | (1 << INPUT_EVENT_TYPE_MOTION_ABSOLUTE) | (1 << INPUT_EVENT_TYPE_BUTTON);
    dispatcher_consumers_register("Legacy_BWE_Adapter", DISPATCH_TIER_3_WINDOW_MANAGER, mask, input_adapter_dispatcher_cb, 0);
    display_print("[INPUT ADAPTER] Registered as Phase 4 Event Dispatcher Tier 3 Consumer.\n");
}

void input_adapter_pump(void) {
    BVEvent ev;
    while (kernel_get_event(&ev)) {
        InputCoreEvent core_ev;
        core_ev.device_id = 1;
        core_ev.timestamp_us = 0; // Stamped by core
        core_ev.flags = 0;

        if (ev.type == BV_EVENT_MOUSE_MOVE || ev.type == BV_EVENT_MOUSE_DOWN || ev.type == BV_EVENT_MOUSE_UP) {
            core_ev.device_type = INPUT_DEVICE_TYPE_PS2_MOUSE;
            if (ev.type == BV_EVENT_MOUSE_MOVE) {
                core_ev.type = INPUT_EVENT_TYPE_MOTION_ABSOLUTE;
                core_ev.data.motion_abs.x = ev.mouse_x;
                core_ev.data.motion_abs.y = ev.mouse_y;
                core_ev.data.motion_abs.max_x = 0;
                core_ev.data.motion_abs.max_y = 0;
                core_ev.data.motion_abs.buttons = ev.mouse_buttons;
            } else {
                core_ev.type = INPUT_EVENT_TYPE_BUTTON;
                core_ev.data.button.button_id = 0;
                core_ev.data.button.button_mask = ev.mouse_buttons;
                core_ev.data.button.pressed = (ev.type == BV_EVENT_MOUSE_DOWN);
            }
        } else if (ev.type == BV_EVENT_KEY_DOWN || ev.type == BV_EVENT_KEY_UP) {
            core_ev.device_type = INPUT_DEVICE_TYPE_PS2_KEYBOARD;
            core_ev.type = INPUT_EVENT_TYPE_KEY;
            core_ev.data.key.keycode = ev.key_code;
            core_ev.data.key.ascii = ev.ascii;
            core_ev.data.key.pressed = (ev.type == BV_EVENT_KEY_DOWN);
            core_ev.data.key.modifiers = 0;
            if (ev.shift) core_ev.data.key.modifiers |= (1 << 0);
            if (ev.ctrl)  core_ev.data.key.modifiers |= (1 << 1);
            if (ev.alt)   core_ev.data.key.modifiers |= (1 << 2);
            if (ev.caps_lock) core_ev.data.key.modifiers |= (1 << 3);
        } else {
            core_ev.type = INPUT_EVENT_TYPE_NONE;
        }

        if (core_ev.type != INPUT_EVENT_TYPE_NONE) {
            input_core_push_event(&core_ev);
        }
    }
    
    // 1. Dispatch all queued core events to Tier 0 Pointer Engine and push to Event Dispatcher
    input_core_dispatch_events();

    // 2. Pump Universal Event Dispatcher to route standardized immutable events to UI consumers (Tier 3 BWE)
    dispatcher_pump_events();
}
