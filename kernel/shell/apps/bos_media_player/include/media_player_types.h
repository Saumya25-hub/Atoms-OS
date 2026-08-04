#ifndef MEDIA_PLAYER_TYPES_H
#define MEDIA_PLAYER_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define BOS_PLAYER_MIN_WIDTH   900U
#define BOS_PLAYER_MIN_HEIGHT  600U
#define BOS_PLAYER_DEFAULT_W   1280U
#define BOS_PLAYER_DEFAULT_H   720U
#define BOS_PLAYER_FULLSCREEN_W 1920U
#define BOS_PLAYER_FULLSCREEN_H 1080U

typedef enum {
    PLAYER_VIEW_MODE_NORMAL = 0,
    PLAYER_VIEW_MODE_FULLSCREEN,
    PLAYER_VIEW_MODE_COMPACT
} bos_player_view_mode_t;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} BOS_Rect;

typedef struct {
    BOS_Rect play_pause_btn;
    BOS_Rect stop_btn;
    BOS_Rect prev_btn;
    BOS_Rect next_btn;
    BOS_Rect seekbar_track;
    BOS_Rect seekbar_thumb;
    BOS_Rect volume_track;
    BOS_Rect loop_btn;
    BOS_Rect speed_btn;
    BOS_Rect fullscreen_btn;
} BOS_PlayerControlLayout;

#endif // MEDIA_PLAYER_TYPES_H
