/*
 * BOSPECTRA V3 — Statistics Manager Subsystem
 * kernel/media/bospectra/runtime/statistics_manager.h
 */

#ifndef BOSPECTRA_V3_STATISTICS_MANAGER_H
#define BOSPECTRA_V3_STATISTICS_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t total_sessions_created;
    uint64_t total_frames_decoded_lifetime;
    uint64_t total_frames_rendered_lifetime;
    uint64_t total_bytes_read_lifetime;
} BOSPECTRA_GlobalStatistics;

void                       bospectra_statistics_manager_init(void);
void                       bospectra_statistics_manager_shutdown(void);

BOSPECTRA_GlobalStatistics bospectra_statistics_get_global(void);

#endif /* BOSPECTRA_V3_STATISTICS_MANAGER_H */
