/*
 * BOSPECTRA V3 — Dynamic Container Registry
 * kernel/media/bospectra/registry/container_registry.h
 *
 * Single source of truth for container driver registration, priority resolution, and inspection.
 */

#ifndef BOSPECTRA_V3_CONTAINER_REGISTRY_H
#define BOSPECTRA_V3_CONTAINER_REGISTRY_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../file/bospectra_file.h"
#include "../container/include/bospectra_demux.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BOSPECTRA_MAX_CONTAINER_DRIVERS 16U

typedef struct {
    const char* format_name;
    const char* extensions;
    uint32_t    priority; /* 0..100 */
    int (*probe)(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len);
    bospectra_error_t (*open)(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta);
    bospectra_error_t (*get_stream)(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc);
    bospectra_error_t (*read_packet)(void* driver_ctx, BOSPacket** out_pkt);
    bospectra_error_t (*seek)(void* driver_ctx, uint64_t timestamp_us);
    bospectra_error_t (*close)(void* driver_ctx);
} BOSPECTRA_ContainerDriverRecord;

void bospectra_v3_container_registry_init(void);
void bospectra_v3_container_registry_shutdown(void);

bospectra_error_t bospectra_v3_container_register(const BOSPECTRA_ContainerDriverRecord* driver);
const BOSPECTRA_ContainerDriverRecord* bospectra_v3_container_find_by_name(const char* name);
uint32_t bospectra_v3_container_get_registered(const BOSPECTRA_ContainerDriverRecord** out_drivers, uint32_t max_count);

#endif /* BOSPECTRA_V3_CONTAINER_REGISTRY_H */
