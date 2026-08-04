#ifndef BOSPECTRA_RENDER_H
#define BOSPECTRA_RENDER_H

#include "../../include/bospectra_types.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "bospectra_render_types.h"

typedef uint32_t bospectra_render_session_id_t;

typedef struct {
    uint32_t active_render_sessions;
    uint64_t total_frames_rendered;
    uint64_t total_frames_dropped;
    uint64_t last_render_time_us;
    uint64_t avg_render_time_us;
    uint64_t peak_render_time_us;
    char     active_backend_name[32];
} BOSPECTRA_RenderStats;

// Master Render Engine Subsystem Lifecycle APIs
bospectra_error_t BOSPECTRA_Render_Init(void);
bospectra_error_t BOSPECTRA_Render_Shutdown(void);

// Session & Frame Presentation APIs
bospectra_error_t BOSPECTRA_Render_OpenSession(const BOSPECTRA_RenderSpec* spec, bospectra_render_session_id_t* out_session_id);
bospectra_error_t BOSPECTRA_Render_PresentFrame(bospectra_render_session_id_t session_id, const BOSFrame* frame, int32_t x, int32_t y, int32_t w, int32_t h);
bospectra_error_t BOSPECTRA_Render_SetBackend(bospectra_render_session_id_t session_id, bospectra_render_backend_t backend);
bospectra_error_t BOSPECTRA_Render_GetSurfacePixels(bospectra_render_session_id_t session_id, uint32_t** out_pixels, uint32_t* out_w, uint32_t* out_h);
bospectra_error_t BOSPECTRA_Render_CloseSession(bospectra_render_session_id_t session_id);

// Telemetry & Diagnostics Query
void BOSPECTRA_Render_GetStats(BOSPECTRA_RenderStats* out_stats);
void BOSPECTRA_Render_DumpDiagnostics(void);

#endif // BOSPECTRA_RENDER_H
