#ifndef ANIMATION_FADE_H
#define ANIMATION_FADE_H

#include "animation_engine.h"
#include "kernel/gui/surface/surface.h"

// Wrappers for starting common animations
void wallpaper_transition(struct BOSSurface* new_wallpaper, uint32_t duration_ms);

#endif
