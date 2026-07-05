#include "animation_fade.h"
#include "kernel/shell/desktop_shell/desktop_shell.h"

static void wallpaper_fade_update(uint32_t anim_id, int32_t current_val, void* user_data) {
    (void)anim_id;
    (void)user_data;
    
    // current_val is 0 to 1000. Map it to 0 to 255.
    uint32_t alpha = (current_val * 255) / 1000;
    if (alpha > 255) alpha = 255;
    
    desktop_set_wallpaper_alpha(alpha);
    desktop_refresh_background();
}

static void wallpaper_fade_complete(uint32_t anim_id, void* user_data) {
    (void)anim_id;
    (void)user_data;
    desktop_end_wallpaper_transition();
    desktop_refresh_background();
}

void wallpaper_transition(struct BOSSurface* new_wallpaper, uint32_t duration_ms) {
    struct BOSSurface* old_wallpaper = desktop_get_wallpaper();
    
    if (!old_wallpaper) {
        desktop_set_wallpaper(new_wallpaper);
        desktop_refresh_background();
        return;
    }
    
    desktop_set_wallpaper_transition(old_wallpaper, new_wallpaper);
    
    // start 0 to 1000
    uint32_t anim_id = animation_start(ANIM_TYPE_WALLPAPER_TRANSITION, 0, 1000, duration_ms, EASING_EASE_IN_OUT);
    
    if (anim_id > 0) {
        animation_set_update_callback(anim_id, wallpaper_fade_update, 0);
        animation_set_complete_callback(anim_id, wallpaper_fade_complete, 0);
    } else {
        // Fallback if max animations reached
        desktop_end_wallpaper_transition();
        desktop_set_wallpaper(new_wallpaper);
        desktop_refresh_background();
    }
}
