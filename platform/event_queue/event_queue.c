#include "platform/include/bos_events.h"
#include <stddef.h>

static BOS_EventQueue g_global_event_queue;

void BOS_GlobalEventQueue_Init(void) {
    BOS_EventQueue_Init(&g_global_event_queue);
}

BOS_Result BOS_GlobalEventQueue_Post(const BOS_Event* event) {
    return BOS_EventQueue_Push(&g_global_event_queue, event);
}

BOS_Result BOS_GlobalEventQueue_Fetch(BOS_Event* out_event) {
    return BOS_EventQueue_Pop(&g_global_event_queue, out_event);
}
