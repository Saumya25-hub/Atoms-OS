#ifndef TOOLBAR_VIEW_H
#define TOOLBAR_VIEW_H

#include "../include/media_player_types.h"
#include "kernel/wm/bwe/include/bwe.h"

void toolbar_view_render(const BVFramebuffer* fb, const BOS_Rect* bounds, const char* title);

#endif // TOOLBAR_VIEW_H
