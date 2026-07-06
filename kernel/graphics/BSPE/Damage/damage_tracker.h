#ifndef ATOMS_OS_BSPE_DAMAGE_TRACKER_H
#define ATOMS_OS_BSPE_DAMAGE_TRACKER_H

/**
 * @file damage_tracker.h
 * @brief BSPE Damage Tracker Public Header
 * @status Step 6 Production Implementation
 * 
 * @section PURPOSE
 * Defines the hierarchical, dual-page damage tracking API.
 * Computes exact visible damage unions without rendering, VRAM access, or heap allocations.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Configuration Struct --- */
typedef struct {
    uint32_t max_rects_per_page; /* Fixed memory limit (default 32) */
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t reserved[4];        /* Future extension reserve */
} BSPE_DamageTrackerConfig;

/* --- Region Struct Definition --- */
#ifndef BOGE_REGION_DEFINED
#define BOGE_REGION_DEFINED
typedef struct {
    BOGE_Rect rects[32];
    uint32_t rect_count;
} BOGE_Region;
#endif

/* --- Public Lifecycle APIs --- */
BSPE_Error BSPE_DamageTracker_Create(const BSPE_DamageTrackerConfig* config, BSPE_DamageTrackerHandle* out_handle);
void       BSPE_DamageTracker_Destroy(BSPE_DamageTrackerHandle handle);
void       BSPE_DamageTracker_Reset(BSPE_DamageTrackerHandle handle);
void       BSPE_DamageTracker_Clear(BSPE_DamageTrackerHandle handle);
void       BSPE_DamageTracker_AdvanceFrame(BSPE_DamageTrackerHandle handle);

/* --- Core Damage Submission & Merging APIs --- */
BSPE_Error BSPE_DamageTracker_AddRect(BSPE_DamageTrackerHandle handle, const BOGE_Rect* rect);
BSPE_Error BSPE_DamageTracker_AddRegion(BSPE_DamageTrackerHandle handle, const BOGE_Region* region);
BSPE_Error BSPE_DamageTracker_AddWindowDamage(BSPE_DamageTrackerHandle handle, uint32_t window_id, const BOGE_Rect* rect);

/* --- Aliases & Helper APIs Required by Step 6 Specification --- */
static inline BSPE_Error BSPE_DamageTracker_InvalidateRect(BSPE_DamageTrackerHandle handle, const BOGE_Rect* rect) {
    return BSPE_DamageTracker_AddRect(handle, rect);
}
static inline BSPE_Error BSPE_DamageTracker_MergeRect(BSPE_DamageTrackerHandle handle, const BOGE_Rect* rect) {
    return BSPE_DamageTracker_AddRect(handle, rect);
}
uint32_t   BSPE_DamageTracker_GetDamageCount(BSPE_DamageTrackerHandle handle);

/* --- Effective Damage Evaluation (Union(N, N-1)) --- */
BSPE_Error BSPE_DamageTracker_GetEffectiveDamage(BSPE_DamageTrackerHandle handle, BOGE_Rect* out_rects, uint32_t max_rects, uint32_t* out_count);
BSPE_Error BSPE_DamageTracker_GetPreviousDamage(BSPE_DamageTrackerHandle handle, BOGE_Rect* out_rects, uint32_t max_rects, uint32_t* out_count);

/* --- Verification & Diagnostics --- */
bool       BSPE_DamageTracker_RunSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_DAMAGE_TRACKER_H
