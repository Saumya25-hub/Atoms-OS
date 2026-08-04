#include "container_tests.h"
#include "../common/container_common.h"
#include "../registry/container_registry.h"
#include "../include/bospectra_container.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_container_test_probing(void) {
    bospectra_log("TEST", "Running Container Probing Test...");

    // Simulated MP4 Header
    uint8_t mp4_header[16] = {
        0x00, 0x00, 0x00, 0x14, 'f', 't', 'y', 'p',
        'i', 's', 'o', 'm', 0x00, 0x00, 0x00, 0x00
    };

    const BOSPECTRA_ContainerDriver* mp4_drv = bospectra_container_find_driver_by_name("MP4");
    if (!mp4_drv || !mp4_drv->probe) {
        bospectra_log("TEST_FAIL", "MP4 Driver not registered!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    int score = mp4_drv->probe(0, mp4_header, sizeof(mp4_header));
    if (score < 80) {
        bospectra_log("TEST_FAIL", "MP4 Probe score failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Simulated AVI Header
    uint8_t avi_header[16] = {
        'R', 'I', 'F', 'F', 0x00, 0x10, 0x00, 0x00,
        'A', 'V', 'I', ' ', 'L', 'I', 'S', 'T'
    };

    const BOSPECTRA_ContainerDriver* avi_drv = bospectra_container_find_driver_by_name("AVI");
    if (!avi_drv || !avi_drv->probe) {
        bospectra_log("TEST_FAIL", "AVI Driver not registered!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    score = avi_drv->probe(0, avi_header, sizeof(avi_header));
    if (score != 100) {
        bospectra_log("TEST_FAIL", "AVI Probe score failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Container Probing Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_container_test_bounds_security(void) {
    bospectra_log("TEST", "Running Container Security Bounds Test...");

    // Test valid boundary
    if (!bospectra_container_check_bounds(100, 50, 200)) {
        bospectra_log("TEST_FAIL", "Valid bounds check failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Test out-of-bounds read
    if (bospectra_container_check_bounds(100, 150, 200)) {
        bospectra_log("TEST_FAIL", "Out of bounds read allowed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Test integer overflow
    if (bospectra_container_check_bounds(UINT64_MAX - 10, 20, UINT64_MAX)) {
        bospectra_log("TEST_FAIL", "Integer overflow allowed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Container Security Bounds Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_container_tests_run_all(void) {
    display_print("=========== BOSPECTRA CONTAINER ENGINE SELF-TESTS ===========\n");

    bospectra_error_t res = bospectra_container_test_probing();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_container_test_bounds_security();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:CONTAINER_TEST] All Container Self-Tests PASSED!\n");
    display_print("============================================================\n");

    return BOSPECTRA_SUCCESS;
}
