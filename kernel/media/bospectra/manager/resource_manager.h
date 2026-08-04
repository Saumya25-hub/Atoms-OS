/*
 * BOSPECTRA V3 — Resource Manager
 * kernel/media/bospectra/manager/resource_manager.h
 *
 * Centralized allocation tracking, ownership, reference counting, and leak detection.
 */

#ifndef BOSPECTRA_RESOURCE_MANAGER_H
#define BOSPECTRA_RESOURCE_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOSPECTRA_RES_TYPE_PACKET = 1,
    BOSPECTRA_RES_TYPE_FRAME,
    BOSPECTRA_RES_TYPE_TEXTURE,
    BOSPECTRA_RES_TYPE_SURFACE,
    BOSPECTRA_RES_TYPE_BUFFER,
    BOSPECTRA_RES_TYPE_CONTEXT
} bospectra_resource_type_t;

typedef struct {
    uint32_t                 id;
    bospectra_resource_type_t type;
    void*                    ptr;
    size_t                   size;
    const char*              owner_name;
    uint32_t                 ref_count;
    bool                     is_active;
} BOSPECTRA_ResourceRecord;

typedef struct {
    uint32_t total_allocations;
    uint32_t active_allocations;
    size_t   total_bytes_allocated;
    uint32_t leaks_detected;
} BOSPECTRA_ResourceStats;

void bospectra_resource_manager_init(void);
void bospectra_resource_manager_shutdown(void);

bospectra_error_t bospectra_resource_track_alloc(void* ptr, size_t size, bospectra_resource_type_t type, const char* owner_name, uint32_t* out_id);
bospectra_error_t bospectra_resource_track_free(void* ptr);
bospectra_error_t bospectra_resource_ref(void* ptr);
bospectra_error_t bospectra_resource_unref(void* ptr);

void bospectra_resource_get_stats(BOSPECTRA_ResourceStats* out_stats);
void bospectra_resource_dump_leaks(void);

#endif /* BOSPECTRA_RESOURCE_MANAGER_H */
