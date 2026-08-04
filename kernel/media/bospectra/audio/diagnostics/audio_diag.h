#ifndef AUDIO_DIAG_H
#define AUDIO_DIAG_H

#include "../include/bospectra_audio.h"

void bospectra_audio_diag_init(void);
void bospectra_audio_diag_shutdown(void);
void bospectra_audio_diag_record_pcm(size_t bytes_played, bool success);
void bospectra_audio_collect_stats(BOSPECTRA_AudioStats* out_stats);
void bospectra_audio_dump_telemetry(void);

#endif // AUDIO_DIAG_H
