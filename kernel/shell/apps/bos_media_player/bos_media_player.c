/*
 * BOS Media Player — bos_media_player.c
 * Phase 10.1: Native Video Pipeline Integration
 *
 * Architecture:
 *   BOS Media Player -> Playback Controller -> AVI Demux -> MJPEG Decoder
 *                    -> Color Engine -> Rendering Engine -> BWE Window
 */

#include "include/bos_media_player.h"
#include "core/player_core.h"
#include "views/viewport_view.h"
#include "views/toolbar_view.h"
#include "controls/playback_controls.h"
#include "playlist/playlist_manager.h"
#include "library/media_library.h"
#include "recent/recent_history.h"
#include "settings/player_settings.h"
#include "diagnostics/player_diag.h"
#include "tests/player_tests.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/media/bospectra/debug/bospectra_debug.h"
#include "kernel/media/bospectra/playback/session/playback_session.h"

/* =======================================================================
 * Internal constants
 * ===================================================================== */
#define CTRL_PANEL_H    80
#define TOOLBAR_H       32
#define VP_X            0
#define VP_Y            TOOLBAR_H
#define VP_W            BOS_PLAYER_DEFAULT_W
#define VP_H            (BOS_PLAYER_DEFAULT_H - TOOLBAR_H - CTRL_PANEL_H)

/* =======================================================================
 * App state
 * ===================================================================== */
static PlayerCoreCtx       g_player_ctx;
static BOS_PlaylistManager g_playlist;
static BOS_RecentHistory   g_recent_history;
static BOS_PlayerSettings  g_player_settings;
static bool                g_app_running        = false;
static uint32_t            g_canvas_id          = 0;
static uint32_t            g_ctrl_canvas_id     = 0;
static BOS_PlayerControlLayout g_ctrl_layout;

/* Diagnostic Telemetry Counters */
static uint32_t g_trace_paint_calls = 0;
static uint32_t g_trace_blits = 0;
static uint32_t g_trace_tick_count = 0;

/* =======================================================================
 * Canvas paint callbacks (called by BWE Compositor every frame)
 * ===================================================================== */

/* Video viewport canvas callback */
static void on_paint_video(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    g_trace_paint_calls++;
    BWE_Window* win = BWE_GetWindow(canvas_id);

    bospectra_trace_str("TRACE 13 — Canvas on_paint_video", "Triggered");
    bospectra_trace_u32("Canvas ID", canvas_id);
    bospectra_trace_u32("Window ID", win ? win->id : 0);
    bospectra_trace_str("Visible", win ? "TRUE" : "FALSE");
    if (clip) {
        bospectra_trace_u32("Clip X", clip->x);
        bospectra_trace_u32("Clip Y", clip->y);
        bospectra_trace_u32("Clip W", clip->width);
        bospectra_trace_u32("Clip H", clip->height);
    }
    if (win) {
        bospectra_trace_u32("Screen Rect X", win->screen_bounds.x);
        bospectra_trace_u32("Screen Rect Y", win->screen_bounds.y);
        bospectra_trace_u32("Screen Rect W", win->screen_bounds.width);
        bospectra_trace_u32("Screen Rect H", win->screen_bounds.height);
    }

    if (!win || !fb || !g_app_running) {
        bospectra_trace_str("Canvas Redraw Skipped", "Window, FB, or App not ready");
        return;
    }

    PlaybackSessionCtx* sess = playback_session_get_by_id(g_player_ctx.playback_session_id);
    if (!sess || sess->render_session_id == 0) {
        bospectra_trace_str("Canvas Redraw Skipped", "Playback session or render session ID is 0");
        return;
    }

    uint32_t* pixels = NULL;
    uint32_t tex_w = 0, tex_h = 0;
    bospectra_error_t err = BOSPECTRA_Render_GetSurfacePixels(sess->render_session_id, &pixels, &tex_w, &tex_h);
    bospectra_trace_u32("Surface Pixels Query Result", (uint32_t)err);
    bospectra_trace_hex("Texture Pixels Address", (uint64_t)(uintptr_t)pixels);
    bospectra_trace_u32("Texture Width", tex_w);
    bospectra_trace_u32("Texture Height", tex_h);

    if (err == BOSPECTRA_SUCCESS && pixels != NULL && tex_w > 0 && tex_h > 0) {
        int32_t canvas_w = (int32_t)fb->width;
        int32_t canvas_h = (int32_t)fb->height;

        /* Calculate Aspect-Ratio Correct Letterbox / Pillarbox destination rectangle (Bug 3 Fix) */
        int32_t fit_w = canvas_w;
        int32_t fit_h = (canvas_w * (int32_t)tex_h) / (int32_t)tex_w;
        if (fit_h > canvas_h) {
            fit_h = canvas_h;
            fit_w = (canvas_h * (int32_t)tex_w) / (int32_t)tex_h;
        }

        int32_t dx = (canvas_w - fit_w) / 2;
        int32_t dy = (canvas_h - fit_h) / 2;
        int32_t dw = fit_w;
        int32_t dh = fit_h;

        /* Fill letterbox / pillarbox margins with solid black using stride-safe BWE_FillRect */
        if (dx > 0 || dy > 0 || dw < canvas_w || dh < canvas_h) {
            BWE_FillRect(fb, 0, 0, canvas_w, canvas_h, 0xFF000000U);
        }

        bospectra_trace_str("TRACE 14 — Bitmap Blit", "Immediately before BWE_DrawBitmap");
        bospectra_trace_hex("Framebuffer Address", (uint64_t)(uintptr_t)fb);
        bospectra_trace_hex("Pixels Address", (uint64_t)(uintptr_t)pixels);
        bospectra_trace_u32("Destination X", dx);
        bospectra_trace_u32("Destination Y", dy);
        bospectra_trace_u32("Destination W", dw);
        bospectra_trace_u32("Destination H", dh);
        bospectra_trace_u32("Source W", tex_w);
        bospectra_trace_u32("Source H", tex_h);
        bospectra_trace_u32("Byte Pitch", tex_w * 4);

        BWE_DrawBitmap(fb, pixels, dx, dy, dw, dh, 0, 0, (int32_t)tex_w, (int32_t)tex_h, (int32_t)(tex_w * 4));
        g_trace_blits++;
        bospectra_trace_str("TRACE 14 — Bitmap Blit Result", "SUCCESS");
    } else {
        bospectra_trace_str("TRACE 14 — Bitmap Blit Result", "FAILED (Invalid Pixels or Zero Dimensions)");
    }
}

