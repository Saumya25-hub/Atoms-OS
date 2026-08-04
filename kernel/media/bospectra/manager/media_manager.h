/*
 * BOSPECTRA V3 — Media Manager
 * kernel/media/bospectra/manager/media_manager.h
 *
 * Top-Level Pipeline Orchestrator & Inspector.
 * Only orchestrates pipelines; never decodes, renders, or parses containers directly.
 */

#ifndef BOSPECTRA_MEDIA_MANAGER_H
#define BOSPECTRA_MEDIA_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "session_manager.h"
#include "container_manager.h"
#include "decoder_manager.h"
#include "render_manager.h"
#include "sync_manager.h"
#include "resource_manager.h"
#include <stdint.h>
#include <stdbool.h>

bospectra_error_t bospectra_media_manager_init(void);
bospectra_error_t bospectra_media_manager_shutdown(void);

bospectra_error_t bospectra_media_open(const char* media_path, bospectra_playback_session_id_t* out_session_id);
bospectra_error_t bospectra_media_play(bospectra_playback_session_id_t session_id);
bospectra_error_t bospectra_media_pause(bospectra_playback_session_id_t session_id);
bospectra_error_t bospectra_media_resume(bospectra_playback_session_id_t session_id);
bospectra_error_t bospectra_media_stop(bospectra_playback_session_id_t session_id);
bospectra_error_t bospectra_media_seek(bospectra_playback_session_id_t session_id, uint64_t target_position_us);
bospectra_error_t bospectra_media_close(bospectra_playback_session_id_t session_id);

void bospectra_media_inspect_pipeline(bospectra_playback_session_id_t session_id);

#endif /* BOSPECTRA_MEDIA_MANAGER_H */
