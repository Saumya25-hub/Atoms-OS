/*
 * BOSPECTRA V3 — Reference Counting Subsystem
 * kernel/media/bospectra/resource/reference_manager.h
 *
 * Atomic reference counting, acquire, release, unref, and destroy operations for shared media resources.
 */

#ifndef BOSPECTRA_V3_REFERENCE_MANAGER_H
#define BOSPECTRA_V3_REFERENCE_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t resource_id;
    uint32_t ref_count;
    bool     is_active;
} BOSPECTRA_ReferenceControlBlock;

void              bospectra_reference_manager_init(void);
void              bospectra_reference_manager_shutdown(void);

bospectra_error_t bospectra_ref_register(uint32_t res_id);
bospectra_error_t bospectra_ref_add(uint32_t res_id);
bospectra_error_t bospectra_ref_release(uint32_t res_id, uint32_t* out_remaining_refs);
uint32_t          bospectra_ref_get_count(uint32_t res_id);
uint32_t          bospectra_ref_get_errors_count(void);

#endif /* BOSPECTRA_V3_REFERENCE_MANAGER_H */
