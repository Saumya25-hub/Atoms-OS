#include "dispatcher_filters.h"
#include "dispatcher_diag.h"

static DispatcherEvent s_last_event;
static bool s_has_last = false;

void dispatcher_filters_init(void) {
    s_has_last = false;
}

bool dispatcher_filters_evaluate(const DispatcherEvent* event) {
    if (!event) {
        dispatcher_diag_record_filter_drop(false, false, true);
        return false;
    }

    // 1. Hardware Error / Overflow Filter (Flag bit 0: overflow/error)
    if (event->flags & 0x01) {
        dispatcher_diag_record_filter_drop(false, false, true);
        return false;
    }

    // 2. Zero-Delta Motion Filter (Relative Motion with dx=0, dy=0, and unchanged buttons)
    if (event->type == INPUT_EVENT_TYPE_MOTION_RELATIVE) {
        if (event->data.motion_rel.dx == 0 && event->data.motion_rel.dy == 0) {
            if (s_has_last && s_last_event.type == INPUT_EVENT_TYPE_MOTION_RELATIVE &&
                s_last_event.data.motion_rel.buttons == event->data.motion_rel.buttons) {
                dispatcher_diag_record_filter_drop(true, false, false);
                return false;
            }
        }
    }

    // 3. Duplicate Key-Repeat Bounce Filter (identical key packet within 2ms)
    if (event->type == INPUT_EVENT_TYPE_KEY && s_has_last) {
        if (s_last_event.type == INPUT_EVENT_TYPE_KEY &&
            s_last_event.data.key.keycode == event->data.key.keycode &&
            s_last_event.data.key.pressed == event->data.key.pressed &&
            s_last_event.data.key.modifiers == event->data.key.modifiers) {
            
            if (event->timestamp_us >= s_last_event.timestamp_us &&
                (event->timestamp_us - s_last_event.timestamp_us) < 2000) {
                dispatcher_diag_record_filter_drop(false, true, false);
                return false;
            }
        }
    }

    s_last_event = *event;
    s_has_last = true;
    return true;
}
