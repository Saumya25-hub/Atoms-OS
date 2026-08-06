/**
 * @file bos_cursor.h
 * @brief BOS Cursor Engine (BCE) V1.0 - Master Header & Data Structures
 * @section PURPOSE
 * Production-grade cursor management subsystem for ATOMS OS comparable to
 * Windows win32k cursor manager and Linux DRM/KMS + libXcursor.
 */

#ifndef BOS_CURSOR_ENGINE_H
#define BOS_CURSOR_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BCE_OK = 0,
    BCE_ERR_INVALID_PARAM,
    BCE_ERR_CORRUPT_DATA,
    BCE_ERR_NO_MEMORY,
    BCE_ERR_UNSUPPORTED,
    BCE_ERR_NOT_FOUND,
    BCE_ERR_GPU_FAIL
} bce_error_t;

typedef enum {
    BCE_CURSOR_ARROW = 0,
    BCE_CURSOR_HAND,
    BCE_CURSOR_IBEAM,
    BCE_CURSOR_WAIT,
    BCE_CURSOR_APPSTARTING,
    BCE_CURSOR_CROSSHAIR,
    BCE_CURSOR_RESIZE_NS,
    BCE_CURSOR_RESIZE_WE,
    BCE_CURSOR_RESIZE_NWSE,
    BCE_CURSOR_RESIZE_NESW,
    BCE_CURSOR_MOVE,
    BCE_CURSOR_HELP,
    BCE_CURSOR_PRECISION,
    BCE_CURSOR_UNAVAILABLE,
    BCE_CURSOR_CUSTOM,
    BCE_CURSOR_TYPE_COUNT
} bce_cursor_type_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t bpp;
    uint32_t* argb_pixels; /* 32-bit ARGB Premultiplied Alpha */
} bce_frame_t;

typedef struct {
    uint32_t id;
    bce_cursor_type_t type;
    uint32_t frame_count;
    bce_frame_t* frames;
    uint32_t ref_count;
    bool is_animated;
} bce_cursor_t;

/* Master Subsystem API */
bce_error_t bos_cursor_subsystem_init(void);
void        bos_cursor_subsystem_shutdown(void);
bce_error_t bos_cursor_set_active_type(bce_cursor_type_t type);
bce_error_t bos_cursor_set_custom(bce_cursor_t* cursor);
void        bos_cursor_tick(void);
bce_cursor_t* bos_cursor_get_current(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_ENGINE_H */
