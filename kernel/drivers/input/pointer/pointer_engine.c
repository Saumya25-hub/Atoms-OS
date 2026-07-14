#include "pointer_engine.h"
#include "kernel/drivers/display/display.h"

bool pointer_engine_on_event(const InputCoreEvent* event, void* user_data) {
    (void)user_data;
    if (!event) return false;
    extern void display_print(const char*);
    display_print("[INPUT TRACE] PointerEngine\n");
    if (event->type == INPUT_EVENT_TYPE_MOTION_RELATIVE ||
        event->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE ||
        event->type == INPUT_EVENT_TYPE_BUTTON) {
        pointer_motion_process(event);
        return true;
    }
    return false;
}

void pointer_engine_init(uint32_t screen_width, uint32_t screen_height) {
    uint32_t w = (screen_width > 0) ? screen_width : 1280;
    uint32_t h = (screen_height > 0) ? screen_height : 720;

    display_print("[POINTER ENGINE] Initializing ATOMS OS Pointer Engine (Phase 3)...\n");

    pointer_state_init(w / 2, h / 2);
    pointer_precision_init();
    pointer_velocity_init();
    pointer_buttons_init();
    pointer_bounds_init(w, h);
    pointer_consumers_init();
    pointer_motion_init();

    // Register Pointer Engine as Tier 0 Consumer in Input Core
    bool registered = input_core_register_consumer("PointerEngine", INPUT_PRIORITY_POINTER_ENGINE,
                                                   pointer_engine_on_event, 0);
    if (registered) {
        display_print("[POINTER ENGINE] Successfully Registered as Tier 0 Input Core Consumer.\n");
    } else {
        display_print("[POINTER ENGINE] ERROR: Failed to register with Input Core!\n");
    }
}
