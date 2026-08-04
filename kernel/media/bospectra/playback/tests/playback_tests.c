#include "playback_tests.h"
#include "../include/bospectra_playback.h"
#include "../state/playback_state.h"
#include "../timeline/playback_timeline.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_playback_test_state_machine(void) {
    bospectra_log("TEST", "Running Playback State Machine Test...");

    BOSPECTRA_StateMachine sm;
    playback_state_init(&sm);

    // Test valid IDLE -> OPENING transition
    if (playback_state_transition(&sm, BOSPECTRA_PLAYBACK_STATE_OPENING) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Valid IDLE -> OPENING state transition failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Test invalid OPENING -> PAUSED transition rejection
    if (playback_state_transition(&sm, BOSPECTRA_PLAYBACK_STATE_PAUSED) == BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Invalid OPENING -> PAUSED transition accepted unexpectedly!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Playback State Machine Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_playback_test_timeline(void) {
    bospectra_log("TEST", "Running Microsecond Timeline Test...");

    BOSPECTRA_Timeline tl;
    playback_timeline_init(&tl, 60000000ULL); // 60 seconds

    playback_timeline_update_position(&tl, 30000000ULL); // 30 seconds
    BOSPECTRA_TimelineInfo info;
    playback_timeline_get_info(&tl, &info);

    if (info.progress_percent_x10 != 500) { // 50.0%
        bospectra_log("TEST_FAIL", "Timeline percent math failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Microsecond Timeline Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_playback_test_rapid_controls(void) {
    bospectra_log("TEST", "Running Rapid Play/Pause/Seek Control Test...");

    bospectra_playback_session_id_t sess_id = 0;
    if (BOSPECTRA_Playback_OpenSession("test_stream.mp4", &sess_id) != BOSPECTRA_SUCCESS || sess_id == 0) {
        bospectra_log("TEST_FAIL", "Failed to open playback session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPECTRA_Playback_Play(sess_id);
    BOSPECTRA_Playback_Pause(sess_id);
    BOSPECTRA_Playback_Resume(sess_id);
    BOSPECTRA_Playback_Seek(sess_id, 15000000ULL);
    BOSPECTRA_Playback_SetSpeed(sess_id, 150); // 1.5x
    BOSPECTRA_Playback_SetLoop(sess_id, true);
    BOSPECTRA_Playback_StepFrame(sess_id);
    BOSPECTRA_Playback_Stop(sess_id);

    BOSPECTRA_Playback_CloseSession(sess_id);
    bospectra_log("TEST_PASS", "Rapid Play/Pause/Seek Control Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_playback_test_1k_session_stress(void) {
    bospectra_log("TEST", "Running 1,000 Session Open/Play/Close Stress Test...");

    for (int i = 0; i < 1000; i++) {
        bospectra_playback_session_id_t sess_id = 0;
        if (BOSPECTRA_Playback_OpenSession("stress_stream.mkv", &sess_id) != BOSPECTRA_SUCCESS) {
            bospectra_log("TEST_FAIL", "1K Session stress open failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }

        BOSPECTRA_Playback_Play(sess_id);
        BOSPECTRA_Playback_StepFrame(sess_id);
        BOSPECTRA_Playback_CloseSession(sess_id);
    }

    bospectra_log("TEST_PASS", "1,000 Session Open/Play/Close Stress Test PASSED.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_playback_tests_run_all(void) {
    display_print("========= BOSPECTRA PLAYBACK CONTROLLER TESTS =========\n");

    bospectra_error_t res = bospectra_playback_test_state_machine();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_playback_test_timeline();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_playback_test_rapid_controls();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_playback_test_1k_session_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:PLAYBACK_TEST] All Playback Controller Self-Tests PASSED!\n");
    display_print("========================================================\n");

    return BOSPECTRA_SUCCESS;
}
