#ifndef AUDIO_FORENSIC_H
#define AUDIO_FORENSIC_H

#include <stdint.h>
#include <stddef.h>

#define FORENSIC_SAMPLES_COUNT 2048

void audio_forensic_init(void);
void audio_forensic_log_pcm_gen(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_stream_write(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_stream_read(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_mixer_out(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_log_dma_out(const uint8_t* pcm_data, size_t bytes);
void audio_forensic_dump_oscilloscope(void);

#endif
