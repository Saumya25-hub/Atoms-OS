#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PLAYER_STATE_STOPPED,
    PLAYER_STATE_OPENED,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED
} AudioPlayerState;

void audio_player_open(const char* path);
void audio_player_play(void);
void audio_player_pause(void);
void audio_player_resume(void);
void audio_player_stop(void);
void audio_player_close(void);
void audio_player_update(void);
bool audio_player_is_playing(void);
void audio_player_get_diag_info(uint32_t* stream_id, uint32_t* bytes_played, uint32_t* data_size, uint64_t* read_time_ticks);

#endif // AUDIO_PLAYER_H
