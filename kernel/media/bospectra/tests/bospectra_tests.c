#include "bospectra_tests.h"
#include "../include/bospectra.h"
#include "../include/bospectra_errors.h"
#include "../core/bospectra_state.h"
#include "../memory/bospectra_memory.h"
#include "../packet/bospectra_packet.h"
#include "../buffer/bospectra_buffer.h"
#include "../stream/bospectra_stream.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

bospectra_error_t bospectra_tests_test_lifecycle(void) {
    bospectra_log("TEST", "Running Lifecycle Self-Test...");

    if (BOSPECTRA_GetState() != BOSPECTRA_STATE_READY) {
        bospectra_log("TEST_FAIL", "Engine state not READY after init!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Lifecycle Self-Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_tests_test_memory(void) {
    bospectra_log("TEST", "Running Memory Allocator Self-Test...");

    size_t initial_bytes = bospectra_memory_get_live_bytes();
    void* ptr = bospectra_mem_alloc(1024, "TestAlloc");
    if (!ptr) {
        bospectra_log("TEST_FAIL", "Memory allocation failed!");
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    if (bospectra_memory_get_live_bytes() < initial_bytes + 1024) {
        bospectra_log("TEST_FAIL", "Memory tracking mismatch!");
        bospectra_mem_free(ptr);
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_mem_free(ptr);
    if (bospectra_memory_get_live_bytes() != initial_bytes) {
        bospectra_log("TEST_FAIL", "Memory leak detected after free!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_log("TEST_PASS", "Memory Allocator Self-Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_tests_test_packet_buffer(void) {
    bospectra_log("TEST", "Running Packet & Ring Buffer Self-Test...");

    bospectra_buffer_id_t buf_id = 0;
    if (bospectra_buffer_create(&buf_id) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Failed to create ring buffer!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPacket* pkt = bospectra_packet_alloc(512);
    if (!pkt) {
        bospectra_buffer_destroy(buf_id);
        bospectra_log("TEST_FAIL", "Failed to allocate packet!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    if (bospectra_buffer_push(buf_id, pkt) != BOSPECTRA_SUCCESS) {
        bospectra_packet_free(pkt);
        bospectra_buffer_destroy(buf_id);
        bospectra_log("TEST_FAIL", "Failed to push packet to buffer!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPacket* popped_pkt = NULL;
    if (bospectra_buffer_pop(buf_id, &popped_pkt) != BOSPECTRA_SUCCESS || popped_pkt != pkt) {
        if (popped_pkt) bospectra_packet_free(popped_pkt);
        bospectra_buffer_destroy(buf_id);
        bospectra_log("TEST_FAIL", "Failed to pop packet from buffer!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_packet_free(popped_pkt);
    bospectra_buffer_destroy(buf_id);

    bospectra_log("TEST_PASS", "Packet & Ring Buffer Self-Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_tests_test_stream_registry(void) {
    bospectra_log("TEST", "Running Stream Registry Self-Test...");

    BOSPECTRA_StreamDescriptor desc;
    desc.type = BOSPECTRA_STREAM_VIDEO;
    desc.width = 1920;
    desc.height = 1080;
    desc.frame_rate_num = 60;
    desc.frame_rate_den = 1;

    bospectra_stream_id_t stream_id = 0;
    if (bospectra_stream_register(&desc, &stream_id) != BOSPECTRA_SUCCESS) {
        bospectra_log("TEST_FAIL", "Failed to register stream!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    BOSPECTRA_StreamDescriptor queried;
    if (bospectra_stream_get_descriptor(stream_id, &queried) != BOSPECTRA_SUCCESS || queried.width != 1920) {
        bospectra_stream_unregister(stream_id);
        bospectra_log("TEST_FAIL", "Failed to query stream descriptor!");
        return BOSPECTRA_ERR_SELF_TEST_FAILED;
    }

    bospectra_stream_unregister(stream_id);
    bospectra_log("TEST_PASS", "Stream Registry Self-Test Passed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_tests_run_all(void) {
    display_print("=============== BOSPECTRA ENGINE SELF-TESTS ===============\n");

    bospectra_error_t res = bospectra_tests_test_lifecycle();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_tests_test_memory();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_tests_test_packet_buffer();
    if (res != BOSPECTRA_SUCCESS) return res;

    res = bospectra_tests_test_stream_registry();
    if (res != BOSPECTRA_SUCCESS) return res;

    display_print("[BOSPECTRA:TEST] All Phase 1 Self-Tests PASSED cleanly!\n");
    display_print("===========================================================\n");

    return BOSPECTRA_SUCCESS;
}
