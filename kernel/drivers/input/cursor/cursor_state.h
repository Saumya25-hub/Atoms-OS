#ifndef ATOMS_OS_INPUT_CURSOR_STATE_H
#define ATOMS_OS_INPUT_CURSOR_STATE_H

/**
 * @file cursor_state.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor State Singleton
 * @section PURPOSE
 * Authoritative storage for visual cursor presentation properties: coordinates, shape,
 * visibility, hotspot, scale, and backend fallback flags. Decoupled from pointer kinematics.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Cursor Shapes --- */
typedef enum {
    CURSOR_SHAPE_ARROW = 0,
    CURSOR_SHAPE_TEXT_BEAM,
    CURSOR_SHAPE_RESIZE_H,
    CURSOR_SHAPE_RESIZE_V,
    CURSOR_SHAPE_BUSY,
    CURSOR_SHAPE_WAIT,
    CURSOR_SHAPE_CROSSHAIR,
    CURSOR_SHAPE_HAND,
    CURSOR_SHAPE_MAX
} CursorShape;

/* --- Cursor Layers --- */
typedef enum {
    CURSOR_LAYER_DESKTOP = 0,
    CURSOR_LAYER_WINDOW,
    CURSOR_LAYER_SYSTEM_OVERLAY
} CursorLayer;

/* --- Cursor Scale Percentages --- */
typedef enum {
    CURSOR_SCALE_100 = 100,
    CURSOR_SCALE_125 = 125,
    CURSOR_SCALE_150 = 150,
    CURSOR_SCALE_200 = 200
} CursorScale;

/* --- Authoritative Visual State Struct --- */
typedef struct {
    int32_t screen_x;
    int32_t screen_y;
    bool visible;
    CursorShape current_shape;
    
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t width;
    uint32_t height;
    
    uint32_t scale_percent;
    CursorLayer layer;
    
    bool hw_capability;
    bool software_fallback;
    
    uint32_t current_anim_frame;
    uint32_t screen_width;
    uint32_t screen_height;
} CursorState;

extern CursorState g_cursor_state;

/* --- Lifecycle & Initialization --- */
void cursor_state_init(uint32_t screen_w, uint32_t screen_h);
void cursor_state_update_resolution(uint32_t screen_w, uint32_t screen_h);

/* --- Thread-Safe State Accessors --- */
void cursor_state_set_position(int32_t x, int32_t y);
void cursor_state_get_position(int32_t* out_x, int32_t* out_y);

void cursor_state_set_shape(CursorShape shape);
CursorShape cursor_state_get_shape(void);

void cursor_state_set_visible(bool visible);
bool cursor_state_is_visible(void);

void cursor_state_set_scale(uint32_t scale_percent);
uint32_t cursor_state_get_scale(void);

void cursor_state_set_dimensions(uint32_t w, uint32_t h, uint32_t hx, uint32_t hy);
void cursor_state_get_dimensions(uint32_t* out_w, uint32_t* out_h, uint32_t* out_hx, uint32_t* out_hy);

void cursor_state_set_backend_mode(bool hw_capable, bool sw_fallback);
bool cursor_state_is_software_fallback(void);

void cursor_state_set_anim_frame(uint32_t frame);
uint32_t cursor_state_get_anim_frame(void);

/* --- Direct Snapshot Read (For High-Frequency Rendering) --- */
void cursor_state_get_snapshot(CursorState* out_snapshot);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_STATE_H
