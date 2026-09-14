/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Audio Output Implementation
 */

#include "apal_audio.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define MAX_AUDIO_STREAMS 4
#define AUDIO_STREAM_QUEUE_SIZE 32768

typedef struct {
    bool in_use;
    apal_audio_config_t config;
    uint8_t buffer[AUDIO_STREAM_QUEUE_SIZE];
    size_t head;
    size_t tail;
    size_t queued_bytes;
} apal_audio_stream_internal_t;

static apal_audio_stream_internal_t g_audio_streams[MAX_AUDIO_STREAMS];
static bool g_audio_init = false;

static void ensure_audio_init(void) {
    if (!g_audio_init) {
        memset(g_audio_streams, 0, sizeof(g_audio_streams));
        g_audio_init = true;
    }
}

apal_status_t apal_audio_stream_open(const apal_audio_config_t *config, apal_audio_stream_t *out_stream) {
    if (!config || !out_stream) return APAL_ERR_INVALID_PARAM;
    ensure_audio_init();

    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (!g_audio_streams[i].in_use) {
            g_audio_streams[i].in_use = true;
            g_audio_streams[i].config = *config;
            g_audio_streams[i].head = 0;
            g_audio_streams[i].tail = 0;
            g_audio_streams[i].queued_bytes = 0;

            *out_stream = (apal_audio_stream_t)(i + 1);
            return APAL_OK;
        }
    }
    return APAL_ERR_NO_MEMORY;
}

int64_t apal_audio_stream_write(apal_audio_stream_t stream, const void *pcm_data, size_t bytes) {
    if (stream == 0 || !pcm_data || bytes == 0) return -1;
    int idx = (int)(stream - 1);
    if (idx < 0 || idx >= MAX_AUDIO_STREAMS || !g_audio_streams[idx].in_use) {
        return -1;
    }

    apal_audio_stream_internal_t *s = &g_audio_streams[idx];
    size_t space = AUDIO_STREAM_QUEUE_SIZE - s->queued_bytes;
    size_t to_write = (bytes < space) ? bytes : space;

    for (size_t i = 0; i < to_write; i++) {
        s->buffer[s->head] = ((const uint8_t *)pcm_data)[i];
        s->head = (s->head + 1) % AUDIO_STREAM_QUEUE_SIZE;
    }
    s->queued_bytes += to_write;

    return (int64_t)to_write;
}

apal_status_t apal_audio_stream_flush(apal_audio_stream_t stream) {
    if (stream == 0) return APAL_ERR_INVALID_PARAM;
    int idx = (int)(stream - 1);
    if (idx < 0 || idx >= MAX_AUDIO_STREAMS || !g_audio_streams[idx].in_use) {
        return APAL_ERR_NOT_FOUND;
    }

    /* Clear playback queue */
    g_audio_streams[idx].head = 0;
    g_audio_streams[idx].tail = 0;
    g_audio_streams[idx].queued_bytes = 0;
    return APAL_OK;
}

apal_status_t apal_audio_stream_close(apal_audio_stream_t stream) {
    if (stream == 0) return APAL_ERR_INVALID_PARAM;
    int idx = (int)(stream - 1);
    if (idx < 0 || idx >= MAX_AUDIO_STREAMS || !g_audio_streams[idx].in_use) {
        return APAL_ERR_NOT_FOUND;
    }

    g_audio_streams[idx].in_use = false;
    g_audio_streams[idx].queued_bytes = 0;
    return APAL_OK;
}
