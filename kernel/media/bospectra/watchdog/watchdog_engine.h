/*
 * BOSPECTRA V3 — Watchdog Engine Subsystem
 * kernel/media/bospectra/watchdog/watchdog_engine.h
 */

#ifndef BOSPECTRA_V3_WATCHDOG_ENGINE_H
#define BOSPECTRA_V3_WATCHDOG_ENGINE_H

#include "../playback/session/playback_session.h"

void              bospectra_watchdog_engine_init(void);
void              bospectra_watchdog_engine_shutdown(void);

bospectra_error_t bospectra_watchdog_register_session(PlaybackSessionCtx* sess);
void              bospectra_watchdog_tick(void);

#endif /* BOSPECTRA_V3_WATCHDOG_ENGINE_H */
