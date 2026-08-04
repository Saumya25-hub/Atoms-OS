#include "include/bospectra_audio.h"
#include "bridge/audio_bridge.h"
#include "session/audio_session.h"
#include "diagnostics/audio_diag.h"
#include "tests/audio_tests.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"

static bool g_audio_engine_initialized = false;

bospectra_error_t BOSPECTRA_Audio_Init(void) {
    if (g_audio_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    bospectra_audio_session_subsystem_init();
    bospectra_audio_bridge_init();
    bospectra_audio_diag_init();

    g_audio_engine_initialized = true;
    bospectra_log("AUDIO_ENGINE", "Audio Subsystem Initialized (Bridge linked to ATOMS OS AC97 HAL).");

    // Run self-tests and 1,000 PCM frame delivery stress test (disabled for instant boot)
    // bospectra_audio_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Audio_Shutdown(void) {
    if (!g_audio_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    bospectra_audio_diag_shutdown();
    bospectra_audio_bridge_shutdown();
    bospectra_audio_session_subsystem_shutdown();

    g_audio_engine_initialized = false;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Audio_OpenSession(const BOSPECTRA_AudioSpec* spec, bospectra_audio_session_id_t* out_session_id) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_audio_session_open(spec, out_session_id);
}

bospectra_error_t BOSPECTRA_Audio_Play(bospectra_audio_session_id_t session_id) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_audio_session_play(session_id);
}

bospectra_error_t BOSPECTRA_Audio_Pause(bospectra_audio_session_id_t session_id) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_audio_session_pause(session_id);
}

bospectra_error_t BOSPECTRA_Audio_Resume(bospectra_audio_session_id_t session_id) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_audio_session_resume(session_id);
}

bospectra_error_t BOSPECTRA_Audio_SubmitPacket(bospectra_audio_session_id_t session_id, const BOSPacket* packet) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!packet || !packet->data || packet->size == 0) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    return BOSPECTRA_Audio_SubmitPCM(session_id, packet->data, packet->size);
}

bospectra_error_t BOSPECTRA_Audio_SubmitPCM(bospectra_audio_session_id_t session_id, const uint8_t* pcm_data, size_t size_bytes) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!pcm_data || size_bytes == 0) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    BOSPECTRA_AudioSession* sess = bospectra_audio_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    size_t pushed = 0;
    bospectra_error_t err = bospectra_audio_queue_push_pcm(&sess->queue, pcm_data, size_bytes, &pushed);
    
    // Transmit PCM frame buffer into OS Audio HAL via Audio Bridge
    bospectra_audio_bridge_write_pcm(pcm_data, size_bytes, &sess->spec);

    bool success = (err == BOSPECTRA_SUCCESS || err == BOSPECTRA_ERR_BUFFER_OVERFLOW);
    bospectra_audio_diag_record_pcm(pushed, success);

    return err;
}

bospectra_error_t BOSPECTRA_Audio_Flush(bospectra_audio_session_id_t session_id) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_audio_session_flush(session_id);
}

bospectra_error_t BOSPECTRA_Audio_CloseSession(bospectra_audio_session_id_t session_id) {
    if (!g_audio_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_audio_session_close(session_id);
}

void BOSPECTRA_Audio_GetStats(BOSPECTRA_AudioStats* out_stats) {
    bospectra_audio_collect_stats(out_stats);
}

void BOSPECTRA_Audio_DumpDiagnostics(void) {
    bospectra_audio_dump_telemetry();
}
