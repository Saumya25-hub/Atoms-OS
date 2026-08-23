#ifndef ATOMS_ICON_ENGINE_H
#define ATOMS_ICON_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ATOMS OS — PRODUCTION VECTOR ICON ENGINE (V1.0)
 * Deterministic, Resolution-Independent, Zero-Allocation Icon Subsystem
 * ========================================================================= */

/**
 * @brief Canonical Icon Identifiers
 */
typedef enum {
    ICON_ID_NONE = 0,
    ICON_ID_ATOMS_START,      /**< Authoritative ATOMS System Emblem */
    
    // Core Desktop Application Icons
    ICON_ID_COMPUTER,         /**< Computer / Workstation / This PC */
    ICON_ID_EXPLORER,         /**< File Explorer */
    ICON_ID_TERMINAL,         /**< ATOMS Console / Shell */
    ICON_ID_NOTES,            /**< Notes & Text Editor */
    ICON_ID_CALCULATOR,       /**< Bishop Scientific Calculator */
    ICON_ID_SETTINGS,         /**< System Configuration */
    ICON_ID_MEDIA_PLAYER,     /**< BOSpectra Media Suite */
    ICON_ID_ATRIX,            /**< ATRIX Web Browser Engine */
    ICON_ID_TASK_MANAGER,     /**< TMH Performance Monitor */
    ICON_ID_CONTROL_PANEL,    /**< Hardware & Device Management */
    ICON_ID_DOOM,             /**< DOOM Classical Game */
    ICON_ID_GRAPH_3D,         /**< 3D Graphics Benchmark */
    ICON_ID_INPUT_LAB,        /**< InputLab Diagnostics */
    ICON_ID_FOLDER,           /**< Directory / Folder */
    ICON_ID_RECYCLE_BIN,      /**< Trash / Recycle Bin */
    ICON_ID_USB_DISK,         /**< Removable USB Disk */

    // System Status & Tray Glyphs
    ICON_ID_SYS_ETHERNET,     /**< Wired Gigabit LAN / Ethernet */
    ICON_ID_SYS_LAN,          /**< Canonical LAN identifier */
    ICON_ID_SYS_WIFI,
    ICON_ID_SYS_VOLUME,
    ICON_ID_SYS_BATTERY,
    ICON_ID_SYS_BELL,
    ICON_ID_SYS_POWER,
    ICON_ID_SYS_SEARCH,

    ICON_ID_MAX
} IconId;

/**
 * @brief Interactive Visual States
 */
typedef enum {
    ICON_STATE_NORMAL = 0,   /**< Default resting state */
    ICON_STATE_HOVER,        /**< Pointer hovering over icon */
    ICON_STATE_PRESSED,      /**< Mouse button actively depressed */
    ICON_STATE_ACTIVE,       /**< Associated window/menu is open/focused */
    ICON_STATE_DISABLED      /**< Inactive / greyed out */
} IconState;

/**
 * @brief Execution Context for Rendering an Icon
 */
typedef struct {
    int32_t         x;            /**< Target top-left X coordinate */
    int32_t         y;            /**< Target top-left Y coordinate */
    int32_t         width;        /**< Target pixel width (e.g. 16, 20, 24, 32) */
    int32_t         height;       /**< Target pixel height (e.g. 16, 20, 24, 32) */
    IconState       state;        /**< Interaction state */
    uint32_t        accent_color; /**< Custom accent override (0 = default theme) */
    const BWE_Rect* clip;         /**< BWE clipping rectangle (NULL = viewport) */
} IconRenderContext;

/**
 * @brief Procedural Icon Renderer Function Signature
 */
typedef bool (*IconRendererFn)(const BVFramebuffer* fb, const IconRenderContext* ctx);

/* =========================================================================
 * Engine Public API
 * ========================================================================= */

/**
 * @brief Initialize Icon Engine and register core procedural icons
 */
void IconEngine_Initialize(void);

/**
 * @brief Register a procedural icon renderer
 */
bool IconEngine_Register(IconId id, IconRendererFn renderer);

/**
 * @brief Render an icon into the specified framebuffer context
 * @return true if rendered successfully, false otherwise
 */
bool IconEngine_Render(const BVFramebuffer* fb, IconId id, const IconRenderContext* ctx);

/**
 * @brief Check if an icon renderer is registered
 */
bool IconEngine_HasIcon(IconId id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_ICON_ENGINE_H */
