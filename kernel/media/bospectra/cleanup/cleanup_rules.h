/*
 * BOSPECTRA V3 — Cleanup Rules Subsystem
 * kernel/media/bospectra/cleanup/cleanup_rules.h
 */

#ifndef BOSPECTRA_V3_CLEANUP_RULES_H
#define BOSPECTRA_V3_CLEANUP_RULES_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void bospectra_cleanup_rules_init(void);
void bospectra_cleanup_rules_shutdown(void);

bool bospectra_cleanup_can_destroy(uint32_t object_id, uint32_t ref_count, uint32_t child_count);

#endif /* BOSPECTRA_V3_CLEANUP_RULES_H */
