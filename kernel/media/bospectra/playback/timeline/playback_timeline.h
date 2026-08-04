#ifndef PLAYBACK_TIMELINE_H
#define PLAYBACK_TIMELINE_H

#include "../include/bospectra_playback_types.h"
#include "../../include/bospectra_errors.h"

typedef struct {
    uint64_t current_position_us;
    uint64_t duration_us;
    uint32_t speed_x100;
    bool     is_looping;
    uint32_t loop_count;
} BOSPECTRA_Timeline;

void              playback_timeline_init(BOSPECTRA_Timeline* tl, uint64_t duration_us);
void              playback_timeline_update_position(BOSPECTRA_Timeline* tl, uint64_t position_us);
void              playback_timeline_seek(BOSPECTRA_Timeline* tl, uint64_t seek_position_us);
void              playback_timeline_set_speed(BOSPECTRA_Timeline* tl, uint32_t speed_x100);
void              playback_timeline_set_loop(BOSPECTRA_Timeline* tl, bool loop);
bospectra_error_t playback_timeline_get_info(const BOSPECTRA_Timeline* tl, BOSPECTRA_TimelineInfo* out_info);

#endif // PLAYBACK_TIMELINE_H
