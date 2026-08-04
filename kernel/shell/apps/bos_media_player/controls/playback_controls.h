#ifndef PLAYBACK_CONTROLS_H
#define PLAYBACK_CONTROLS_H

#include "../include/media_player_types.h"
#include "kernel/wm/bwe/include/bwe.h"

void playback_controls_calculate_layout(const BOS_Rect* panel_bounds, BOS_PlayerControlLayout* out_layout);
void playback_controls_render(const BVFramebuffer* fb, const BOS_PlayerControlLayout* layout, bool is_playing, uint32_t progress_percent_x10);
bool playback_controls_hittest(const BOS_Rect* btn_rect, int32_t mouse_x, int32_t mouse_y);

#endif // PLAYBACK_CONTROLS_H
