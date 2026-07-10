#ifndef ATOMS_OS_DISPLAY_MANAGER_H
#define ATOMS_OS_DISPLAY_MANAGER_H

/**
 * @file display_manager.h
 * @brief ATOMS OS Display Intelligence Engine (DIE) V1 Master Public Header
 * Authoritative subsystem for display detection, capability analysis, policy evaluation,
 * and centralized geometry/work-area calculation across ATOMS OS.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Maximum Constants --- */
#define DIE_MAX_DISPLAYS        4
#define DIE_MAX_MODES           32
#define DIE_DEFAULT_DPI_SCALE   100 /* 100 = 100% (1.0x native pixel mapping) */

/* --- Detected Environment Types --- */
typedef enum {
    DIE_ENV_UNKNOWN         = 0,
    DIE_ENV_PHYSICAL_PC     = 1,
    DIE_ENV_QEMU            = 2,
    DIE_ENV_VIRTUALBOX      = 3,
    DIE_ENV_VMWARE          = 4,
    DIE_ENV_HYPERV          = 5,
    DIE_ENV_UEFI_GOP        = 6,
    DIE_ENV_PCI_GPU         = 7
} DIE_EnvironmentType;

/* --- Display Capability Mode Descriptor --- */
typedef struct {
    uint32_t mode_id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch_bytes;
    uint32_t bpp;
    uint32_t refresh_rate_hz;
    bool is_preferred;
    bool is_supported;
    uint32_t policy_score;
} DIE_DisplayMode;

/* --- Display Capability List --- */
typedef struct {
    DIE_DisplayMode modes[DIE_MAX_MODES];
    uint32_t mode_count;
    uint32_t preferred_index;
} DIE_CapabilityList;

/* --- Centralized Display Geometry Specification (All 9 Core Rects) --- */
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} DIE_Rect;

typedef struct {
    DIE_Rect desktop_rect;
    DIE_Rect wallpaper_rect;
    DIE_Rect taskbar_rect;
    DIE_Rect notification_area;
    DIE_Rect popup_area;
    DIE_Rect window_work_area;
    DIE_Rect cursor_bounds;
    DIE_Rect safe_area;
    DIE_Rect dock_area;
} DIE_DisplayGeometry;

/* --- Policy Evaluation Verdict --- */
typedef struct {
    DIE_DisplayMode selected_mode;
    DIE_EnvironmentType environment;
    uint32_t dpi_scale;
    const char* policy_rationale;
} DIE_PolicyDecision;

/* --- Unified Authoritative Display Info Structure --- */
typedef struct {
    uint32_t display_id;
    bool is_active;
    DIE_EnvironmentType environment;
    char environment_name[32];
    char controller_name[64];
    
    uint64_t framebuffer_paddr;
    void* framebuffer_vaddr;
    uint32_t vram_size_bytes;
    
    DIE_DisplayMode active_mode;
    DIE_CapabilityList capabilities;
    DIE_DisplayGeometry geometry;
    DIE_PolicyDecision policy_decision;
    
    uint32_t dpi_scale;
    uint64_t last_reconfigure_time_us;
} DIE_DisplayInfo;

/* --- Subsystem Module Prototypes --- */

/**
 * @brief Initialize the entire Display Intelligence Engine V1.
 */
void DIE_Initialize(void);

/**
 * @brief Get authoritative pointer to the primary display info.
 */
DIE_DisplayInfo* DIE_GetPrimaryDisplay(void);

/**
 * @brief Get authoritative display info by ID (0..DIE_MAX_DISPLAYS-1).
 */
DIE_DisplayInfo* DIE_GetDisplay(uint32_t display_id);

/**
 * @brief Get centralized geometry definition for active primary display.
 */
const DIE_DisplayGeometry* DIE_GetPrimaryGeometry(void);

/**
 * @brief Execute policy evaluation across capabilities to select optimal display mode.
 */
bool DIE_EvaluateAndApplyPolicy(uint32_t display_id);

/**
 * @brief Sync computed geometry with legacy global variables and AGDTE surfaces.
 */
void DIE_SyncWithKernelAndAGDTE(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_MANAGER_H */
