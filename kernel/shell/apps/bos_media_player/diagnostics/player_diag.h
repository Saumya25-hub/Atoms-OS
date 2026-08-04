#ifndef PLAYER_DIAG_H
#define PLAYER_DIAG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t active_window_id;
    uint32_t render_fps;
    uint64_t render_latency_us;
    uint32_t playlist_item_count;
    char     current_uri[128];
} BOS_PlayerStats;

void player_diag_init(void);
void player_diag_shutdown(void);
void player_diag_record_frame(uint64_t latency_us);
void player_diag_get_stats(BOS_PlayerStats* out_stats);
void player_diag_dump_telemetry(void);

#endif // PLAYER_DIAG_H
