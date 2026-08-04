#ifndef PLAYBACK_EVENTS_H
#define PLAYBACK_EVENTS_H

#include "../include/bospectra_playback_types.h"
#include "../../include/bospectra_errors.h"

#define BOSPECTRA_EVENT_QUEUE_SIZE 16U

typedef struct {
    bospectra_playback_event_t event;
    uint64_t                   timestamp_us;
    uint32_t                   param;
} BOSPECTRA_MediaEvent;

typedef struct {
    BOSPECTRA_MediaEvent queue[BOSPECTRA_EVENT_QUEUE_SIZE];
    uint32_t             head;
    uint32_t             tail;
    uint32_t             count;
} BOSPECTRA_EventBus;

void              playback_events_init(BOSPECTRA_EventBus* bus);
bospectra_error_t playback_events_post(BOSPECTRA_EventBus* bus, bospectra_playback_event_t evt, uint32_t param);
bospectra_error_t playback_events_pop(BOSPECTRA_EventBus* bus, BOSPECTRA_MediaEvent* out_event);

#endif // PLAYBACK_EVENTS_H
