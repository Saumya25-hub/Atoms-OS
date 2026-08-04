#include "player_settings.h"
#include "kernel/core/lib/include/string.h"

void player_settings_init_defaults(BOS_PlayerSettings* settings) {
    if (!settings) return;
    memset(settings, 0, sizeof(BOS_PlayerSettings));
    settings->default_speed_x100 = 100; // 1.0x
    settings->auto_play_next = true;
    settings->hardware_acceleration = true;
    settings->volume_percent = 80;
}
