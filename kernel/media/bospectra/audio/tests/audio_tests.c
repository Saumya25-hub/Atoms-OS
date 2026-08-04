#include "audio_tests.h"
#include "../include/bospectra_audio.h"
#include "../bridge/audio_bridge.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_audio_test_session_lifecycle(void) {
    bospectra_log("TEST", "Running Audio Session Lifecycle Test...");

    BOSPECTRA_AudioSpec spec;
    spec.format = BOSPECTRA_AUDIO_FORMAT_PCM_S16LE;
    spec.sample_rate = 44100;
    spec.channels = 2;
    spec.bits_per_sample = 16;

    bospectra_audio_session_id_t sess_id = 0;
    if (BOSPECTRA_Audio_OpenSession(&spec, &sess_id) != BOSPECTRA_SUCCESS || sess_id == 0) {
        bospectra_log("TEST_FAIL", "Failed to open audio session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (BOSPECTRA_Audio_Play(sess_id) != BOSPECTRA_SUCCESS) {
        BOSPECTRA_Audio_CloseSession(sess_id);
        bospectra_log("TEST_FAIL", "Failed to play audio session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (BOSPECTRA_Audio_Pause(sess_id) != BOSPECTRA_SUCCESS) {
        BOSPECTRA_Audio_CloseSession(sess_id);
        bospectra_log("TEST_FAIL", "Failed to pause audio session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (BOSPECTRA_Audio_Resume(sess_id) != BOSPECTRA_SUCCESS) {
        BOSPECTRA_Audio_CloseSession(sess_id);
        bospectra_log("TEST_FAIL", "Failed to resume audio session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (BOSPECTRA_Audio_Flush(sess_id) != BOSPECTRA_SUCCESS) {
        BOSPECTRA_Audio_CloseSession(sess_id);
        bospectra_log("TEST_FAIL", "Failed to flush audio session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPECTRA_Audio_CloseSession(sess_id);
    bospectra_log("TEST_PASS", "Audio Session Lifecycle Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_test_queue_fifo(void) {
    bospectra_log("TEST", "Running Audio Queue FIFO Test...");

    BOSPECTRA_AudioSpec spec;
    spec.format = BOSPECTRA_AUDIO_FORMAT_PCM_S16LE;
    spec.sample_rate = 44100;
    spec.channels = 2;
    spec.bits_per_sample = 16;

    bospectra_audio_session_id_t sess_id = 0;
    BOSPECTRA_Audio_OpenSession(&spec, &sess_id);

    uint8_t pcm_data[256];
    memset(pcm_data, 0xA5, sizeof(pcm_data));

    if (BOSPECTRA_Audio_SubmitPCM(sess_id, pcm_data, sizeof(pcm_data)) != BOSPECTRA_SUCCESS) {
        BOSPECTRA_Audio_CloseSession(sess_id);
        bospectra_log("TEST_FAIL", "Failed to submit PCM data!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPECTRA_Audio_CloseSession(sess_id);
    bospectra_log("TEST_PASS", "Audio Queue FIFO Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_test_bridge_transmission(void) {
    bospectra_log("TEST", "Running Audio Bridge Hardware Transmission Test...");

    if (!bospectra_audio_bridge_is_hardware_ready()) {
        bospectra_log("TEST_FAIL", "Audio bridge not initialized!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    uint8_t pcm_buf[512];
    memset(pcm_buf, 0x12, sizeof(pcm_buf));

    BOSPECTRA_AudioSpec spec;
    spec.format = BOSPECTRA_AUDIO_FORMAT_PCM_S16LE;
    spec.sample_rate = 44100;
    spec.channels = 2;
    spec.bits_per_sample = 16;

    if (bospectra_audio_bridge_write_pcm(pcm_buf, sizeof(pcm_buf), &spec) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Failed to transmit PCM through Audio Bridge!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Audio Bridge Hardware Transmission Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_test_1k_pcm_stress(void) {
    bospectra_log("TEST", "Running 1,000 PCM Frame Audio Stress Test...");

    BOSPECTRA_AudioSpec spec;
    spec.format = BOSPECTRA_AUDIO_FORMAT_PCM_S16LE;
    spec.sample_rate = 44100;
    spec.channels = 2;
    spec.bits_per_sample = 16;

    bospectra_audio_session_id_t sess_id = 0;
    BOSPECTRA_Audio_OpenSession(&spec, &sess_id);

    uint8_t pcm_chunk[256];
    memset(pcm_chunk, 0x7E, sizeof(pcm_chunk));

    for (int i = 0; i < 1000; i++) {
        if (BOSPECTRA_Audio_SubmitPCM(sess_id, pcm_chunk, sizeof(pcm_chunk)) != BOSPECTRA_SUCCESS) {
            BOSPECTRA_Audio_CloseSession(sess_id);
            bospectra_log("TEST_FAIL", "1K PCM Frame Audio Stress failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
    }

    BOSPECTRA_Audio_CloseSession(sess_id);
    bospectra_log("TEST_PASS", "1,000 PCM Frame Audio Stress Test PASSED.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_tests_run_all(void) {
    display_print("========= BOSPECTRA AUDIO ENGINE INTEGRATION TESTS =========\n");

    bospectra_error_t res = bospectra_audio_test_session_lifecycle();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_audio_test_queue_fifo();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_audio_test_bridge_transmission();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_audio_test_1k_pcm_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:AUDIO_TEST] All Audio Engine Integration Tests PASSED!\n");
    display_print("============================================================\n");

    return BOSPECTRA_SUCCESS;
}
