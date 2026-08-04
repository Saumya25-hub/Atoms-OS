#ifndef PLAYBACK_DIAG_H
#define PLAYBACK_DIAG_H

#include "../include/bospectra_playback.h"
#include "../session/playback_session.h"

void playback_diag_collect_stats(const PlaybackSessionCtx* sess, BOSPECTRA_PlaybackStats* out_stats);
void playback_diag_dump_telemetry(const PlaybackSessionCtx* sess);

#endif // PLAYBACK_DIAG_H
