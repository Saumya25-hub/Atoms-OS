#include "color_tests.h"
#include "../include/bospectra_color.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_color_test_red_reference(void) {
    bospectra_log("TEST", "Running Pure Red Reference Color Test...");

    BOSFrame* src_frame = NULL;
    if (bospectra_frame_acquire(16, 16, BOSPECTRA_PIXEL_FORMAT_YUV420P, &src_frame) != BOSPECTRA_SUCCESS || !src_frame) {
        bospectra_log("TEST_FAIL", "Failed to acquire source frame!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Fill Y, U, V with Red Reference values (Y=81, U=90, V=240)
    memset(src_frame->data[0], 81, 16 * 16);
    memset(src_frame->data[1], 90, 8 * 8);
    memset(src_frame->data[2], 240, 8 * 8);

    BOSFrame* dst_frame = NULL;
    bospectra_error_t err = BOSPECTRA_Color_ConvertFrame(src_frame, BOSPECTRA_PIXEL_FORMAT_ARGB32, BOSPECTRA_COLOR_SPACE_BT601, &dst_frame);
    if (err != BOSPECTRA_SUCCESS || !dst_frame) {
        bospectra_frame_release(src_frame);
        bospectra_log("TEST_FAIL", "Red frame conversion failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    uint32_t px = ((uint32_t*)dst_frame->data[0])[0];
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;

    // Verify Red is dominant (r > 200, g < 50, b < 50)
    if (r < 200 || g > 50 || b > 50) {
        bospectra_frame_release(src_frame);
        bospectra_frame_release(dst_frame);
        bospectra_log("TEST_FAIL", "Red reference color test failed! Color tint mismatch.");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_release(src_frame);
    bospectra_frame_release(dst_frame);
    bospectra_log("TEST_PASS", "Pure Red Reference Color Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_color_test_green_reference(void) {
    bospectra_log("TEST", "Running Pure Green Reference Color Test...");

    BOSFrame* src_frame = NULL;
    if (bospectra_frame_acquire(16, 16, BOSPECTRA_PIXEL_FORMAT_YUV420P, &src_frame) != BOSPECTRA_SUCCESS || !src_frame) {
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Fill Y, U, V with Green Reference values (Y=145, U=54, V=34)
    memset(src_frame->data[0], 145, 16 * 16);
    memset(src_frame->data[1], 54, 8 * 8);
    memset(src_frame->data[2], 34, 8 * 8);

    BOSFrame* dst_frame = NULL;
    if (BOSPECTRA_Color_ConvertFrame(src_frame, BOSPECTRA_PIXEL_FORMAT_ARGB32, BOSPECTRA_COLOR_SPACE_BT601, &dst_frame) != BOSPECTRA_SUCCESS) {
        bospectra_frame_release(src_frame);
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    uint32_t px = ((uint32_t*)dst_frame->data[0])[0];
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;

    if (g < 200 || r > 50 || b > 50) {
        bospectra_frame_release(src_frame);
        bospectra_frame_release(dst_frame);
        bospectra_log("TEST_FAIL", "Green reference color test failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_release(src_frame);
    bospectra_frame_release(dst_frame);
    bospectra_log("TEST_PASS", "Pure Green Reference Color Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_color_test_blue_reference(void) {
    bospectra_log("TEST", "Running Pure Blue Reference Color Test...");

    BOSFrame* src_frame = NULL;
    if (bospectra_frame_acquire(16, 16, BOSPECTRA_PIXEL_FORMAT_YUV420P, &src_frame) != BOSPECTRA_SUCCESS || !src_frame) {
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Fill Y, U, V with Blue Reference values (Y=41, U=240, V=110)
    memset(src_frame->data[0], 41, 16 * 16);
    memset(src_frame->data[1], 240, 8 * 8);
    memset(src_frame->data[2], 110, 8 * 8);

    BOSFrame* dst_frame = NULL;
    if (BOSPECTRA_Color_ConvertFrame(src_frame, BOSPECTRA_PIXEL_FORMAT_ARGB32, BOSPECTRA_COLOR_SPACE_BT601, &dst_frame) != BOSPECTRA_SUCCESS) {
        bospectra_frame_release(src_frame);
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    uint32_t px = ((uint32_t*)dst_frame->data[0])[0];
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;

    if (b < 200 || r > 60 || g > 60) {
        bospectra_frame_release(src_frame);
        bospectra_frame_release(dst_frame);
        bospectra_log("TEST_FAIL", "Blue reference color test failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_release(src_frame);
    bospectra_frame_release(dst_frame);
    bospectra_log("TEST_PASS", "Pure Blue Reference Color Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_color_test_black_white_reference(void) {
    bospectra_log("TEST", "Running Black & White Reference Color Test...");

    BOSFrame* src_frame = NULL;
    bospectra_frame_acquire(16, 16, BOSPECTRA_PIXEL_FORMAT_YUV420P, &src_frame);

    // Black frame (Y=0, U=128, V=128)
    memset(src_frame->data[0], 0, 16 * 16);
    memset(src_frame->data[1], 128, 8 * 8);
    memset(src_frame->data[2], 128, 8 * 8);

    BOSFrame* dst_frame = NULL;
    BOSPECTRA_Color_ConvertFrame(src_frame, BOSPECTRA_PIXEL_FORMAT_ARGB32, BOSPECTRA_COLOR_SPACE_BT601, &dst_frame);

    uint32_t black_px = ((uint32_t*)dst_frame->data[0])[0] & 0x00FFFFFF;
    if (black_px != 0x00000000) {
        bospectra_frame_release(src_frame);
        bospectra_frame_release(dst_frame);
        bospectra_log("TEST_FAIL", "Black reference color test failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_release(src_frame);
    bospectra_frame_release(dst_frame);
    bospectra_log("TEST_PASS", "Black & White Reference Color Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_color_test_1k_conversion_stress(void) {
    bospectra_log("TEST", "Running 1,000 Frame Color Conversion Stress Test...");

    BOSFrame* src_frame = NULL;
    bospectra_frame_acquire(64, 64, BOSPECTRA_PIXEL_FORMAT_YUV420P, &src_frame);

    for (int i = 0; i < 1000; i++) {
        BOSFrame* dst_frame = NULL;
        if (BOSPECTRA_Color_ConvertFrame(src_frame, BOSPECTRA_PIXEL_FORMAT_ARGB32, BOSPECTRA_COLOR_SPACE_BT601, &dst_frame) != BOSPECTRA_SUCCESS) {
            bospectra_frame_release(src_frame);
            bospectra_log("TEST_FAIL", "1K Color conversion stress failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
        bospectra_frame_release(dst_frame);
    }

    bospectra_frame_release(src_frame);
    bospectra_log("TEST_PASS", "1,000 Frame Color Conversion Stress Test PASSED.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_color_tests_run_all(void) {
    display_print("========= BOSPECTRA COLOR ENGINE REFERENCE TESTS =========\n");

    bospectra_error_t res = bospectra_color_test_red_reference();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_color_test_green_reference();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_color_test_blue_reference();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_color_test_black_white_reference();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_color_test_1k_conversion_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:COLOR_TEST] All Color Accuracy Self-Tests PASSED!\n");
    display_print("=========================================================\n");

    return BOSPECTRA_SUCCESS;
}
