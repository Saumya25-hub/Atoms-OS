#ifndef CONTAINER_REGISTRY_H
#define CONTAINER_REGISTRY_H

#include "../../include/bospectra_types.h"
#include "../../file/bospectra_file.h"
#include "../../packet/bospectra_packet.h"
#include "../include/bospectra_demux.h"

// Polymorphic Container Driver Interface
typedef struct BOSPECTRA_ContainerDriver {
    const char* format_name;
    const char* extensions;

    // Sniff file header (returns confidence score 0 to 100)
    int (*probe)(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len);

    // Open session & parse metadata
    bospectra_error_t (*open)(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta);

    // Get stream descriptor by index
    bospectra_error_t (*get_stream)(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc);

    // Demux next elementary packet
    bospectra_error_t (*read_packet)(void* driver_ctx, BOSPacket** out_pkt);

    // Seek to timestamp (microseconds)
    bospectra_error_t (*seek)(void* driver_ctx, uint64_t timestamp_us);

    // Close session & cleanup context
    bospectra_error_t (*close)(void* driver_ctx);
} BOSPECTRA_ContainerDriver;

void                             bospectra_container_registry_init(void);
void                             bospectra_container_registry_shutdown(void);
bospectra_error_t                bospectra_container_register_driver(const BOSPECTRA_ContainerDriver* driver);
const BOSPECTRA_ContainerDriver* bospectra_container_find_driver_by_name(const char* name);
const BOSPECTRA_ContainerDriver* bospectra_container_probe_driver(bospectra_file_id_t file_id);

#endif // CONTAINER_REGISTRY_H
