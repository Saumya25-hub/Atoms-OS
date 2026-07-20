#ifndef AUDIO_FORENSIC_H
#define AUDIO_FORENSIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint64_t timestamp;
    uint8_t civ;
    uint8_t lvi;
    bool dch;
    uint32_t software_ring_available;
    uint32_t bytes_played;
    uint32_t producer_refill_calls;
    uint32_t producer_bytes_read;
    uint32_t mixer_active_streams;
    uint32_t mixer_silence_bytes;
    uint32_t descriptor_checksum;
    
    // vfs_read latency telemetry
    uint64_t last_vfs_read_us;
    uint64_t max_vfs_read_us;
    uint32_t last_read_requested;
    uint32_t last_read_returned;
} AudioForensicSample;

void audio_forensic_init(void);
void audio_forensic_record_sample(uint8_t civ, uint8_t lvi, bool dch, uint32_t desc_checksum);
void audio_forensic_dump(const char* reason);
void audio_forensic_reset(void);

// Stubs for previous prototypes to prevent compile errors
void audio_forensic_log_pcm_gen(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_stream_write(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_stream_read(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_mixer_out(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_dma_out(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_dump_oscilloscope(void);

#endif
