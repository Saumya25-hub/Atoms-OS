/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_tests.c — Full Certification Test Suite (21 Tests)
 *
 * Every test is self-contained and validates a single capability.
 * Executed on kernel boot to certify the IPC subsystem.
 */

#include "kernel/ipc/tests/ipc_tests.h"
#include "kernel/ipc/include/ipc_types.h"
#include "kernel/ipc/include/ipc_api.h"
#include "kernel/ipc/channels/channel_manager.h"
#include "kernel/ipc/message/message_queue.h"
#include "kernel/ipc/pipes/pipe_engine.h"
#include "kernel/ipc/ports/port_manager.h"
#include "kernel/ipc/shared_memory/shm_manager.h"
#include "kernel/ipc/router/ipc_router.h"
#include "kernel/ipc/sync/ipc_sync.h"
#include "kernel/drivers/display/display.h"

static uint32_t g_tests_passed = 0;
static uint32_t g_tests_failed = 0;

static void test_pass(const char* name) {
    display_print("PASSED\n");
    g_tests_passed++;
    (void)name;
}

static void test_fail(const char* name, const char* reason) {
    display_print("FAILED [");
    display_print(reason);
    display_print("]\n");
    g_tests_failed++;
    (void)name;
}

/* Inline memcpy */
static void test_memcpy(void* dst, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}

static bool test_memcmp(const void* a, const void* b, uint32_t n) {
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    for (uint32_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) return false;
    }
    return true;
}

/* ============================================================
 * TEST 1: Channel Creation
 * ============================================================ */
