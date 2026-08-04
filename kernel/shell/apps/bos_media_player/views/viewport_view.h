#ifndef VIEWPORT_VIEW_H
#define VIEWPORT_VIEW_H

#include "../include/media_player_types.h"
#include "kernel/wm/bwe/include/bwe.h"

void viewport_view_render(const BVFramebuffer* fb, const BOS_Rect* bounds, bool has_active_video);

#endif // VIEWPORT_VIEW_H
