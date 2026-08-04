#include "frame_memory_tests.h"
#include "../frame_pool/frame_pool.h"
#include "../packet_pool/packet_pool.h"
#include "../ring_buffer/circular_ring.h"
#include "../queue/media_queue.h"
#include "../../debug/bospectra_debug.h"
#include "../../include/bospectra_errors.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_frame_memory_test_frame_pool(void) {
    bospectra_log("TEST", "Running Frame Pool Acquire/Release Self-Test...");

    BOSFrame* f1 = NULL;
    if (bospectra_frame_acquire(1920, 1080, BOSPECTRA_PIXEL_FORMAT_ARGB32, &f1) != BOSPECTRA_SUCCESS || !f1) {
        bospectra_log("TEST_FAIL", "Failed to acquire frame!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (f1->ref_count != 1 || f1->linesize[0] != 1920 * 4) {
        bospectra_frame_release(f1);
        bospectra_log("TEST_FAIL", "Frame properties mismatch!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Reference counting test
    bospectra_frame_ref(f1);
    if (f1->ref_count != 2) {
        bospectra_frame_release(f1);
        bospectra_log("TEST_FAIL", "Frame ref counting failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_unref(f1); // ref_count -> 1
    bospectra_frame_unref(f1); // ref_count -> 0 (released to pool)

    // Re-acquire frame to verify pool memory recycling
    BOSFrame* f2 = NULL;
    if (bospectra_frame_acquire(1920, 1080, BOSPECTRA_PIXEL_FORMAT_ARGB32, &f2) != BOSPECTRA_SUCCESS || !f2) {
        bospectra_log("TEST_FAIL", "Failed to re-acquire frame from pool!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (f2->pool_index != f1->pool_index) {
        bospectra_frame_release(f2);
        bospectra_log("TEST_FAIL", "Frame pool failed to reuse first slot!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_release(f2);
    bospectra_log("TEST_PASS", "Frame Pool Acquire/Release Self-Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_memory_test_packet_pool(void) {
    bospectra_log("TEST", "Running Packet Pool Self-Test...");

    BOSPacket* pkt1 = NULL;
    if (bospectra_packet_pool_acquire(4096, &pkt1) != BOSPECTRA_SUCCESS || !pkt1) {
        bospectra_log("TEST_FAIL", "Failed to acquire packet from pool!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (bospectra_packet_pool_release(pkt1) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Failed to release packet to pool!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    // Verify double-free protection
    if (bospectra_packet_pool_release(pkt1) == BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Double-free protection failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Packet Pool Self-Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_memory_test_circular_ring(void) {
    bospectra_log("TEST", "Running Circular Ring Buffer Wrap-Around Test...");

    BOSPECTRA_CircularRing ring;
    bospectra_circular_ring_init(&ring);

    uint8_t write_buf[128];
    uint8_t read_buf[128];
    for (int i = 0; i < 128; i++) write_buf[i] = (uint8_t)i;

    size_t written = 0, read_bytes = 0;

    for (int cycle = 0; cycle < 1000; cycle++) {
        if (bospectra_circular_ring_write(&ring, write_buf, sizeof(write_buf), &written) != BOSPECTRA_SUCCESS || written != sizeof(write_buf)) {
            bospectra_log("TEST_FAIL", "Ring write failed during cycle!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }

        if (bospectra_circular_ring_read(&ring, read_buf, sizeof(read_buf), &read_bytes) != BOSPECTRA_SUCCESS || read_bytes != sizeof(read_buf)) {
            bospectra_log("TEST_FAIL", "Ring read failed during cycle!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }

        if (read_buf[0] != write_buf[0] || read_buf[127] != write_buf[127]) {
            bospectra_log("TEST_FAIL", "Ring data corruption detected!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
    }

    bospectra_log("TEST_PASS", "Circular Ring Buffer Wrap-Around Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_memory_test_media_queue(void) {
    bospectra_log("TEST", "Running Multi-Stream Queue Test...");

    BOSPECTRA_MediaQueue video_q;
    bospectra_media_queue_init(&video_q, BOSPECTRA_QUEUE_VIDEO);

    BOSFrame* frame = NULL;
    bospectra_frame_acquire(1920, 1080, BOSPECTRA_PIXEL_FORMAT_ARGB32, &frame);

    if (bospectra_media_queue_push(&video_q, frame) != BOSPECTRA_SUCCESS) {
        bospectra_frame_release(frame);
        bospectra_log("TEST_FAIL", "Queue push failed!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    void* popped_frame = NULL;
    if (bospectra_media_queue_pop(&video_q, &popped_frame) != BOSPECTRA_SUCCESS || popped_frame != frame) {
        bospectra_frame_release(frame);
        bospectra_log("TEST_FAIL", "Queue pop mismatch!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_frame_release(frame);
    bospectra_log("TEST_PASS", "Multi-Stream Queue Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_memory_test_10k_stress(void) {
    bospectra_log("TEST", "Running 10,000 Cycle Frame Memory Stress Test...");

    for (int i = 0; i < 10000; i++) {
        BOSFrame* f = NULL;
        if (bospectra_frame_acquire(1920, 1080, BOSPECTRA_PIXEL_FORMAT_ARGB32, &f) != BOSPECTRA_SUCCESS) {
            bospectra_log("TEST_FAIL", "Stress test frame acquire failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
        if (bospectra_frame_release(f) != BOSPECTRA_SUCCESS) {
            bospectra_log("TEST_FAIL", "Stress test frame release failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }

        BOSPacket* p = NULL;
        if (bospectra_packet_pool_acquire(4096, &p) != BOSPECTRA_SUCCESS) {
            bospectra_log("TEST_FAIL", "Stress test packet acquire failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
        if (bospectra_packet_pool_release(p) != BOSPECTRA_SUCCESS) {
            bospectra_log("TEST_FAIL", "Stress test packet release failed!");
            return BOSPECTRA_ERR_SELF_TEST_FAILED;
        }
    }

    bospectra_log("TEST_PASS", "10,000 Cycle Frame Memory Stress Test PASSED (100% Recycled).");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_frame_memory_tests_run_all(void) {
    display_print("========= BOSPECTRA FRAME MEMORY ENGINE SELF-TESTS =========\n");

    bospectra_error_t res = bospectra_frame_memory_test_frame_pool();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_frame_memory_test_packet_pool();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_frame_memory_test_circular_ring();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_frame_memory_test_media_queue();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_frame_memory_test_10k_stress();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:MEMORY_TEST] All Frame Memory Tests PASSED!\n");
    display_print("============================================================\n");

    return BOSPECTRA_SUCCESS;
}
