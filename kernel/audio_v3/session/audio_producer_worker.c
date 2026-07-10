#include "kernel/audio/session/audio_producer_worker.h"
#include "kernel/audio/session/audio_player.h"
#include "kernel/audio/api/audio_api.h"
#include <stddef.h>

static AudioProducerTelemetry g_prod_telemetry = {
    .min_occupancy_pct = 100,
    .avg_occupancy_pct = 80,
    .max_occupancy_pct = 0,
    .current_occupancy_pct = 0,
    .is_refilling = false
};

static uint64_t g_occupancy_acc = 0;
static uint64_t g_occupancy_samples = 0;

void audio_producer_worker_init(void) {
    g_prod_telemetry.total_chunks_produced = 0;
    g_prod_telemetry.total_bytes_produced = 0;
    g_prod_telemetry.total_read_time_ticks = 0;
    g_prod_telemetry.min_occupancy_pct = 100;
    g_prod_telemetry.avg_occupancy_pct = 80;
    g_prod_telemetry.max_occupancy_pct = 0;
    g_prod_telemetry.current_occupancy_pct = 0;
    g_prod_telemetry.is_refilling = false;
    g_occupancy_acc = 0;
    g_occupancy_samples = 0;
}

/**
 * @brief Low-Priority Audio Producer Worker Hook.
 *
 * Runs in the background (audio_service_entry) to monitor ring buffer watermarks
 * and perform asynchronous/staged VFS/FAT32 reads without blocking the realtime consumer pump.
 */
void audio_producer_worker_run(void) {
    if (!audio_player_is_playing()) {
        return;
    }

    uint32_t stream_id = 0;
    uint32_t bytes_played = 0;
    uint32_t data_size = 0;
    uint64_t read_time = 0;
    audio_player_get_diag_info(&stream_id, &bytes_played, &data_size, &read_time);

    size_t available = audio_stream_available(stream_id);
    size_t capacity = audio_stream_capacity(stream_id);
    if (capacity == 0) return;

    uint32_t occ_pct = (uint32_t)((available * 100) / capacity);
    g_prod_telemetry.current_occupancy_pct = occ_pct;

    if (occ_pct < g_prod_telemetry.min_occupancy_pct) g_prod_telemetry.min_occupancy_pct = occ_pct;
    if (occ_pct > g_prod_telemetry.max_occupancy_pct) g_prod_telemetry.max_occupancy_pct = occ_pct;
    
    g_occupancy_acc += occ_pct;
    g_occupancy_samples++;
    g_prod_telemetry.avg_occupancy_pct = (uint32_t)(g_occupancy_acc / g_occupancy_samples);

    // Watermark state machine: begin refilling immediately at or below LOW watermark (50%)
    if (occ_pct <= PRODUCER_WATERMARK_LOW_PCT) {
        g_prod_telemetry.is_refilling = true;
    } else if (occ_pct >= PRODUCER_WATERMARK_HIGH_PCT) {
        g_prod_telemetry.is_refilling = false;
    }

    // Execute producer update if refilling or in initial buffer state
    if (g_prod_telemetry.is_refilling || occ_pct < PRODUCER_WATERMARK_TARGET_PCT) {
        uint32_t before_played = bytes_played;
        audio_player_update();
        
        audio_player_get_diag_info(&stream_id, &bytes_played, &data_size, &read_time);
        if (bytes_played != before_played) {
            g_prod_telemetry.total_chunks_produced++;
            uint32_t produced = (bytes_played >= before_played) ? (bytes_played - before_played) : bytes_played;
            g_prod_telemetry.total_bytes_produced += produced;
            g_prod_telemetry.total_read_time_ticks += read_time;
        }
    }
}

void audio_producer_worker_get_telemetry(AudioProducerTelemetry* out_stats) {
    if (out_stats) {
        *out_stats = g_prod_telemetry;
    }
}
