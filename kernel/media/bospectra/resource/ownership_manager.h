/*
 * BOSPECTRA V3 — Production Resource Ownership Framework
 * kernel/media/bospectra/resource/ownership_manager.h
 *
 * Defines explicit resource owner types, ownership table entries, and transfer rules.
 */

#ifndef BOSPECTRA_V3_OWNERSHIP_MANAGER_H
#define BOSPECTRA_V3_OWNERSHIP_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OWNER_TYPE_NONE = 0,
    OWNER_TYPE_MEDIA_MANAGER,
    OWNER_TYPE_PLAYBACK_SESSION,
    OWNER_TYPE_CONTAINER,
    OWNER_TYPE_PACKET_QUEUE,
    OWNER_TYPE_DECODER,
    OWNER_TYPE_FRAME_QUEUE,
    OWNER_TYPE_COLOR_ENGINE,
    OWNER_TYPE_RENDERER_QUEUE,
    OWNER_TYPE_DISPLAY_SCHEDULER,
    OWNER_TYPE_COMPOSITOR_BWE
} BOSPECTRA_OwnerType;

typedef struct {
    uint32_t            resource_id;
    BOSPECTRA_OwnerType owner_type;
    uint32_t            owner_id;
    uint32_t            creator_id;
    uint32_t            generation_id;
    bool                is_valid;
} BOSPECTRA_OwnershipEntry;

void              bospectra_ownership_manager_init(void);
void              bospectra_ownership_manager_shutdown(void);

bospectra_error_t bospectra_ownership_register(uint32_t res_id, BOSPECTRA_OwnerType owner_type, uint32_t owner_id);
bospectra_error_t bospectra_ownership_transfer(uint32_t res_id, BOSPECTRA_OwnerType new_owner_type, uint32_t new_owner_id);
bospectra_error_t bospectra_ownership_unregister(uint32_t res_id);

bool              bospectra_ownership_verify(uint32_t res_id, BOSPECTRA_OwnerType expected_type, uint32_t expected_id);
uint32_t          bospectra_ownership_get_violations_count(void);

#endif /* BOSPECTRA_V3_OWNERSHIP_MANAGER_H */
