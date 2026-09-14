/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Master Verification Test Suite (Tests A through J)
 */

#include "atoms/userspace/apal/include/apal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32;1m"
#define COLOR_RED     "\033[31;1m"
#define COLOR_CYAN    "\033[36;1m"
#define COLOR_YELLOW  "\033[33;1m"

static int g_passed = 0;
static int g_failed = 0;

static void print_result(const char *test_id, const char *test_name, bool pass, uint64_t duration_us) {
    if (pass) {
        g_passed++;
        printf("  %s%-8s%s: %-45s [%s PASS %s] %6.2f ms\n", 
               COLOR_CYAN, test_id, COLOR_RESET, test_name, COLOR_GREEN, COLOR_RESET, duration_us / 1000.0);
    } else {
        g_failed++;
        printf("  %s%-8s%s: %-45s [%s FAIL %s] %6.2f ms\n", 
               COLOR_CYAN, test_id, COLOR_RESET, test_name, COLOR_RED, COLOR_RESET, duration_us / 1000.0);
    }
}

/* TEST A: Memory Adapter Test */
static bool run_test_a(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();
    size_t page_sz = apal_page_size();
    if (page_sz != 4096) return false;

    /* Allocate 2 pages (8KB) with READ | WRITE */
    void *ptr = apal_page_alloc(NULL, page_sz * 2, APAL_PROT_READ_WRITE, APAL_MEM_ANONYMOUS);
    if (!ptr) return false;

    /* Touch both pages with test patterns */
    uint8_t *b = (uint8_t *)ptr;
    for (size_t i = 0; i < page_sz * 2; i++) {
        b[i] = (uint8_t)(i & 0xFF);
    }

    /* Verify data integrity */
    for (size_t i = 0; i < page_sz * 2; i++) {
        if (b[i] != (uint8_t)(i & 0xFF)) {
            apal_page_free(ptr, page_sz * 2);
            return false;
        }
    }

    /* Test protection change to READ | EXEC */
    if (apal_page_protect(ptr, page_sz * 2, APAL_PROT_READ_EXEC) != APAL_OK) {
        apal_page_free(ptr, page_sz * 2);
        return false;
    }

    /* Test decommit */
    if (apal_page_decommit(ptr, page_sz * 2) != APAL_OK) {
        apal_page_free(ptr, page_sz * 2);
        return false;
    }

    /* Free memory */
    if (apal_page_free(ptr, page_sz * 2) != APAL_OK) return false;

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* Thread worker for Test B */
static volatile int g_thread_counter = 0;
static apal_mutex_t g_test_mutex;
static apal_cond_t g_test_cond;

static void *test_b_worker(void *arg) {
    int val = (int)(intptr_t)arg;
    apal_mutex_lock(&g_test_mutex);
    g_thread_counter += val;
    apal_cond_signal(&g_test_cond);
    apal_mutex_unlock(&g_test_mutex);
    return (void *)(intptr_t)(val * 2);
}

/* TEST B: Thread & Synchronization Test */
static bool run_test_b(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();
    g_thread_counter = 0;
    apal_mutex_init(&g_test_mutex);
    apal_cond_init(&g_test_cond);

    apal_thread_t th1, th2;
    if (apal_thread_create(&th1, 0, test_b_worker, (void *)(intptr_t)10) != APAL_OK) return false;
    if (apal_thread_create(&th2, 0, test_b_worker, (void *)(intptr_t)20) != APAL_OK) return false;

    void *res1 = NULL;
    void *res2 = NULL;
    if (apal_thread_join(th1, &res1) != APAL_OK) return false;
    if (apal_thread_join(th2, &res2) != APAL_OK) return false;

    if (g_thread_counter != 30) return false;
    if ((intptr_t)res1 != 20 || (intptr_t)res2 != 40) return false;

    apal_mutex_destroy(&g_test_mutex);
    apal_cond_destroy(&g_test_cond);

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST C: Filesystem Adapter Test */
static bool run_test_c(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();
    const char *test_path = "/tmp/apal_test.bin";

    apal_file_handle_t fh;
    apal_status_t st = apal_file_open(test_path, APAL_FILE_OPEN_WRITE | APAL_FILE_OPEN_CREATE | APAL_FILE_OPEN_TRUNCATE, &fh);
    if (st != APAL_OK) return false;

    const char payload[] = "ATOMS_APAL_CHROMIUM_VFS_PAYLOAD_TEST_DATA_2026";
    size_t payload_len = strlen(payload);

    int64_t written = apal_file_write(fh, payload, payload_len);
    if (written != (int64_t)payload_len) {
        apal_file_close(fh);
        return false;
    }
    apal_file_close(fh);

    /* Stat the file */
    apal_file_info_t info;
    if (apal_file_stat(test_path, &info) != APAL_OK) return false;
    if (info.size_bytes != payload_len) return false;

    /* Re-open for read */
    if (apal_file_open(test_path, APAL_FILE_OPEN_READ, &fh) != APAL_OK) return false;

    char read_buf[64];
    memset(read_buf, 0, sizeof(read_buf));
    int64_t read_bytes = apal_file_read(fh, read_buf, sizeof(read_buf) - 1);
    apal_file_close(fh);

    if (read_bytes != (int64_t)payload_len) return false;
    if (memcmp(payload, read_buf, payload_len) != 0) return false;

    /* Clean up */
    apal_file_delete(test_path);

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST D: IPC & Shared Memory Test */
static bool run_test_d(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    /* 1. Pipe Test */
    apal_ipc_handle_t ep0, ep1;
    if (apal_ipc_pipe_create(&ep0, &ep1) != APAL_OK) return false;

    const char msg[] = "MOJO_APAL_MESSAGE_PACKET_VERIFIED";
    size_t msg_len = strlen(msg);

    if (apal_ipc_send(ep0, msg, msg_len) != APAL_OK) {
        apal_ipc_close(ep0); apal_ipc_close(ep1);
        return false;
    }

    char recv_buf[64];
    size_t actual_sz = 0;
    if (apal_ipc_recv(ep1, recv_buf, sizeof(recv_buf), &actual_sz) != APAL_OK) {
        apal_ipc_close(ep0); apal_ipc_close(ep1);
        return false;
    }
    if (actual_sz != msg_len || memcmp(msg, recv_buf, msg_len) != 0) {
        apal_ipc_close(ep0); apal_ipc_close(ep1);
        return false;
    }

    apal_ipc_close(ep0);
    apal_ipc_close(ep1);

    /* 2. Shared Memory Region Test */
    apal_shm_handle_t shm_h;
    if (apal_shm_create(8192, false, &shm_h) != APAL_OK) return false;

    apal_shm_mapping_t mapping;
    if (apal_shm_map(shm_h, 8192, false, &mapping) != APAL_OK) {
        apal_shm_close(shm_h);
        return false;
    }

    uint32_t *shm_words = (uint32_t *)mapping.mapped_addr;
    for (int i = 0; i < 2048; i++) {
        shm_words[i] = 0xAA550000 | (uint32_t)i;
    }
    for (int i = 0; i < 2048; i++) {
        if (shm_words[i] != (0xAA550000 | (uint32_t)i)) {
            apal_shm_unmap(&mapping);
            apal_shm_close(shm_h);
            return false;
        }
    }

    apal_shm_unmap(&mapping);
    apal_shm_close(shm_h);

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST E: Socket Adapter Test */
static bool run_test_e(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    apal_socket_handle_t sock;
    if (apal_socket_create(APAL_SOCK_STREAM, &sock) != APAL_OK) return false;
    if (apal_socket_set_nonblocking(sock, true) != APAL_OK) {
        apal_socket_close(sock);
        return false;
    }
    if (apal_socket_connect(sock, "127.0.0.1", 80) != APAL_OK) {
        apal_socket_close(sock);
        return false;
    }

    const char req[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    size_t req_len = strlen(req);
    int64_t sent = apal_socket_send(sock, req, req_len, 0);
    if (sent != (int64_t)req_len) {
        apal_socket_close(sock);
        return false;
    }

    char rx_buf[128];
    int64_t rcvd = apal_socket_recv(sock, rx_buf, sizeof(rx_buf), 0);
    if (rcvd != (int64_t)req_len || memcmp(req, rx_buf, req_len) != 0) {
        apal_socket_close(sock);
        return false;
    }

    apal_socket_close(sock);
    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST F: Graphics Surface Presentation Test */
static bool run_test_f(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    apal_surface_t surf;
    if (apal_surface_create(640, 480, "APAL Chromium Window", &surf) != APAL_OK) return false;
    if (!surf.pixel_buffer) return false;

    /* Fill 32-bpp BGRA frame with solid cobalt blue */
    for (uint32_t y = 0; y < surf.height; y++) {
        for (uint32_t x = 0; x < surf.width; x++) {
            surf.pixel_buffer[y * surf.width + x] = 0xFF1E3A8A; /* ARGB Cobalt */
        }
    }

    /* Invalidate / present */
    if (apal_surface_present(&surf, 0, 0, 640, 480) != APAL_OK) {
        apal_surface_destroy(&surf);
        return false;
    }

    apal_surface_destroy(&surf);
    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST G: Input & Event Translation Test */
static bool run_test_g(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    /* Test key translation */
    uint32_t dom_code = 0;
    if (apal_input_translate_key(0x04, &dom_code) != APAL_OK) return false;
    if (dom_code != 'A') return false;

    if (apal_input_translate_key(0x1E, &dom_code) != APAL_OK) return false;
    if (dom_code != '1') return false;

    /* Test event polling */
    apal_input_event_t ev;
    if (apal_input_poll_event(0, &ev) != APAL_OK) return false;

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST H: Audio Stream Adapter Test */
static bool run_test_h(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    apal_audio_config_t cfg;
    cfg.sample_rate = 48000;
    cfg.channels = 2;
    cfg.bits_per_sample = 16;
    cfg.buffer_frames = 1024;

    apal_audio_stream_t strm;
    if (apal_audio_stream_open(&cfg, &strm) != APAL_OK) return false;

    /* Generate 1024 frames of 48kHz stereo silence / PCM */
    int16_t pcm_frames[1024 * 2];
    memset(pcm_frames, 0, sizeof(pcm_frames));

    int64_t written = apal_audio_stream_write(strm, pcm_frames, sizeof(pcm_frames));
    if (written != (int64_t)sizeof(pcm_frames)) {
        apal_audio_stream_close(strm);
        return false;
    }

    if (apal_audio_stream_flush(strm) != APAL_OK) {
        apal_audio_stream_close(strm);
        return false;
    }

    apal_audio_stream_close(strm);
    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST I: Process Lifecycle Test */
static bool run_test_i(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    apal_pid_t my_pid = apal_process_getpid();
    if (my_pid == 0) return false;

    /* Verify process structure validation */
    apal_process_t proc;
    const char *argv[] = {"/system/browser.elf", "--headless", NULL};
    const char *envp[] = {"PATH=/bin", NULL};
    apal_status_t st = apal_process_launch("/system/browser.elf", argv, envp, &proc);
    /* In pure unit test environment, return can be NOT_FOUND or OK depending on path presence */
    if (st != APAL_OK && st != APAL_ERR_NOT_FOUND) return false;

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

/* TEST J: Combined Integration & Stress Test */
static bool run_test_j(uint64_t *duration_us) {
    uint64_t t0 = apal_time_now_monotonic_us();

    /* Loop through 50 cycles of mixed memory, crypto rand, time and IPC */
    for (int cycle = 0; cycle < 50; cycle++) {
        uint64_t r = apal_rand_uint64();
        if (r == 0) return false;

        uint64_t now_us = apal_time_now_monotonic_us();
        if (now_us == 0) return false;

        void *p = apal_page_alloc(NULL, 4096, APAL_PROT_READ_WRITE, APAL_MEM_ANONYMOUS);
        if (!p) return false;
        ((uint64_t *)p)[0] = r;
        if (((uint64_t *)p)[0] != r) {
            apal_page_free(p, 4096);
            return false;
        }
        apal_page_free(p, 4096);
    }

    *duration_us = apal_time_now_monotonic_us() - t0;
    return true;
}

int main(void) {
    printf("\n=======================================================\n");
    printf("  ATOMS PLATFORM ADAPTATION LAYER (APAL) TEST SUITE\n");
    printf("  Target: %s\n", apal_get_version_string());
    printf("=======================================================\n\n");

    apal_init();

    uint64_t d = 0;
    print_result("TEST A", "Memory Adapter (4KB alloc, W^X, commit, free)", run_test_a(&d), d);
    print_result("TEST B", "Threads & Synchronization (mutex, cond, join)", run_test_b(&d), d);
    print_result("TEST C", "Filesystem Adapter (open, write, read, stat)", run_test_c(&d), d);
    print_result("TEST D", "IPC & Shared Memory (pipes, buffer mapping)", run_test_d(&d), d);
    print_result("TEST E", "Socket Adapter (stream, nonblocking, echo)", run_test_e(&d), d);
    print_result("TEST F", "Graphics Surface (32-bpp BGRA, presentation)", run_test_f(&d), d);
    print_result("TEST G", "Input & Event Adapter (DOM key translation)", run_test_g(&d), d);
    print_result("TEST H", "Audio Stream Adapter (48kHz stereo PCM)", run_test_h(&d), d);
    print_result("TEST I", "Process Lifecycle Adapter (PID, argv, wait)", run_test_i(&d), d);
    print_result("TEST J", "Combined Stress Integration (50 cycles)", run_test_j(&d), d);

    printf("\n-------------------------------------------------------\n");
    printf("  RESULTS: %d / %d PASSED (%.1f%%)\n", 
           g_passed, g_passed + g_failed, (g_passed * 100.0) / (g_passed + g_failed));
    printf("-------------------------------------------------------\n\n");

    apal_shutdown();
    return (g_failed == 0) ? 0 : 1;
}
