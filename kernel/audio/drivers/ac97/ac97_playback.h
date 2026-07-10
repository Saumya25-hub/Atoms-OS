#ifndef AC97_PLAYBACK_H
#define AC97_PLAYBACK_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AC97_PB_STATE_UNINITIALIZED,
    AC97_PB_STATE_PREPARED,
    AC97_PB_STATE_READY,
    AC97_PB_STATE_STARTING,
    AC97_PB_STATE_RUNNING,
    AC97_PB_STATE_STOPPING,
    AC97_PB_STATE_STOPPED,
    AC97_PB_STATE_ERROR
} Ac97PlaybackState;

typedef struct {
    uint32_t frames_played;
    uint32_t bytes_sent;
    uint32_t underruns;
    uint32_t restarts;
} Ac97PlaybackTelemetry;

bool ac97_playback_init(void);
void ac97_playback_shutdown(void);
bool ac97_playback_prepare(void);
bool ac97_playback_start(void);
void ac97_playback_stop(void);
void ac97_playback_update(void);
void ac97_playback_status(void);

void ac97_playback_run_stress_test(void);

uint32_t ac97_playback_get_rotations(void);
uint32_t ac97_playback_get_dch_halts(void);
uint32_t ac97_playback_get_underruns(void);
uint32_t ac97_playback_get_bytes_sent(void);
uint32_t ac97_playback_get_frames_played(void);
uint32_t ac97_playback_get_late_refills(void);
Ac97PlaybackState ac97_playback_get_state(void);
void ac97_playback_dump_trace(void);

#endif // AC97_PLAYBACK_H
