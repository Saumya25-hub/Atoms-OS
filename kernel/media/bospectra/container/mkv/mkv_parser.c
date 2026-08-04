#include "mkv_parser.h"
#include "../common/container_common.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

// MKV Driver Probing
static int mkv_probe(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len) {
    (void)file_id;
    if (!header_data || header_len < 4) return 0;

    uint32_t ebml_id = bospectra_read_u32_be(header_data);
    if (ebml_id == EBML_ID_HEADER) {
        return 100;
    }

    return 0;
}

// MKV Driver Open Stub
static bospectra_error_t mkv_open(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta) {
    (void)file_id;
    if (!driver_ctx || !out_meta) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    out_meta->duration_us = 0;
    out_meta->stream_count = 1;
    out_meta->video_stream_count = 1;
    strncpy(out_meta->format_name, "MKV", sizeof(out_meta->format_name) - 1);
    strncpy(out_meta->container_brand, "Matroska", sizeof(out_meta->container_brand) - 1);

    *driver_ctx = (void*)(uintptr_t)1; // Stub context
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mkv_get_stream(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc) {
    (void)driver_ctx;
    if (!out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (stream_index > 0) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    memset(out_desc, 0, sizeof(BOSPECTRA_StreamDescriptor));
    out_desc->id = 1;
    out_desc->type = BOSPECTRA_STREAM_VIDEO;
    out_desc->width = 1920;
    out_desc->height = 1080;
    out_desc->is_active = true;
    strncpy(out_desc->codec_name, "VP9", sizeof(out_desc->codec_name) - 1);

    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mkv_read_packet(void* driver_ctx, BOSPacket** out_pkt) {
    (void)driver_ctx;
    (void)out_pkt;
    return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
}

static bospectra_error_t mkv_seek(void* driver_ctx, uint64_t timestamp_us) {
    (void)driver_ctx;
    (void)timestamp_us;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mkv_close(void* driver_ctx) {
    (void)driver_ctx;
    return BOSPECTRA_SUCCESS;
}

// MKV Driver Vtable Definition
const BOSPECTRA_ContainerDriver g_mkv_container_driver = {
    .format_name = "MKV",
    .extensions  = "mkv,webm",
    .probe       = mkv_probe,
    .open        = mkv_open,
    .get_stream  = mkv_get_stream,
    .read_packet = mkv_read_packet,
    .seek        = mkv_seek,
    .close       = mkv_close
};
