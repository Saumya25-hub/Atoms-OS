#include "render_tests.h"
#include "../include/bospectra_render.h"
#include "../texture_pool/texture_pool.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_render_test_session_lifecycle(void) {
    bospectra_log("TEST", "Running Render Session Lifecycle Test...");

    BOSPECTRA_RenderSpec spec;
    spec.backend = BOSPECTRA_RENDER_BACKEND_SOFTWARE;
    spec.filter = BOSPECTRA_SCALING_FILTER_BILINEAR;
    spec.rotation = BOSPECTRA_ROTATION_0;
    spec.target_window_id = 100;
    spec.flags = 0;

    bospectra_render_session_id_t sess_id = 0;
    if (BOSPECTRA_Render_OpenSession(&spec, &sess_id) != BOSPECTRA_SUCCESS || sess_id == 0) {
        bospectra_log("TEST_FAIL", "Failed to open render session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (BOSPECTRA_Render_CloseSession(sess_id) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Failed to close render session!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Render Session Lifecycle Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_render_test_texture_pool_recycling(void) {
    bospectra_log("TEST", "Running BOTexture Pool Recycling Test...");

    BOTexture* t1 = NULL;
    if (bospectra_texture_acquire(1920, 1080, &t1) != BOSPECTRA_SUCCESS || !t1) {
        bospectra_log("TEST_FAIL", "Failed to acquire BOTexture from pool!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (bospectra_texture_release(t1) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Failed to release BOTexture to pool!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOTexture* t2 = NULL;
    if (bospectra_texture_acquire(1920, 1080, &t2) != BOSPECTRA_SUCCESS || !t2) {
        bospectra_log("TEST_FAIL", "Failed to re-acquire BOTexture from pool!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (t2 != t1) {
        bospectra_texture_release(t2);
        bospectra_log("TEST_FAIL", "BOTexture pool failed to recycle first slot!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_texture_release(t2);
    bospectra_log("TEST_PASS", "BOTexture Pool Recycling Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_render_test_boundary_guards(void) {
    bospectra_log("TEST", "Running Render Boundary Guard Test...");

    BOSPECTRA_RenderSpec spec;
    spec.backend = BOSPECTRA_RENDER_BACKEND_SOFTWARE;
    spec.filter = BOSPECTRA_SCALING_FILTER_NEAREST;
    spec.rotation = BOSPECTRA_ROTATION_0;
    spec.target_window_id = 101;
    spec.flags = 0;

    bospectra_render_session_id_t sess_id = 0;
    BOSPECTRA_Render_OpenSession(&spec, &sess_id);

    // Test Null Frame Rejection
    if (BOSPECTRA_Render_PresentFrame(sess_id, NULL, 0, 0, 100, 100) == BOSPECTRA_SUCCESS) {
        BOSPECTRA_Render_CloseSession(sess_id);
        bospectra_log("TEST_FAIL", "Null frame presentation succeeded unexpectedly!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPECTRA_Render_CloseSession(sess_id);
    bospectra_log("TEST_PASS", "Render Boundary Guard Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_render_test_1k_render_stress(void) {
    bospectra_log("TEST", "Running 1,000 Frame Render Presentation Stress Test...");

    BOSPECTRA_RenderSpec spec;
    spec.backend = BOSPECTRA_RENDER_BACKEND_SOFTWARE;
    spec.filter = BOSPECTRA_SCALING_FILTER_BILINEAR;
    spec.rotation = BOSPECTRA_ROTATION_0;
    spec.target_window_id = 102;
    spec.flags = 0;

    bospectra_render_session_id_t sess_id = 0;
    BOSPECTRA_Render_OpenSession(&spec, &sess_id);

    BOSFrame* frame = NULL;
    bospectra_frame_acquire(64, 64, BOSPECTRA_PIXEL_FORMAT_ARGB32, &frame);

    for (int i = 0; i < 1000; i++) {
        if (BOSPECTRA_Render_PresentFrame(sess_id, frame, 0, 0, 64, 64) != BOSPECTRA_SUCCESS) {
            bospectra_frame_release(frame);
            BOSPECTRA_Render_CloseSession(sess_id);
            bospectra_log("TEST_FAIL", "1K Render presentation stress failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
    }

    bospectra_frame_release(frame);
    BOSPECTRA_Render_CloseSession(sess_id);

    bospectra_log("TEST_PASS", "1,000 Frame Render Presentation Stress Test PASSED.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_render_tests_run_all(void) {
    display_print("========= BOSPECTRA RENDERING ENGINE TESTS =========\n");

    bospectra_error_t res = bospectra_render_test_session_lifecycle();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_render_test_texture_pool_recycling();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_render_test_boundary_guards();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_render_test_1k_render_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:RENDER_TEST] All Rendering Engine Self-Tests PASSED!\n");
    display_print("====================================================\n");

    return BOSPECTRA_SUCCESS;
}
