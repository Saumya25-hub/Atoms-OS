#include "playback_timeline.h"
#include "kernel/core/lib/include/string.h"

void playback_timeline_init(BOSPECTRA_Timeline* tl, uint64_t duration_us) {
    if (!tl) return;
    memset(tl, 0, sizeof(BOSPECTRA_Timeline));
    tl->duration_us = (duration_us > 0) ? duration_us : 60000000ULL; // 60s default
    tl->speed_x100 = 100; // 1.0x
    tl->is_looping = false;
    tl->loop_count = 0;
}

void playback_timeline_update_position(BOSPECTRA_Timeline* tl, uint64_t position_us) {
    if (!tl) return;
    tl->current_position_us = (position_us <= tl->duration_us) ? position_us : tl->duration_us;
}

void playback_timeline_seek(BOSPECTRA_Timeline* tl, uint64_t seek_position_us) {
    if (!tl) return;
    tl->current_position_us = (seek_position_us <= tl->duration_us) ? seek_position_us : tl->duration_us;
}

void playback_timeline_set_speed(BOSPECTRA_Timeline* tl, uint32_t speed_x100) {
    if (tl && speed_x100 >= 25 && speed_x100 <= 400) {
        tl->speed_x100 = speed_x100;
    }
}

void playback_timeline_set_loop(BOSPECTRA_Timeline* tl, bool loop) {
    if (tl) {
        tl->is_looping = loop;
    }
}

bospectra_error_t playback_timeline_get_info(const BOSPECTRA_Timeline* tl, BOSPECTRA_TimelineInfo* out_info) {
    if (!tl || !out_info) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    out_info->current_position_us = tl->current_position_us;
    out_info->duration_us = tl->duration_us;
    out_info->remaining_time_us = (tl->duration_us > tl->current_position_us) ? (tl->duration_us - tl->current_position_us) : 0;
    out_info->speed_x100 = tl->speed_x100;
    out_info->is_looping = tl->is_looping;
    out_info->loop_count = tl->loop_count;

    if (tl->duration_us > 0) {
        out_info->progress_percent_x10 = (uint32_t)((tl->current_position_us * 1000ULL) / tl->duration_us);
    } else {
        out_info->progress_percent_x10 = 0;
    }

    return BOSPECTRA_SUCCESS;
}
