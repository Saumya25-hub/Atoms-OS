#include "player_core.h"
#include "kernel/media/bospectra/include/bospectra.h"
#include "kernel/media/bospectra/core/bospectra_core.h"
#include "kernel/media/bospectra/frame_memory/frame_pool/frame_pool.h"
#include "kernel/media/bospectra/frame_memory/packet_pool/packet_pool.h"
#include "kernel/media/bospectra/color/include/bospectra_color.h"
#include "kernel/media/bospectra/render/include/bospectra_render.h"
#include "kernel/media/bospectra/playback/include/bospectra_playback.h"
#include "kernel/media/bospectra/debug/bospectra_debug.h"
#include "kernel/media/bospectra/include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

static PlayerCoreCtx g_player_core_instance;
static bool g_player_core_initialized = false;

PlayerCoreCtx* player_core_get_instance(void) {
    return g_player_core_initialized ? &g_player_core_instance : NULL;
}

bwe_error_t player_core_init(PlayerCoreCtx* ctx) {
    if (!ctx) return 1;

    memset(ctx, 0, sizeof(PlayerCoreCtx));
    ctx->app.width = BOS_PLAYER_DEFAULT_W;
    ctx->app.height = BOS_PLAYER_DEFAULT_H;
    ctx->app.view_mode = PLAYER_VIEW_MODE_NORMAL;
    ctx->app.is_running = true;

    /* Initialize all BOSPECTRA Engine Subsystems */
    bospectra_trace_str("TRACE 2 — BOSPECTRA Subsystem Bootstrapping", "START");
    bospectra_error_t e1 = BOSPECTRA_Init();
    bospectra_trace_str("Frame Pool Engine", "READY");
    bospectra_frame_pool_init();
    bospectra_trace_str("Packet Pool Engine", "READY");
    bospectra_packet_pool_init();
    bospectra_trace_str("File Engine", "READY");
    bospectra_error_t e2 = BOSPECTRA_Color_Init();
    bospectra_trace_str("Color Engine", e2 == BOSPECTRA_SUCCESS ? "READY" : "ALREADY_INITIALIZED");
    bospectra_error_t e3 = BOSPECTRA_Render_Init();
    bospectra_trace_str("Render Engine", e3 == BOSPECTRA_SUCCESS ? "READY" : "ALREADY_INITIALIZED");
    bospectra_error_t e4 = BOSPECTRA_Playback_Init();
    bospectra_trace_str("Playback Engine", e4 == BOSPECTRA_SUCCESS ? "READY" : "ALREADY_INITIALIZED");
    bospectra_trace_str("TRACE 2 — BOSPECTRA Subsystem Bootstrapping", "COMPLETE");

    g_player_core_instance = *ctx;
    g_player_core_initialized = true;

    return BWE_SUCCESS;
}

void player_core_shutdown(PlayerCoreCtx* ctx) {
    if (!ctx || !ctx->app.is_running) return;

    if (ctx->is_session_open && ctx->playback_session_id != 0) {
        BOSPECTRA_Playback_Stop(ctx->playback_session_id);
        BOSPECTRA_Playback_CloseSession(ctx->playback_session_id);
        ctx->playback_session_id = 0;
        ctx->is_session_open = false;
    }

    ctx->app.is_running = false;
    g_player_core_initialized = false;
}

#include "kernel/media/bospectra/manager/media_manager.h"

extern void display_print(const char* str);

bwe_error_t player_core_open_media(PlayerCoreCtx* ctx, const char* media_path) {
    if (!ctx || !media_path) return 1;

    if (ctx->is_session_open && ctx->playback_session_id != 0) {
        bospectra_media_close(ctx->playback_session_id);
        ctx->playback_session_id = 0;
        ctx->is_session_open = false;
    }

    strncpy(ctx->app.current_file, media_path, sizeof(ctx->app.current_file) - 1);

    bospectra_error_t err = bospectra_media_open(media_path, &ctx->playback_session_id);
    if (err != BOSPECTRA_SUCCESS) {
        return 1;
    }

    ctx->is_session_open = true;
    bospectra_media_play(ctx->playback_session_id);
    display_print("[BOSPECTRA:PLAYBACK] Media Manager Auto-Playback Started!\n");
    return BWE_SUCCESS;
}

bwe_error_t player_core_toggle_play_pause(PlayerCoreCtx* ctx) {
    if (!ctx || !ctx->is_session_open || ctx->playback_session_id == 0) return 1;

    bospectra_playback_state_t st = BOSPECTRA_Playback_GetState(ctx->playback_session_id);
    if (st == BOSPECTRA_PLAYBACK_STATE_PLAYING) {
        BOSPECTRA_Playback_Pause(ctx->playback_session_id);
    } else {
        BOSPECTRA_Playback_Play(ctx->playback_session_id);
    }

    return BWE_SUCCESS;
}

bwe_error_t player_core_stop(PlayerCoreCtx* ctx) {
    if (!ctx || !ctx->is_session_open || ctx->playback_session_id == 0) return 1;
    BOSPECTRA_Playback_Stop(ctx->playback_session_id);
    return BWE_SUCCESS;
}

bwe_error_t player_core_seek(PlayerCoreCtx* ctx, uint64_t position_us) {
    if (!ctx || !ctx->is_session_open || ctx->playback_session_id == 0) return 1;
    BOSPECTRA_Playback_Seek(ctx->playback_session_id, position_us);
    return BWE_SUCCESS;
}

bwe_error_t player_core_toggle_fullscreen(PlayerCoreCtx* ctx) {
    if (!ctx) return 1;

    if (ctx->app.view_mode == PLAYER_VIEW_MODE_NORMAL) {
        ctx->app.view_mode = PLAYER_VIEW_MODE_FULLSCREEN;
        ctx->app.width = BOS_PLAYER_FULLSCREEN_W;
        ctx->app.height = BOS_PLAYER_FULLSCREEN_H;
    } else {
        ctx->app.view_mode = PLAYER_VIEW_MODE_NORMAL;
        ctx->app.width = BOS_PLAYER_DEFAULT_W;
        ctx->app.height = BOS_PLAYER_DEFAULT_H;
    }

    return BWE_SUCCESS;
}
