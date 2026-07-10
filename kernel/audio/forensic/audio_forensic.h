#ifndef AUDIO_FORENSIC_H
#define AUDIO_FORENSIC_H

#define MAX_EVENTS 100000

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    EV_PIT_TICK = 1,
    EV_SCHEDULER_WAKE = 2,
    EV_AUDIO_THREAD_START = 3,
    EV_AUDIO_THREAD_END = 4,
    EV_VFS_READ_START = 5,
    EV_VFS_READ_END = 6,
    EV_STREAM_WRITE = 7,
    EV_STREAM_READ = 8,
    EV_STREAM_STARVED = 9,
    EV_MIXER_START = 10,
    EV_MIXER_END = 11,
    EV_DMA_REFILL_START = 12,
    EV_DMA_REFILL_END = 13,
    EV_DMA_UNDERRUN = 14,
    EV_PRESENT_START = 15,
    EV_PRESENT_END = 16
} AudioEventType;

void audio_forensic_init(void);
void audio_forensic_record(AudioEventType type, uint32_t data1, uint32_t data2);
void audio_forensic_dump(void);
void audio_forensic_reset(void);

// Keep previous prototypes so we don't break compile
void audio_forensic_log_pcm_gen(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_stream_write(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_stream_read(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_mixer_out(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_dma_out(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_dump_oscilloscope(void);

#endif // AUDIO_FORENSIC_H
