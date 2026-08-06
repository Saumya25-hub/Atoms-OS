/**
 * @file bos_cursor.h
 * @brief BOS OS Hardware Cursor Subsystem V1.0 - Cursor HAL Interface
 * @section PURPOSE
 * Unified hardware cursor abstraction layer separating Input Engine from
 * Visual Cursor Manager and driving GPU Hardware Cursor Overlay Planes.
 */

#ifndef BOS_CURSOR_H
#define BOS_CURSOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOS_CURSOR_TYPE_NONE = 0,
    BOS_CURSOR_TYPE_ARROW,
    BOS_CURSOR_TYPE_HAND,
    BOS_CURSOR_TYPE_IBEAM,
    BOS_CURSOR_TYPE_RESIZE_NS,
    BOS_CURSOR_TYPE_RESIZE_EW,
    BOS_CURSOR_TYPE_RESIZE_NWSE,
    BOS_CURSOR_TYPE_RESIZE_NESW,
    BOS_CURSOR_TYPE_WAIT,
    BOS_CURSOR_TYPE_CUSTOM,
    BOS_CURSOR_TYPE_COUNT
} bos_cursor_type_t;

typedef enum {
    BOS_CURSOR_BACKEND_NONE = 0,
    BOS_CURSOR_BACKEND_SOFTWARE,
    BOS_CURSOR_BACKEND_HARDWARE
} bos_cursor_backend_t;

typedef struct {
    int32_t              x;
    int32_t              y;
    int32_t              delta_x;
    int32_t              delta_y;
    uint32_t             width;
    uint32_t             height;
    uint32_t             hotspot_x;
    uint32_t             hotspot_y;
    bool                 visible;
    bos_cursor_type_t    current_type;
    bos_cursor_backend_t active_backend;
    bool                 hardware_overlay_active;
    uint64_t             present_count;
    uint64_t             update_time_us;
} bos_cursor_state_t;

typedef struct {
    int32_t  x;
    int32_t  y;
    int32_t  delta_x;
    int32_t  delta_y;
    float    velocity;
    float    acceleration;
    uint32_t polling_rate_hz;
    uint32_t current_type;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t width;
    uint32_t height;
    bool     hardware_cursor;
    bool     overlay_plane;
    uint64_t surface_phys_addr;
    uint64_t present_count;
    uint32_t frame_age_ms;
    uint32_t latency_us;
    uint32_t update_time_us;
    uint64_t irq_count;
    uint32_t dropped_updates;
    uint32_t cursor_crc;
} bos_cursor_diag_t;

/* --- Cursor HAL API --- */
void               bos_cursor_init(void);
void               bos_cursor_shutdown(void);
void               bos_cursor_move(int32_t x, int32_t y);
void               bos_cursor_show(void);
void               bos_cursor_hide(void);
void               bos_cursor_set_type(bos_cursor_type_t type);
void               bos_cursor_set_custom_image(const uint32_t* argb, uint32_t w, uint32_t h, uint32_t hx, uint32_t hy);
void               bos_cursor_get_state(bos_cursor_state_t* out_state);
void               bos_cursor_get_diag(bos_cursor_diag_t* out_diag);
void               bos_cursor_dump_diagnostics(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_H */
