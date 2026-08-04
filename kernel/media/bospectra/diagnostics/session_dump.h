/*
 * BOSPECTRA V3 — Session Dump Subsystem
 * kernel/media/bospectra/diagnostics/session_dump.h
 */

#ifndef BOSPECTRA_V3_SESSION_DUMP_H
#define BOSPECTRA_V3_SESSION_DUMP_H

#include "../playback/session/playback_session.h"

void bospectra_dump_session(const PlaybackSessionCtx* sess);

#endif /* BOSPECTRA_V3_SESSION_DUMP_H */
