#include "playback_controls.h"
#include "kernel/core/lib/include/string.h"

extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void playback_controls_calculate_layout(const BOS_Rect* panel_bounds, BOS_PlayerControlLayout* out_layout) {
    if (!panel_bounds || !out_layout) return;

    memset(out_layout, 0, sizeof(BOS_PlayerControlLayout));

    int32_t margin = 10;
    int32_t cy = panel_bounds->y + (panel_bounds->h - 36) / 2;

    // Timeline seekbar track
    out_layout->seekbar_track.x = panel_bounds->x + margin;
    out_layout->seekbar_track.y = panel_bounds->y + 5;
    out_layout->seekbar_track.w = panel_bounds->w - (margin * 2);
    out_layout->seekbar_track.h = 6;

    // Play / Pause button
    out_layout->play_pause_btn.x = panel_bounds->x + (panel_bounds->w - 40) / 2;
    out_layout->play_pause_btn.y = cy;
    out_layout->play_pause_btn.w = 40;
    out_layout->play_pause_btn.h = 36;

    // Stop button
    out_layout->stop_btn.x = out_layout->play_pause_btn.x + 50;
    out_layout->stop_btn.y = cy;
    out_layout->stop_btn.w = 36;
    out_layout->stop_btn.h = 36;

    // Previous button
    out_layout->prev_btn.x = out_layout->play_pause_btn.x - 46;
    out_layout->prev_btn.y = cy;
    out_layout->prev_btn.w = 36;
    out_layout->prev_btn.h = 36;

    // Next button
    out_layout->next_btn.x = out_layout->stop_btn.x + 46;
    out_layout->next_btn.y = cy;
    out_layout->next_btn.w = 36;
    out_layout->next_btn.h = 36;

    // Fullscreen button
    out_layout->fullscreen_btn.x = panel_bounds->x + panel_bounds->w - 46;
    out_layout->fullscreen_btn.y = cy;
    out_layout->fullscreen_btn.w = 36;
    out_layout->fullscreen_btn.h = 36;
}

void playback_controls_render(const BVFramebuffer* fb, const BOS_PlayerControlLayout* layout, bool is_playing, uint32_t progress_percent_x10) {
    (void)is_playing;
    if (!fb || !layout) return;

    // Render Timeline Track (#2A3245)
    BWE_FillRect(fb, layout->seekbar_track.x, layout->seekbar_track.y, layout->seekbar_track.w, layout->seekbar_track.h, 0xFF2A3245);

    // Render Progress Fill (#0088FF)
    int32_t fill_w = (int32_t)((layout->seekbar_track.w * progress_percent_x10) / 1000U);
    if (fill_w > 0) {
        BWE_FillRect(fb, layout->seekbar_track.x, layout->seekbar_track.y, fill_w, layout->seekbar_track.h, 0xFF0088FF);
    }

    // Render Buttons
    BWE_FillRect(fb, layout->play_pause_btn.x, layout->play_pause_btn.y, layout->play_pause_btn.w, layout->play_pause_btn.h, 0xFF0088FF);
    BWE_FillRect(fb, layout->stop_btn.x, layout->stop_btn.y, layout->stop_btn.w, layout->stop_btn.h, 0xFF2A3245);
    BWE_FillRect(fb, layout->prev_btn.x, layout->prev_btn.y, layout->prev_btn.w, layout->prev_btn.h, 0xFF2A3245);
    BWE_FillRect(fb, layout->next_btn.x, layout->next_btn.y, layout->next_btn.w, layout->next_btn.h, 0xFF2A3245);
    BWE_FillRect(fb, layout->fullscreen_btn.x, layout->fullscreen_btn.y, layout->fullscreen_btn.w, layout->fullscreen_btn.h, 0xFF2A3245);
}

bool playback_controls_hittest(const BOS_Rect* btn_rect, int32_t mouse_x, int32_t mouse_y) {
    if (!btn_rect) return false;
    return (mouse_x >= btn_rect->x && mouse_x < (btn_rect->x + btn_rect->w) &&
            mouse_y >= btn_rect->y && mouse_y < (btn_rect->y + btn_rect->h));
}
