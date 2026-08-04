#include "playback_events.h"
#include "kernel/core/lib/include/string.h"

void playback_events_init(BOSPECTRA_EventBus* bus) {
    if (!bus) return;
    memset(bus, 0, sizeof(BOSPECTRA_EventBus));
}

bospectra_error_t playback_events_post(BOSPECTRA_EventBus* bus, bospectra_playback_event_t evt, uint32_t param) {
    if (!bus) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (bus->count >= BOSPECTRA_EVENT_QUEUE_SIZE) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    bus->queue[bus->tail].event = evt;
    bus->queue[bus->tail].param = param;
    bus->queue[bus->tail].timestamp_us = 0; // Relative timeline timestamp

    bus->tail = (bus->tail + 1) % BOSPECTRA_EVENT_QUEUE_SIZE;
    bus->count++;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t playback_events_pop(BOSPECTRA_EventBus* bus, BOSPECTRA_MediaEvent* out_event) {
    if (!bus || !out_event) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (bus->count == 0) {
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_event = bus->queue[bus->head];
    bus->head = (bus->head + 1) % BOSPECTRA_EVENT_QUEUE_SIZE;
    bus->count--;

    return BOSPECTRA_SUCCESS;
}
