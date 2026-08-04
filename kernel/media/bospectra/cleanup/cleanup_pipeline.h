/*
 * BOSPECTRA V3 — Cleanup Pipeline Subsystem
 * kernel/media/bospectra/cleanup/cleanup_pipeline.h
 */

#ifndef BOSPECTRA_V3_CLEANUP_PIPELINE_H
#define BOSPECTRA_V3_CLEANUP_PIPELINE_H

#include "../playback/session/playback_session.h"

void              bospectra_cleanup_pipeline_init(void);
void              bospectra_cleanup_pipeline_shutdown(void);

bospectra_error_t bospectra_cleanup_execute_reverse_teardown(PlaybackSessionCtx* sess);

#endif /* BOSPECTRA_V3_CLEANUP_PIPELINE_H */
