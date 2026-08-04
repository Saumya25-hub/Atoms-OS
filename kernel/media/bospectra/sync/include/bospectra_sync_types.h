#ifndef BOSPECTRA_SYNC_TYPES_H
#define BOSPECTRA_SYNC_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOSPECTRA_CLOCK_SOURCE_AUDIO = 0,   // Primary Master Clock (Audio PTS)
    BOSPECTRA_CLOCK_SOURCE_SYSTEM,      // Fallback Master Clock (System RDTSC)
    BOSPECTRA_CLOCK_SOURCE_HARDWARE     // Hardware Timer Clock
} bospectra_clock_source_t;

typedef enum {
    BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY = 0,
    BOSPECTRA_SYNC_DECISION_WAIT,
    BOSPECTRA_SYNC_DECISION_DROP_FRAME,
    BOSPECTRA_SYNC_DECISION_REPEAT_FRAME
} bospectra_sync_decision_t;

#include "../../playback/include/bospectra_playback_types.h"

#endif // BOSPECTRA_SYNC_TYPES_H
