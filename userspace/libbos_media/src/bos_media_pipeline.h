/*
 * ============================================================================
 * ATOMS OS — Userspace Native Media Pipeline Manager
 * userspace/libbos_media/src/bos_media_pipeline.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Direct integration of real demuxers (MP4/WAV/MP3) and decoders (H.264 CABAC/minimp3/dr_wav)
 * with BOSurface v2.5 and Audio HAL.
 * ============================================================================
 */

#ifndef BOS_MEDIA_PIPELINE_H
#define BOS_MEDIA_PIPELINE_H

#include "../include/bos_media.h"
#include "../include/bos_media_stream.h"
#include "../demux/mp4_demuxer.h"
#include "../audio/mp3_decoder.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BOSMediaPipeline BOSMediaPipeline;

BOSMediaPipeline* bos_media_pipeline_create(void);
void              bos_media_pipeline_destroy(BOSMediaPipeline* p);

int               bos_media_pipeline_open(BOSMediaPipeline* p, const char* uri);
int               bos_media_pipeline_play(BOSMediaPipeline* p);
int               bos_media_pipeline_pause(BOSMediaPipeline* p);
int               bos_media_pipeline_stop(BOSMediaPipeline* p);
int               bos_media_pipeline_seek(BOSMediaPipeline* p, int64_t position_ms);
int               bos_media_pipeline_set_volume(BOSMediaPipeline* p, float volume);
int               bos_media_pipeline_set_mute(BOSMediaPipeline* p, bool mute);

int               bos_media_pipeline_render_frame(BOSMediaPipeline* p, uint32_t* target_fb, int target_w, int target_h, int stride_pixels);
void              bos_media_pipeline_tick(BOSMediaPipeline* p);

BOSMediaState     bos_media_pipeline_get_state(BOSMediaPipeline* p);
int               bos_media_pipeline_get_position(BOSMediaPipeline* p, int64_t* out_position_ms);
int               bos_media_pipeline_get_duration(BOSMediaPipeline* p, int64_t* out_duration_ms);
int               bos_media_pipeline_get_metadata(BOSMediaPipeline* p, BOSMediaMetadata* out_metadata);
int               bos_media_pipeline_get_telemetry(BOSMediaPipeline* p, BOSMediaTelemetry* out_telemetry);

#ifdef __cplusplus
}
#endif

#endif /* BOS_MEDIA_PIPELINE_H */
