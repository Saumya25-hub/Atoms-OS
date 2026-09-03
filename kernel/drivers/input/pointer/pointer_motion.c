#include "pointer_motion.h"
#include "pointer_precision.h"
#include "pointer_velocity.h"
#include "pointer_buttons.h"
#include "pointer_bounds.h"
#include "pointer_consumers.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/input/dispatcher/dispatcher.h"

volatile uint64_t g_pointer_event_count = 0;

void pointer_motion_init(void) {
    display_print("[POINTER MOTION] 7-Stage Deterministic Motion Pipeline Initialized.\n");
}

void pointer_motion_process(const InputCoreEvent* event) {
    if (!event) return;

    g_pointer_event_count++;

    // We only process relative motion, absolute motion, or button state events
    if (event->type != INPUT_EVENT_TYPE_MOTION_RELATIVE &&
        event->type != INPUT_EVENT_TYPE_MOTION_ABSOLUTE &&
        event->type != INPUT_EVENT_TYPE_BUTTON) {
        return;
    }

    PointerState* state = pointer_state_get_mutable();
    int32_t prev_x = state->current_x;
    int32_t prev_y = state->current_y;
    int32_t sub_x  = state->subpixel_x;
    int32_t sub_y  = state->subpixel_y;
    uint32_t button_mask = state->button_mask;

    int32_t raw_dx = 0;
    int32_t raw_dy = 0;
    int32_t whole_dx = 0;
    int32_t whole_dy = 0;
    int32_t vel_raw = 0;
    int32_t accel_fp16 = FP16_ONE; // 1.0x default

    // =========================================================================
    // Stage 1: Motion Ingest & Stage 2: Filtering
    // =========================================================================
    if (event->type == INPUT_EVENT_TYPE_MOTION_RELATIVE) {
        raw_dx = event->data.motion_rel.dx;
        raw_dy = event->data.motion_rel.dy;
        button_mask = event->data.motion_rel.buttons;

        // Stage 3: Velocity & Ballistic Acceleration Calculation
        accel_fp16 = pointer_velocity_calculate(raw_dx, raw_dy, event->timestamp_us, &vel_raw);

        // Stage 4: 16.16 Fixed-Point Sub-Pixel Accumulation
        pointer_precision_accumulate(raw_dx, raw_dy, accel_fp16, &sub_x, &sub_y, &whole_dx, &whole_dy);
    } else if (event->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) {
        int32_t abs_x = event->data.motion_abs.x;
        int32_t abs_y = event->data.motion_abs.y;
        uint32_t max_x = event->data.motion_abs.max_x;
        uint32_t max_y = event->data.motion_abs.max_y;
        button_mask = event->data.motion_abs.buttons;

        // Downscale from Canonical Coordinate Space to Display Space
        if (max_x > 0 && max_y > 0) {
            int32_t screen_w, screen_h;
            pointer_bounds_get_max(&screen_w, &screen_h);
            
            int64_t scaled_x = ((int64_t)abs_x * screen_w) / max_x;
            int64_t scaled_y = ((int64_t)abs_y * screen_h) / max_y;
            
            abs_x = (int32_t)scaled_x;
            abs_y = (int32_t)scaled_y;
        }

        whole_dx = abs_x - prev_x;
        whole_dy = abs_y - prev_y;
        raw_dx = whole_dx;
        raw_dy = whole_dy;

        vel_raw = 0;
        accel_fp16 = FP16_ONE;
        sub_x = FP16_FROM_INT(abs_x);
        sub_y = FP16_FROM_INT(abs_y);
    } else if (event->type == INPUT_EVENT_TYPE_BUTTON) {
        // Button-only event (e.g. click without movement)
        button_mask = event->data.button.button_mask;
    }

    int32_t new_x = prev_x + whole_dx;
    int32_t new_y = prev_y + whole_dy;

    // =========================================================================
    // Stage 5: Multi-Monitor Bounds Clamping
    // =========================================================================
    int32_t pre_clamp_x = new_x;
    int32_t pre_clamp_y = new_y;
    pointer_bounds_clamp(&new_x, &new_y);

    // If clamped against screen edge, synchronize sub-pixel accumulators
    if (new_x != pre_clamp_x) sub_x = FP16_FROM_INT(new_x);
    if (new_y != pre_clamp_y) sub_y = FP16_FROM_INT(new_y);

    // =========================================================================
    // Stage 6: State Machine Commit (Buttons, Dragging, Click Counts, Spatial)
    // =========================================================================
    uint32_t pressed = 0;
    uint32_t released = 0;
    uint8_t  clicks = 0;
    bool     dragging = false;
    int32_t  drag_x = 0;
    int32_t  drag_y = 0;
    uint32_t drag_btn = 0;

    pointer_buttons_process(button_mask, new_x, new_y, event->timestamp_us,
                            &pressed, &released, &clicks, &dragging,
                            &drag_x, &drag_y, &drag_btn);

    pointer_state_update_position(new_x, new_y, raw_dx, raw_dy, sub_x, sub_y,
                                  new_x - prev_x, new_y - prev_y, vel_raw, accel_fp16,
                                  event->timestamp_us, event->device_id);

    pointer_state_update_buttons(button_mask, pressed, released, clicks,
                                 dragging, drag_x, drag_y, drag_btn);

    // =========================================================================
    // Stage 7: Consumer Dispatch (Publish-Subscribe Notification Broadcast)
    // =========================================================================
    pointer_consumers_notify(pointer_state_get());

    /* High-Frequency Asynchronous Micro-Tile VRAM Cursor Presenter */
    extern volatile uint32_t g_cursor_position_requests;
    g_cursor_position_requests++;
    extern void BSPE_CursorPresenter_FastTileUpdate(void);
    BSPE_CursorPresenter_FastTileUpdate();

    DispatcherEvent smoothed_event = *event;
    if (event->type == INPUT_EVENT_TYPE_MOTION_RELATIVE || event->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) {
        smoothed_event.type = INPUT_EVENT_TYPE_MOTION_ABSOLUTE;
        smoothed_event.data.motion_abs.x = new_x;
        smoothed_event.data.motion_abs.y = new_y;
        smoothed_event.data.motion_abs.max_x = 0;
        smoothed_event.data.motion_abs.max_y = 0;
        smoothed_event.data.motion_abs.buttons = button_mask;
        dispatcher_push_event(&smoothed_event);
        
        // Also emit separate BUTTON events if any buttons changed state during this motion
        for (int i = 0; i < 8; i++) {
            if (pressed & (1 << i)) {
                DispatcherEvent btn_ev = *event;
                btn_ev.type = INPUT_EVENT_TYPE_BUTTON;
                btn_ev.data.button.button_id = i;
                btn_ev.data.button.button_mask = button_mask;
                btn_ev.data.button.pressed = true;
                dispatcher_push_event(&btn_ev);
            }
            if (released & (1 << i)) {
                DispatcherEvent btn_ev = *event;
                btn_ev.type = INPUT_EVENT_TYPE_BUTTON;
                btn_ev.data.button.button_id = i;
                btn_ev.data.button.button_mask = button_mask;
                btn_ev.data.button.pressed = false;
                dispatcher_push_event(&btn_ev);
            }
        }
    } else if (event->type == INPUT_EVENT_TYPE_BUTTON) {
        smoothed_event.data.button.button_mask = button_mask;
        dispatcher_push_event(&smoothed_event);
    }
}
