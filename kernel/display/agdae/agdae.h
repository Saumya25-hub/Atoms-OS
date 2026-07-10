/**
 * @file agdae.h
 * @brief ATOMS OS Display Adaptation Engine (AGDAE) - Core Header
 *
 * Provides a single source of truth for all display geometry, scaling, and layout
 * metrics across the operating system.
 */

#ifndef _AGDAE_H_
#define _AGDAE_H_

#include <stdint.h>
#include <stdbool.h>

/* Display Geometry Rect (Compatible with BWE) */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} AGDAE_Rect;

/* Core Metrics provided by AGDAE to all Subsystems */
typedef struct {
    /* 1. Resolutions */
    uint32_t physical_width;
    uint32_t physical_height;
    
    uint32_t logical_width;
    uint32_t logical_height;

    /* 2. Scaling (Percentage-based, e.g., 100, 125, 150) */
    uint32_t scale_factor_pct;

    /* 3. Global Coordinates (The Single Source of Truth) */
    AGDAE_Rect desktop_rect;       /* Full area available for desktop rendering */
    AGDAE_Rect wallpaper_rect;     /* Physical background fill area */
    AGDAE_Rect taskbar_rect;       /* Precise bounds for the Taskbar / Dock */
    AGDAE_Rect window_work_area;   /* Usable area for maximized windows */
    AGDAE_Rect safe_area;          /* Region guaranteed to be visible (avoids overscan/clipping) */
    AGDAE_Rect notification_area;  /* Valid region for popup alerts */
    
    /* 4. Display Flags */
    bool is_scaled;
    bool is_letterboxed;           /* True if logical aspect ratio differs from physical */
    bool is_hidpi;                 /* True if scaling >= 150% */
} AGDAE_Metrics;

/* Core Initialization */
void AGDAE_Initialize(uint32_t hw_physical_w, uint32_t hw_physical_h, uint32_t optimal_logical_w, uint32_t optimal_logical_h);

/* Fetch Global Metrics */
const AGDAE_Metrics* AGDAE_GetMetrics(void);

/* Scaling Utilities (Integer Math Only) */
int32_t AGDAE_Scale(int32_t value);
int32_t AGDAE_Unscale(int32_t scaled_value);

/* Geometry Calculation (Internal but accessible) */
void AGDAE_Geometry_Update(AGDAE_Metrics* metrics);

/* Diagnostic Report */
void AGDAE_DumpDiagnostics(void);

#endif /* _AGDAE_H_ */