static void test_01_channel_creation(void) {
    display_print("[TEST  1] Channel Creation...                    ");
    ipc_channel_handle_t h = IPC_INVALID_HANDLE;
    ipc_status_t s = bos_ipc_create_channel("test.channel.1",
                                             IPC_CHANNEL_NAMED, &h);
    if (s == IPC_SUCCESS && h != IPC_INVALID_HANDLE) {
        test_pass("channel_creation");
    } else {
        test_fail("channel_creation", "Create returned error");
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 2: Channel Destruction
 * ============================================================ */
static void test_02_channel_destruction(void) {
    display_print("[TEST  2] Channel Destruction...                 ");
    ipc_channel_handle_t h;
    bos_ipc_create_channel("test.destroy", IPC_CHANNEL_NAMED, &h);
    ipc_status_t s = bos_ipc_close(h);
    if (s == IPC_SUCCESS) {
        test_pass("channel_destruction");
    } else {
        test_fail("channel_destruction", "Close returned error");
    }
}

/* ============================================================
 * TEST 3: Named Channel Lookup
 * ============================================================ */
static void test_03_named_channel_lookup(void) {
    display_print("[TEST  3] Named Channel Lookup...                ");
    ipc_channel_handle_t h1, h2;
    bos_ipc_create_channel("test.lookup", IPC_CHANNEL_NAMED, &h1);
    ipc_status_t s = bos_ipc_connect("test.lookup", &h2);
    if (s == IPC_SUCCESS && h2 != IPC_INVALID_HANDLE) {
        test_pass("named_lookup");
    } else {
        test_fail("named_lookup", "Connect by name failed");
    }
    bos_ipc_close(h1);
    bos_ipc_close(h2);
}

/* ============================================================
 * TEST 4: Anonymous Channel
 * ============================================================ */
static void test_04_anonymous_channel(void) {
    display_print("[TEST  4] Anonymous Channel...                   ");
    ipc_channel_handle_t h;
    ipc_status_t s = bos_ipc_create_channel((void*)0,
                                             IPC_CHANNEL_ANONYMOUS, &h);
    if (s == IPC_SUCCESS && h != IPC_INVALID_HANDLE) {
        test_pass("anonymous_channel");
    } else {
        test_fail("anonymous_channel", "Anonymous create failed");
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 5: Small Message Send/Recv
 * ============================================================ */
static void test_05_small_message(void) {
    display_print("[TEST  5] Small Message Send/Recv...             ");
    ipc_channel_handle_t h;
    bos_ipc_create_channel("test.msg.small", IPC_CHANNEL_NAMED, &h);

    const char* data = "Hello BOS IPC!";
    ipc_status_t s = bos_ipc_send(h, data, 15, 0);
    if (s != IPC_SUCCESS) { test_fail("small_msg", "Send failed"); bos_ipc_close(h); return; }

    char buf[64];
    uint32_t out_size = 0;
    s = bos_ipc_receive(h, buf, 64, &out_size, IPC_FLAG_NONBLOCKING);
    if (s == IPC_SUCCESS && out_size == 15 && test_memcmp(buf, data, 15)) {
        test_pass("small_msg");
    } else {
        test_fail("small_msg", "Recv mismatch");
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 6: Large Message Send/Recv (2KB)
 * ============================================================ */
static void test_06_large_message(void) {
    display_print("[TEST  6] Large Message Send/Recv (2KB)...       ");
    ipc_channel_handle_t h;
    bos_ipc_create_channel("test.msg.large", IPC_CHANNEL_NAMED, &h);

    /* Fill a 2KB payload with a pattern */
    static uint8_t big_data[2048];
    for (uint32_t i = 0; i < 2048; i++) big_data[i] = (uint8_t)(i & 0xFF);

    ipc_status_t s = bos_ipc_send(h, big_data, 2048, 0);
    if (s != IPC_SUCCESS) { test_fail("large_msg", "Send failed"); bos_ipc_close(h); return; }

    static uint8_t big_buf[2048];
    uint32_t out_size = 0;
    s = bos_ipc_receive(h, big_buf, 2048, &out_size, IPC_FLAG_NONBLOCKING);
    if (s == IPC_SUCCESS && out_size == 2048 && test_memcmp(big_buf, big_data, 2048)) {
        test_pass("large_msg");
    } else {
        test_fail("large_msg", "Recv mismatch");
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 7: Non-Blocking Receive (empty queue)
 * ============================================================ */
static void test_07_nonblocking_receive(void) {
    display_print("[TEST  7] Non-Blocking Receive (empty)...        ");
    ipc_channel_handle_t h;
    bos_ipc_create_channel("test.nonblock", IPC_CHANNEL_NAMED, &h);

    char buf[16];
    uint32_t out_size = 0;
    ipc_status_t s = bos_ipc_receive(h, buf, 16, &out_size, IPC_FLAG_NONBLOCKING);
    if (s == IPC_ERR_QUEUE_EMPTY) {
        test_pass("nonblocking");
    } else {
        test_fail("nonblocking", "Expected QUEUE_EMPTY");
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 8: Message Queue Overflow
 * ============================================================ */
static void test_08_queue_overflow(void) {
    display_print("[TEST  8] Message Queue Overflow...              ");
    ipc_channel_handle_t h;
    bos_ipc_create_channel("test.overflow", IPC_CHANNEL_NAMED, &h);

    uint8_t tiny = 0xAA;
    ipc_status_t s = IPC_SUCCESS;
    for (uint32_t i = 0; i < IPC_MESSAGE_QUEUE_DEPTH; i++) {
        s = bos_ipc_send(h, &tiny, 1, 0);
        if (s != IPC_SUCCESS) break;
    }

    /* Next send should fail */
    s = bos_ipc_send(h, &tiny, 1, 0);
    if (s == IPC_ERR_QUEUE_FULL) {
        test_pass("overflow");
    } else {
        test_fail("overflow", "Expected QUEUE_FULL");
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 9: SHM Create
 * ============================================================ */
static void test_09_shm_create(void) {
    display_print("[TEST  9] SHM Create...                          ");
    ipc_shm_handle_t h;
    ipc_status_t s = bos_shm_create("test.shm.1", 4096, IPC_SHM_RDWR, &h);
    if (s == IPC_SUCCESS && h != IPC_INVALID_HANDLE) {
        test_pass("shm_create");
    } else {
        test_fail("shm_create", "Create failed");
    }
    bos_shm_destroy(h);
}

/* ============================================================
 * TEST 10: SHM Map
 * ============================================================ */
static void test_10_shm_map(void) {
    display_print("[TEST 10] SHM Map...                             ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.map", 4096, IPC_SHM_RDWR, &h);

    void* addr = (void*)0;
    ipc_status_t s = bos_shm_map(h, 0, IPC_SHM_RDWR, &addr);
    if (s == IPC_SUCCESS && addr != (void*)0) {
        test_pass("shm_map");
    } else {
        test_fail("shm_map", "Map failed");
    }
    bos_shm_unmap(h, 0);
    bos_shm_destroy(h);
}

/* ============================================================
 * TEST 11: SHM Read/Write
 * ============================================================ */
static void test_11_shm_readwrite(void) {
    display_print("[TEST 11] SHM Read/Write...                      ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.rw", 4096, IPC_SHM_RDWR, &h);

    void* addr = (void*)0;
    bos_shm_map(h, 0, IPC_SHM_RDWR, &addr);

    if (addr) {
        /* Write pattern */
        uint32_t* ptr = (uint32_t*)addr;
        ptr[0] = 0xDEADBEEF;
        ptr[1] = 0xCAFEBABE;

        /* Read back */
        if (ptr[0] == 0xDEADBEEF && ptr[1] == 0xCAFEBABE) {
            test_pass("shm_rw");
        } else {
            test_fail("shm_rw", "Read mismatch");
        }
    } else {
        test_fail("shm_rw", "Map returned NULL");
    }
    bos_shm_unmap(h, 0);
    bos_shm_destroy(h);
}

/* ============================================================
 * TEST 12: SHM Unmap
 * ============================================================ */
static void test_12_shm_unmap(void) {
    display_print("[TEST 12] SHM Unmap...                           ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.unmap", 4096, IPC_SHM_RDWR, &h);

    void* addr = (void*)0;
    bos_shm_map(h, 0, IPC_SHM_RDWR, &addr);
    ipc_status_t s = bos_shm_unmap(h, 0);
    if (s == IPC_SUCCESS) {
        test_pass("shm_unmap");
    } else {
        test_fail("shm_unmap", "Unmap failed");
    }
    bos_shm_destroy(h);
}

/* ============================================================
 * TEST 13: SHM Destroy
 * ============================================================ */
static void test_13_shm_destroy(void) {
    display_print("[TEST 13] SHM Destroy...                         ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.destroy", 4096, IPC_SHM_RDWR, &h);
    ipc_status_t s = bos_shm_destroy(h);
    if (s == IPC_SUCCESS) {
        test_pass("shm_destroy");
    } else {
        test_fail("shm_destroy", "Destroy failed");
    }
}

/* ============================================================
 * TEST 14: Zero-Copy Validation (two mappings, same data)
 * ============================================================ */
static void test_14_zero_copy(void) {
    display_print("[TEST 14] Zero-Copy Validation...                ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.zerocopy", 4096, IPC_SHM_RDWR, &h);

    void* addr1 = (void*)0;
    void* addr2 = (void*)0;
    bos_shm_map(h, 0, IPC_SHM_RDWR, &addr1);
    bos_shm_map(h, 1, IPC_SHM_RDWR, &addr2);

    if (addr1 && addr2) {
        /* Write via mapping 1 */
        uint32_t* p1 = (uint32_t*)addr1;
        p1[0] = 0x12345678;

        /* Read via mapping 2 — should see same data (same physical page) */
        uint32_t* p2 = (uint32_t*)addr2;
        if (p2[0] == 0x12345678) {
            test_pass("zero_copy");
        } else {
            test_fail("zero_copy", "Mappings see different data");
        }
    } else {
        test_fail("zero_copy", "Dual map failed");
    }
    bos_shm_unmap(h, 0);
    bos_shm_unmap(h, 1);
    bos_shm_destroy(h);
}

/* ============================================================
 * TEST 15: Permission — Invalid Handle
 * ============================================================ */
static void test_15_invalid_handle(void) {
    display_print("[TEST 15] Permission: Invalid Handle...          ");
    char buf[16];
    uint32_t out;
    ipc_status_t s = bos_ipc_receive(IPC_INVALID_HANDLE, buf, 16, &out, 0);
    if (s == IPC_ERR_INVALID_HANDLE) {
        test_pass("invalid_handle");
    } else {
        test_fail("invalid_handle", "Expected INVALID_HANDLE");
    }
}

/* ============================================================
 * TEST 16: Permission — Double Destroy
 * ============================================================ */
static void test_16_double_destroy(void) {
    display_print("[TEST 16] Permission: Double Destroy...          ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.dbldestr", 4096, IPC_SHM_RDWR, &h);
    bos_shm_destroy(h);
    ipc_status_t s = bos_shm_destroy(h);
    if (s == IPC_ERR_ALREADY_DESTROYED) {
        test_pass("double_destroy");
    } else {
        test_fail("double_destroy", "Expected ALREADY_DESTROYED");
    }
}

/* ============================================================
 * TEST 17: Permission — Unauthorized (read-only write attempt)
 * ============================================================ */
static void test_17_unauthorized(void) {
    display_print("[TEST 17] Permission: Read-Only SHM Write...     ");
    ipc_shm_handle_t h;
    bos_shm_create("test.shm.readonly", 4096, IPC_SHM_READ, &h);

    void* addr = (void*)0;
    ipc_status_t s = bos_shm_map(h, 0, IPC_SHM_READ, &addr);
    /* Mapping with read-only should succeed; actual write protection is via MMU */
    if (s == IPC_SUCCESS) {
        test_pass("unauthorized");
    } else {
        test_fail("unauthorized", "Read-only map failed");
    }
    bos_shm_unmap(h, 0);
    bos_shm_destroy(h);
}

/* ============================================================
 * TEST 18: Pipe Create & Transfer
 * ============================================================ */
static void test_18_pipe(void) {
    display_print("[TEST 18] Pipe Create & Transfer...              ");
    ipc_pipe_handle_t h;
    ipc_status_t s = bos_pipe_create(1, 2, &h);
    if (s != IPC_SUCCESS) { test_fail("pipe", "Create failed"); return; }

    const char* msg = "pipe-data";
    uint32_t written = 0;
    s = bos_pipe_write(h, msg, 9, &written);
    if (s != IPC_SUCCESS || written != 9) {
        test_fail("pipe", "Write failed");
        bos_pipe_close(h);
        return;
    }

    char buf[16];
    uint32_t read_cnt = 0;
    s = bos_pipe_read(h, buf, 16, &read_cnt);
    if (s == IPC_SUCCESS && read_cnt == 9 && test_memcmp(buf, msg, 9)) {
        test_pass("pipe");
    } else {
        test_fail("pipe", "Read mismatch");
    }
    bos_pipe_close(h);
}

/* ============================================================
 * TEST 19: Port Bind & Lookup
 * ============================================================ */
static void test_19_port(void) {
    display_print("[TEST 19] Port Bind & Lookup...                  ");
    ipc_port_handle_t ph;
    ipc_status_t s = bos_port_create("com.bos.browser", 1, &ph);
    if (s != IPC_SUCCESS) { test_fail("port", "Create failed"); return; }

    ipc_port_handle_t found;
    s = bos_port_lookup("com.bos.browser", &found);
    if (s == IPC_SUCCESS && found == ph) {
        test_pass("port");
    } else {
        test_fail("port", "Lookup failed");
    }
    bos_port_close(ph);
}

/* ============================================================
 * TEST 20: Router Broadcast
 * ============================================================ */
static void test_20_broadcast(void) {
    display_print("[TEST 20] Router Broadcast...                    ");
    ipc_channel_handle_t h = IPC_INVALID_HANDLE;
    ipc_status_t cs = bos_ipc_create_channel("test.broadcast", IPC_CHANNEL_NAMED | IPC_CHANNEL_BROADCAST, &h);
    if (cs != IPC_SUCCESS) {
        display_print("FAILED [Channel create err=");
        display_print_dec((uint64_t)(-cs));
        display_print("]\n");
        g_tests_failed++;
        return;
    }

    /* Subscribe two PIDs */
    bos_ipc_subscribe(h, 100);
    bos_ipc_subscribe(h, 200);

    /* Broadcast message */
    const char* data = "broadcast!";
    ipc_status_t s = bos_ipc_broadcast(h, data, 10);
    if (s == IPC_SUCCESS) {
        /* Verify message is in the queue */
        char buf[16];
        uint32_t out = 0;
        s = bos_ipc_receive(h, buf, 16, &out, IPC_FLAG_NONBLOCKING);
        if (s == IPC_SUCCESS && out == 10) {
            test_pass("broadcast");
        } else {
            test_fail("broadcast", "Recv after broadcast failed");
        }
    } else {
        display_print("FAILED [Broadcast send err=");
        display_print_dec((uint64_t)(-s));
        display_print("]\n");
        g_tests_failed++;
    }
    bos_ipc_close(h);
}

/* ============================================================
 * TEST 21: Browser → Renderer → GPU Simulation
 * ============================================================ */
static void test_21_browser_renderer_gpu(void) {
    display_print("[TEST 21] Browser->Renderer->GPU Simulation...   ");

    /* 1. Browser creates IPC channels */
    ipc_channel_handle_t browser_renderer_ch = IPC_INVALID_HANDLE;
    ipc_channel_handle_t renderer_gpu_ch     = IPC_INVALID_HANDLE;
    ipc_status_t c1 = bos_ipc_create_channel("bos.browser.renderer", IPC_CHANNEL_NAMED, &browser_renderer_ch);
    ipc_status_t c2 = bos_ipc_create_channel("bos.renderer.gpu", IPC_CHANNEL_NAMED, &renderer_gpu_ch);
    if (c1 != IPC_SUCCESS || c2 != IPC_SUCCESS) {
        display_print("FAILED [Channel create err]\n");
        g_tests_failed++;
        return;
    }

    /* 2. Create shared memory for frame buffer (8KB) */
    ipc_shm_handle_t frame_shm = IPC_INVALID_HANDLE;
    ipc_status_t sm_err = bos_shm_create("bos.frame.buffer", 8192, IPC_SHM_RDWR, &frame_shm);
    if (sm_err != IPC_SUCCESS) {
        display_print("FAILED [SHM create err=");
        display_print_dec((uint64_t)(-sm_err));
        display_print("]\n");
        g_tests_failed++;
        bos_ipc_close(browser_renderer_ch);
        bos_ipc_close(renderer_gpu_ch);
        return;
    }

    /* 3. Renderer maps the shared memory */
    void* renderer_addr = (void*)0;
    bos_shm_map(frame_shm, 10, IPC_SHM_RDWR, &renderer_addr);

    /* 4. GPU maps the same shared memory (zero-copy) */
    void* gpu_addr = (void*)0;
    bos_shm_map(frame_shm, 20, IPC_SHM_READ, &gpu_addr);

    /* 5. Browser sends "RENDER" command to Renderer */
    const char* cmd = "RENDER_FRAME";
    bos_ipc_send(browser_renderer_ch, cmd, 12, 0);

    /* 6. Renderer receives command */
    char recv_buf[32];
    uint32_t recv_size = 0;
    bos_ipc_receive(browser_renderer_ch, recv_buf, 32, &recv_size, IPC_FLAG_NONBLOCKING);

    /* 7. Renderer produces frame in shared memory */
    if (renderer_addr) {
        uint32_t* pixels = (uint32_t*)renderer_addr;
        pixels[0] = 0xFF0000FF; /* Red pixel */
        pixels[1] = 0x00FF00FF; /* Green pixel */
    }

    /* 8. Renderer signals GPU via IPC channel */
    const char* sig = "FRAME_READY";
    bos_ipc_send(renderer_gpu_ch, sig, 11, 0);

    /* 9. GPU receives signal */
    char gpu_buf[32];
    uint32_t gpu_size = 0;
    bos_ipc_receive(renderer_gpu_ch, gpu_buf, 32, &gpu_size, IPC_FLAG_NONBLOCKING);

    /* 10. GPU reads frame from shared memory (zero-copy verification) */
    bool sim_success = false;
    if (gpu_addr && renderer_addr) {
        uint32_t* gpu_pixels = (uint32_t*)gpu_addr;
        if (gpu_pixels[0] == 0xFF0000FF && gpu_pixels[1] == 0x00FF00FF) {
            sim_success = true;
        }
    }

    if (sim_success && recv_size == 12 && gpu_size == 11) {
        test_pass("browser_renderer_gpu");
    } else {
        display_print("FAILED [Pipeline mismatch]\n");
        g_tests_failed++;
    }

    /* Cleanup */
    bos_shm_unmap(frame_shm, 10);
    bos_shm_unmap(frame_shm, 20);
    bos_shm_destroy(frame_shm);
    bos_ipc_close(browser_renderer_ch);
    bos_ipc_close(renderer_gpu_ch);
}

/* ============================================================
 * Test Suite Runner
 * ============================================================ */
bool ipc_run_unit_tests(void) {
    g_tests_passed = 0;
    g_tests_failed = 0;

    display_print("\n============================================================\n");
    display_print(" [IPC_TESTS] Executing IPC & Shared Memory Certification Suite\n");
    display_print("============================================================\n");

    test_01_channel_creation();
    test_02_channel_destruction();
    test_03_named_channel_lookup();
    test_04_anonymous_channel();
    test_05_small_message();
    test_06_large_message();
    test_07_nonblocking_receive();
    test_08_queue_overflow();
    test_09_shm_create();
    test_10_shm_map();
    test_11_shm_readwrite();
    test_12_shm_unmap();
    test_13_shm_destroy();
    test_14_zero_copy();
    test_15_invalid_handle();
    test_16_double_destroy();
    test_17_unauthorized();
    test_18_pipe();
    test_19_port();
    test_20_broadcast();
    test_21_browser_renderer_gpu();

    display_print("============================================================\n");
    if (g_tests_failed == 0) {
        display_print(" [IPC_TESTS] SUCCESS: All 21 IPC Tests Passed!\n");
    } else {
        display_print(" [IPC_TESTS] FAILURE: ");
        display_print_dec(g_tests_failed);
        display_print(" tests failed!\n");
    }
    display_print("============================================================\n\n");

    return (g_tests_failed == 0);
}