/* Control panel canvas — draws seekbar + buttons */
static void on_paint_controls(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* win = BWE_GetWindow(canvas_id);
    if (!win || !fb) return;

    BOS_Rect ctrl_bounds = {
        .x = win->screen_bounds.x,
        .y = win->screen_bounds.y,
        .w = win->screen_bounds.width,
        .h = win->screen_bounds.height
    };

    playback_controls_calculate_layout(&ctrl_bounds, &g_ctrl_layout);

    bool is_playing = g_player_ctx.is_session_open;
    uint32_t progress = 0;
    PlaybackSessionCtx* sess = playback_session_get_by_id(g_player_ctx.playback_session_id);
    if (sess && sess->timeline.duration_us > 0) {
        progress = (uint32_t)((sess->timeline.current_position_us * 1000ULL) / sess->timeline.duration_us);
    }

    playback_controls_render(fb, &g_ctrl_layout, is_playing, progress);
}

/* Toolbar canvas — draws title bar area */
static void on_paint_toolbar(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* win = BWE_GetWindow(canvas_id);
    if (!win || !fb) return;

    BOS_Rect tb_bounds = {
        .x = win->screen_bounds.x,
        .y = win->screen_bounds.y,
        .w = win->screen_bounds.width,
        .h = win->screen_bounds.height
    };

    toolbar_view_render(fb, &tb_bounds, "DOLBY.avi  —  640×360  MJPEG  24 FPS");
}

/* =======================================================================
 * Public API
 * ===================================================================== */

