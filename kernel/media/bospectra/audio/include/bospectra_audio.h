#ifndef BOSPECTRA_AUDIO_H
#define BOSPECTRA_AUDIO_H

#include "../../include/bospectra_types.h"
#include "../../packet/bospectra_packet.h"
#include "bospectra_audio_types.h"

typedef uint32_t bospectra_audio_session_id_t;

typedef struct {
    uint32_t active_sessions;
    uint64_t total_packets_processed;
    uint64_t total_pcm_frames_submitted;
    uint64_t total_bytes_played;
    uint64_t underrun_count;
    uint32_t queue_fill_bytes;
    char     driver_status[32];
} BOSPECTRA_AudioStats;

// Master Audio Integration Subsystem Lifecycle APIs
bospectra_error_t BOSPECTRA_Audio_Init(void);
bospectra_error_t BOSPECTRA_Audio_Shutdown(void);

// Audio Session Management APIs
bospectra_error_t BOSPECTRA_Audio_OpenSession(const BOSPECTRA_AudioSpec* spec, bospectra_audio_session_id_t* out_session_id);
bospectra_error_t BOSPECTRA_Audio_Play(bospectra_audio_session_id_t session_id);
bospectra_error_t BOSPECTRA_Audio_Pause(bospectra_audio_session_id_t session_id);
bospectra_error_t BOSPECTRA_Audio_Resume(bospectra_audio_session_id_t session_id);
bospectra_error_t BOSPECTRA_Audio_SubmitPacket(bospectra_audio_session_id_t session_id, const BOSPacket* packet);
bospectra_error_t BOSPECTRA_Audio_SubmitPCM(bospectra_audio_session_id_t session_id, const uint8_t* pcm_data, size_t size_bytes);
bospectra_error_t BOSPECTRA_Audio_Flush(bospectra_audio_session_id_t session_id);
bospectra_error_t BOSPECTRA_Audio_CloseSession(bospectra_audio_session_id_t session_id);

// Telemetry & Diagnostics Query
void BOSPECTRA_Audio_GetStats(BOSPECTRA_AudioStats* out_stats);
void BOSPECTRA_Audio_DumpDiagnostics(void);

#endif // BOSPECTRA_AUDIO_H
