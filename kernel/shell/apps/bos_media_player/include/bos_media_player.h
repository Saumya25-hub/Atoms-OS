#ifndef BOS_MEDIA_PLAYER_H
#define BOS_MEDIA_PLAYER_H

#include "kernel/wm/bwe/include/bwe.h"
#include "media_player_types.h"

typedef struct {
    uint32_t               window_id;
    uint32_t               width;
    uint32_t               height;
    bos_player_view_mode_t view_mode;
    bool                   is_running;
    char                   current_file[128];
} BOS_MediaPlayerApp;

// Application Lifecycle Entry Points
bwe_error_t bos_media_player_launch(uint32_t* out_win_id);
bwe_error_t bos_media_player_launch_file(const char* filepath, uint32_t* out_win_id);
bwe_error_t bos_media_player_close(void);
void        bos_media_player_tick(void);

#endif // BOS_MEDIA_PLAYER_H
