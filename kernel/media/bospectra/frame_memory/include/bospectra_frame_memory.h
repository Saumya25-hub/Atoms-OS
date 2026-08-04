#ifndef BOSPECTRA_FRAME_MEMORY_H
#define BOSPECTRA_FRAME_MEMORY_H

#include "../../include/bospectra_types.h"
#include "bospectra_frame.h"

typedef struct {
    uint32_t frame_pool_capacity;
    uint32_t frame_pool_active;
    uint32_t frame_pool_peak;
    uint32_t packet_pool_capacity;
    uint32_t packet_pool_active;
    uint32_t packet_pool_peak;
    size_t   total_pool_bytes;
    uint64_t total_acquires;
    uint64_t total_releases;
    uint64_t allocation_failures;
    uint64_t double_free_errors;
    bool     is_zero_fragmented;
} BOSPECTRA_FrameMemoryStats;

// Master Frame Memory Engine Lifecycle
bospectra_error_t BOSPECTRA_FrameMemory_Init(void);
bospectra_error_t BOSPECTRA_FrameMemory_Shutdown(void);

// Master Stats & Diagnostics
void BOSPECTRA_FrameMemory_GetStats(BOSPECTRA_FrameMemoryStats* out_stats);
void BOSPECTRA_FrameMemory_DumpDiagnostics(void);

#endif // BOSPECTRA_FRAME_MEMORY_H
