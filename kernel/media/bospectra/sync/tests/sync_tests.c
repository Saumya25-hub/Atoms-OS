#include "sync_tests.h"
#include "../include/bospectra_sync.h"
#include "../clock/master_clock.h"
#include "../scheduler/frame_scheduler.h"
#include "../drift/drift_detector.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_sync_test_master_clock(void) {
    bospectra_log("TEST", "Running Master Presentation Clock Test...");

    master_clock_reset(1000000); // 1.0 sec start PTS
    uint64_t t1 = master_clock_get_time_us();

    master_clock_update_audio_pts(2000000); // 2.0 sec Audio PTS
    uint64_t t2 = master_clock_get_time_us();

    if (t2 <= t1) {
        bospectra_log("TEST_FAIL", "Master clock regression test failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Master Presentation Clock Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_sync_test_frame_scheduler(void) {
    bospectra_log("TEST", "Running Frame Presentation Scheduler Test...");

    uint64_t clock_us = 1000000;

    // Test on-time frame (+0ms) -> PRESENT
    bospectra_sync_decision_t d1 = frame_scheduler_evaluate(1000000, clock_us);
    if (d1 != BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY) {
        bospectra_log("TEST_FAIL", "On-time frame scheduler decision failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Test late frame (-50ms) -> DROP
    bospectra_sync_decision_t d2 = frame_scheduler_evaluate(950000, clock_us);
    if (d2 != BOSPECTRA_SYNC_DECISION_DROP_FRAME) {
        bospectra_log("TEST_FAIL", "Late frame drop decision failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Test future frame (+100ms) -> WAIT
    bospectra_sync_decision_t d3 = frame_scheduler_evaluate(1100000, clock_us);
    if (d3 != BOSPECTRA_SYNC_DECISION_WAIT) {
        bospectra_log("TEST_FAIL", "Future frame wait decision failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Frame Presentation Scheduler Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_sync_test_drift_detector(void) {
    bospectra_log("TEST", "Running A/V Drift Detector Test...");

    drift_detector_reset();
    drift_detector_update(1000000, 1020000); // Video 20ms ahead

    int64_t drift = drift_detector_get_current_drift_us();
    if (drift != 20000) {
        bospectra_log("TEST_FAIL", "Drift detection math failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "A/V Drift Detector Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_sync_test_1k_sync_stress(void) {
    bospectra_log("TEST", "Running 1,000 Frame A/V Sync Stress Test...");

    master_clock_reset(0);

    for (uint64_t i = 0; i < 1000; i++) {
        uint64_t frame_pts = i * 33333; // 30 FPS stream
        master_clock_update_audio_pts(frame_pts);
        bospectra_sync_decision_t dec = BOSPECTRA_Sync_EvaluateFrame(frame_pts);

        if (dec != BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY) {
            bospectra_log("TEST_FAIL", "1K Sync stress frame evaluation mismatch!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
    }

    bospectra_log("TEST_PASS", "1,000 Frame A/V Sync Stress Test PASSED.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_sync_tests_run_all(void) {
    display_print("========= BOSPECTRA A/V SYNC ENGINE TESTS =========\n");

    bospectra_error_t res = bospectra_sync_test_master_clock();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_sync_test_frame_scheduler();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_sync_test_drift_detector();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_sync_test_1k_sync_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:SYNC_TEST] All A/V Sync Self-Tests PASSED!\n");
    display_print("===================================================\n");

    return BOSPECTRA_SUCCESS;
}
