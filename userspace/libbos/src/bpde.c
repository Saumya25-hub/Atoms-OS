#include "../include/bpde.h"
#include "../include/bos.h"

static bool s_key_state[256] = {false};
static int32_t s_mouse_x = 0;
static int32_t s_mouse_y = 0;
static uint32_t s_mouse_buttons = 0;

int BOS_InputPollEvent(bos_input_event_t* out_event) {
    if (!out_event) return 0;
    
    int res = bos_get_input_event(out_event);
    
    if (res) {
        // Update state tracking
        if (out_event->type == BOS_INPUT_KEY_DOWN) {
            if (out_event->data.key.key < 256) {
                s_key_state[out_event->data.key.key] = true;
            }
        } else if (out_event->type == BOS_INPUT_KEY_UP) {
            if (out_event->data.key.key < 256) {
                s_key_state[out_event->data.key.key] = false;
            }
        } else if (out_event->type == BOS_INPUT_MOUSE_MOVE || out_event->type == BOS_INPUT_MOUSE_DOWN || out_event->type == BOS_INPUT_MOUSE_UP) {
            s_mouse_x = out_event->data.mouse.screen_x;
            s_mouse_y = out_event->data.mouse.screen_y;
            s_mouse_buttons = out_event->data.mouse.buttons;
        }
    }
    
    return res;
}

bool BOS_InputGetKeyState(uint32_t bos_key) {
    if (bos_key < 256) {
        return s_key_state[bos_key];
    }
    return false;
}

void BOS_InputGetMouseState(int32_t* x, int32_t* y, uint32_t* buttons) {
    if (x) *x = s_mouse_x;
    if (y) *y = s_mouse_y;
    if (buttons) *buttons = s_mouse_buttons;
}
