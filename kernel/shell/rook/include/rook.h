#ifndef ROOK_H
#define ROOK_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/shell/rook/include/rook_pages.h"

/*
 * ♜ ROOK ENGINE V1.0 — Core Page Navigation & Screen Management Engine
 * Inspired by the Chess Rook (♜). Deterministic straight-line navigation.
 */

#define ROOK_MAX_DIRTY_RECTS 16

/* Lifecycle Stage Enum */
typedef enum {
    ROOK_STATE_UNALLOCATED = 0,
    ROOK_STATE_CREATED,
    ROOK_STATE_INITIALIZED,
    ROOK_STATE_LOADED,
    ROOK_STATE_ACTIVE,
    ROOK_STATE_PAUSED
} rook_page_state_t;

/* Navigation Command Types */
typedef enum {
    ROOK_NAV_LEFT = 0,
    ROOK_NAV_RIGHT,
    ROOK_NAV_UP,
    ROOK_NAV_DOWN,
    ROOK_NAV_NEXT,
    ROOK_NAV_PREVIOUS,
    ROOK_NAV_GOTO
} rook_nav_cmd_t;

/* Dirty Rectangle for Optimization */
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} rook_dirty_rect_t;

struct rook_page;

/* 11-Stage Lifecycle Function Pointer Table */
typedef struct {
    int (*on_create)(struct rook_page* page);
    int (*on_init)(struct rook_page* page);
    int (*on_load)(struct rook_page* page);
    int (*on_enter)(struct rook_page* page);
    int (*on_update)(struct rook_page* page, uint64_t delta_ms);
    int (*on_render)(struct rook_page* page, uint32_t* framebuffer, uint32_t stride);
    int (*on_pause)(struct rook_page* page);
    int (*on_resume)(struct rook_page* page);
    int (*on_exit)(struct rook_page* page);
    int (*on_unload)(struct rook_page* page);
    int (*on_destroy)(struct rook_page* page);
} rook_lifecycle_ops_t;

/* Page Descriptor Structure */
typedef struct rook_page {
    uint16_t id;                        /* Permanent numeric ID */
    const char* name;                   /* Debug display name */
    rook_page_state_t state;            /* Current lifecycle state */
    rook_lifecycle_ops_t ops;           /* Function pointer table */
    
    uint16_t nav_left_id;               /* Rook navigation: Left neighbor ID */
    uint16_t nav_right_id;              /* Rook navigation: Right neighbor ID */
    uint16_t nav_up_id;                 /* Rook navigation: Up neighbor ID */
    uint16_t nav_down_id;               /* Rook navigation: Down neighbor ID */
    uint16_t nav_next_id;               /* Rook navigation: Next neighbor ID */
    uint16_t nav_prev_id;               /* Rook navigation: Prev neighbor ID */
    
    uint64_t render_time_ns;            /* Telemetry: last render duration */
    uint64_t update_time_ns;            /* Telemetry: last update duration */
    uint32_t allocated_bytes;           /* Telemetry: asset memory usage */
    void* page_private_data;            /* Page-specific static state struct */
} rook_page_t;

/* Engine Core State API */
void        rook_init(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride);
int         rook_register_page(rook_page_t* page);
int         rook_goto(uint16_t page_id);
int         rook_next(void);
int         rook_previous(void);
int         rook_navigate(rook_nav_cmd_t cmd);
void        rook_update(uint64_t delta_ms);
void        rook_render(void);
rook_page_t* rook_get_current_page(void);
rook_page_t* rook_get_page(uint16_t page_id);

/* Rendering & Dirty Rects API */
void        rook_invalidate_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void        rook_invalidate_full(void);
uint32_t*   rook_get_backbuffer(void);
uint32_t    rook_get_width(void);
uint32_t    rook_get_height(void);
uint32_t    rook_get_stride(void);

/* Event Dispatcher API */
void        rook_dispatch_event(uint32_t event_id, void* payload);

#endif /* ROOK_H */
