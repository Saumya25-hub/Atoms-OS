#ifndef PLAYER_SETTINGS_H
#define PLAYER_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t default_speed_x100;
    bool     auto_play_next;
    bool     hardware_acceleration;
    uint32_t volume_percent;
} BOS_PlayerSettings;

void player_settings_init_defaults(BOS_PlayerSettings* settings);

#endif // PLAYER_SETTINGS_H
