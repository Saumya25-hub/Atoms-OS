#include "../include/brt_api.h"

static BRTEventCallback s_subscribers[BRT_MAX_SUBSCRIBERS];
static uint32_t         s_sub_count = 0;

int32_t BRT_PostEvent(uint32_t event_id, void* payload) {
    for (uint32_t i = 0; i < s_sub_count; i++) {
        if (s_subscribers[i]) {
            s_subscribers[i](event_id, payload);
        }
    }
    return 0;
}

int32_t BRT_Subscribe(uint32_t event_id, BRTEventCallback cb) {
    (void)event_id;
    if (!cb) return -1;
    if (s_sub_count < BRT_MAX_SUBSCRIBERS) {
        s_subscribers[s_sub_count++] = cb;
        return 0;
    }
    return -1;
}
