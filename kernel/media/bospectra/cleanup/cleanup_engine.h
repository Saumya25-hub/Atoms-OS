/*
 * BOSPECTRA V3 — Cleanup Engine Subsystem
 * kernel/media/bospectra/cleanup/cleanup_engine.h
 */

#ifndef BOSPECTRA_V3_CLEANUP_ENGINE_H
#define BOSPECTRA_V3_CLEANUP_ENGINE_H

#include "../playback/session/playback_session.h"

void              bospectra_cleanup_engine_init(void);
void              bospectra_cleanup_engine_shutdown(void);

bospectra_error_t bospectra_cleanup_destroy_session(PlaybackSessionCtx* sess);

#endif /* BOSPECTRA_V3_CLEANUP_ENGINE_H */
