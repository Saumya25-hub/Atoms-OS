#ifndef PLAYBACK_SESSION_H
#define PLAYBACK_SESSION_H

#include "../include/bospectra_playback.h"
#include "../state/playback_state.h"
#include "../timeline/playback_timeline.h"
#include "../events/playback_events.h"
#include "../../container/include/bospectra_container.h"
#include "../../container/registry/container_registry.h"
#include "../../decoder/include/bospectra_decoder.h"
#include "../../decoder/registry/decoder_registry.h"
#include "../../audio/include/bospectra_audio.h"
#include "../../sync/include/bospectra_sync.h"
#include "../../render/include/bospectra_render.h"

#include "../../pipeline/pipeline_engine.h"
#include "../../scheduler/frame_scheduler.h"

typedef struct {
    bospectra_playback_session_id_t session_id;
    BOSPECTRA_StateMachine          state_machine;
    BOSPECTRA_Timeline              timeline;
    BOSPECTRA_EventBus              event_bus;
    bospectra_audio_session_id_t    audio_session_id;
    bospectra_render_session_id_t   render_session_id;
    uint64_t                        total_frames_decoded;
    uint64_t                        total_frames_rendered;
    uint64_t                        total_audio_bytes;
    bool                            is_active;
    char                            media_uri[128];
    /* Container / demux state */
    const BOSPECTRA_ContainerDriver* container_driver;
    void*                            container_ctx;
    bospectra_file_id_t              file_id;
    /* Decoder state */
    const BOSPECTRA_DecoderDriver*   decoder_driver;
    void*                            decoder_ctx;
    /* Render target (BWE window_id of the video canvas) */
    uint32_t                         target_window_id;
    uint32_t                         target_x;
    uint32_t                         target_y;
    uint32_t                         target_w;
    uint32_t                         target_h;
    /* V3 Staged Pipeline Context */
    BOSPECTRA_PipelineContext        pipeline_ctx;
    /* V3 Frame Scheduler & Master Presentation Clock Context */
    BOSPECTRA_FrameSchedulerContext  frame_scheduler_ctx;
} PlaybackSessionCtx;

void                playback_session_subsystem_init(void);
void                playback_session_subsystem_shutdown(void);
bospectra_error_t   playback_session_create(const char* media_uri, bospectra_playback_session_id_t* out_session_id);
bospectra_error_t   playback_session_set_render_target(bospectra_playback_session_id_t id, uint32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
PlaybackSessionCtx* playback_session_get_by_id(bospectra_playback_session_id_t id);
bospectra_error_t   playback_session_destroy(bospectra_playback_session_id_t id);
void                playback_session_tick(bospectra_playback_session_id_t id);

#endif // PLAYBACK_SESSION_H
