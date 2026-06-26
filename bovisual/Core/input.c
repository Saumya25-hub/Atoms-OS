#include "../Include/input.h"
#include "../Include/layout.h"

// Basic hit testing using Layout Engine
static bool HitTest_Button(const BOVISUAL_Control_Button* button, int32_t mx, int32_t my) {
    // We must hit test against the *content bounds* which is calculated by the layout engine.
    // For simplicity, if a control relies on layout bounds, we can either re-calculate or just test the raw bounds
    // depending on the architecture. Usually bounds represent the allocated space.
    BVPoint pt = { mx, my };
    return BV_RectContains(button->bounds, pt);
}

void BV_Input_ProcessEvent(const BVEvent* event, BOVISUAL_Control_Button* buttons, int button_count) {
    if (!event || !buttons) return;

    for (int i = 0; i < button_count; i++) {
        BOVISUAL_Control_Button* btn = &buttons[i];
        bool is_hit = HitTest_Button(btn, event->mouse_x, event->mouse_y);

        if (event->type == BV_EVENT_MOUSE_MOVE) {
            btn->is_hovered = is_hit;
        } 
        else if (event->type == BV_EVENT_MOUSE_DOWN) {
            if (is_hit) {
                btn->is_pressed = true;
                btn->is_focused = true;
            } else {
                btn->is_focused = false;
            }
        } 
        else if (event->type == BV_EVENT_MOUSE_UP) {
            btn->is_pressed = false;
        }
    }
}