bwe_error_t bos_media_player_launch(uint32_t* out_win_id) {
    if (g_app_running) {
        if (out_win_id) *out_win_id = g_player_ctx.app.window_id;
        return BWE_SUCCESS;
    }

    extern void display_print(const char*);
    display_print("[BOS MEDIA PLAYER] Launching flagship multimedia application...\n");

    /* Subsystem init */
    player_core_init(&g_player_ctx);
    playlist_init(&g_playlist);
    recent_history_init(&g_recent_history);
    player_settings_init_defaults(&g_player_settings);
    player_diag_init();

    /* Create main application window */
    bwe_error_t err = BOS_CreateWindow(100, 50,
                                        BOS_PLAYER_DEFAULT_W,
                                        BOS_PLAYER_DEFAULT_H,
                                        "BOS Media Player",
                                        &g_player_ctx.app.window_id);
    if (err != BWE_SUCCESS) {
        player_core_shutdown(&g_player_ctx);
        return err;
    }

    uint32_t win_id = g_player_ctx.app.window_id;

    /* --- Toolbar Canvas (top 32 px) --- */
    uint32_t tb_canvas_id = 0;
    BOS_CreateCanvas(win_id,
                     0, 0,
                     (uint32_t)BOS_PLAYER_DEFAULT_W, TOOLBAR_H,
                     on_paint_toolbar,
                     &tb_canvas_id);

    /* --- Video Viewport Canvas --- */
    BOS_CreateCanvas(win_id,
                     (uint32_t)VP_X, (uint32_t)VP_Y,
                     (uint32_t)VP_W, (uint32_t)VP_H,
                     on_paint_video,
                     &g_canvas_id);

    /* --- Control Panel Canvas (bottom 80 px) --- */
    BOS_CreateCanvas(win_id,
                     0, (uint32_t)(BOS_PLAYER_DEFAULT_H - CTRL_PANEL_H),
                     (uint32_t)BOS_PLAYER_DEFAULT_W, (uint32_t)CTRL_PANEL_H,
                     on_paint_controls,
                     &g_ctrl_canvas_id);

    /* Scan media directory */
    media_library_scan_vfs(&g_playlist, "/Media");

    /* Open video from FAT32 root — DOLBY.AVI is baked into the disk image */
    /* Try multiple path formats: FAT32 uses uppercase 8.3, VFS may need different formats */
    if (player_core_open_media(&g_player_ctx, "DOLBY.AVI") != BWE_SUCCESS) {
        if (player_core_open_media(&g_player_ctx, "/DOLBY.AVI") != BWE_SUCCESS) {
            if (player_core_open_media(&g_player_ctx, "DOLBY.avi") != BWE_SUCCESS) {
                player_core_open_media(&g_player_ctx, "/Media/DOLBY.avi");
            }
        }
    }

    if (g_player_ctx.is_session_open && g_player_ctx.playback_session_id != 0) {
        playback_session_set_render_target(g_player_ctx.playback_session_id, g_canvas_id, 0, 0, VP_W, VP_H);
    }

    g_app_running = true;
    if (out_win_id) *out_win_id = win_id;

    BOS_Show(win_id);
    bospectra_trace_str("TRACE 1 — Media Player Launch", "SUCCESS");
    bospectra_trace_hex("Player Context Address", (uint64_t)(uintptr_t)&g_player_ctx);
    bospectra_trace_u32("Window ID", win_id);
    bospectra_trace_u32("Canvas ID", g_canvas_id);
    bospectra_trace_u32("Control Canvas ID", g_ctrl_canvas_id);

    return BWE_SUCCESS;
}

bwe_error_t bos_media_player_close(void) {
    if (!g_app_running) return BWE_SUCCESS;

    extern void display_print(const char*);
    display_print("[BOS MEDIA PLAYER] Closing window & tearing down media session cleanly...\n");

    player_diag_shutdown();
    player_core_shutdown(&g_player_ctx);

    if (g_player_ctx.app.window_id != 0) {
        BOS_DestroySurface(g_player_ctx.app.window_id);
        g_player_ctx.app.window_id = 0;
    }

    g_app_running = false;
    g_canvas_id   = 0;
    return BWE_SUCCESS;
}

/*
 * bos_media_player_tick() — called every compositor frame.
 * Ticks the BOSPECTRA native playback pipeline (Demux -> Decode -> Color -> Render).
 */
void bos_media_player_tick(void) {
    if (!g_app_running || g_canvas_id == 0) return;

    g_trace_tick_count++;

    if (g_player_ctx.is_session_open && g_player_ctx.playback_session_id != 0) {
        playback_session_tick(g_player_ctx.playback_session_id);
        BWE_InvalidateWindow(g_canvas_id);
    }

    // TRACE 17 — End of Every Second Telemetry Summary (every ~60 ticks)
    if (g_trace_tick_count % 60 == 0) {
        PlaybackSessionCtx* sess = playback_session_get_by_id(g_player_ctx.playback_session_id);
        extern void display_print(const char*);
        display_print("\n======== VIDEO PIPELINE ========\n");
        bospectra_trace_u32("Session ID", g_player_ctx.playback_session_id);
        bospectra_trace_str("Playback State", (sess && sess->state_machine.current_state == BOSPECTRA_PLAYBACK_STATE_PLAYING) ? "PLAYING" : "PAUSED/STOPPED");
        bospectra_trace_u32("Total Frames Decoded", sess ? (uint32_t)sess->total_frames_decoded : 0);
        bospectra_trace_u32("Total Frames Rendered", sess ? (uint32_t)sess->total_frames_rendered : 0);
        bospectra_trace_u32("Canvas Paint Calls", g_trace_paint_calls);
        bospectra_trace_u32("Bitmap Blits", g_trace_blits);
        display_print("==============================\n\n");
    }
}
