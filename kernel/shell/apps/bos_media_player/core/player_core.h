#ifndef PLAYER_CORE_H
#define PLAYER_CORE_H

#include "../include/bos_media_player.h"
#include "kernel/media/bospectra/playback/include/bospectra_playback.h"

typedef struct {
    BOS_MediaPlayerApp             app;
    bospectra_playback_session_id_t playback_session_id;
    bool                           is_session_open;
} PlayerCoreCtx;

bwe_error_t    player_core_init(PlayerCoreCtx* ctx);
void           player_core_shutdown(PlayerCoreCtx* ctx);
bwe_error_t    player_core_open_media(PlayerCoreCtx* ctx, const char* media_path);
bwe_error_t    player_core_toggle_play_pause(PlayerCoreCtx* ctx);
bwe_error_t    player_core_stop(PlayerCoreCtx* ctx);
bwe_error_t    player_core_seek(PlayerCoreCtx* ctx, uint64_t position_us);
bwe_error_t    player_core_toggle_fullscreen(PlayerCoreCtx* ctx);
PlayerCoreCtx* player_core_get_instance(void);

#endif // PLAYER_CORE_H
