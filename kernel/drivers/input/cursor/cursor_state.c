/**
 * @file cursor_state.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor State Singleton Implementation
 * @section PURPOSE
 * Implements thread-safe, lock-free/spinlock-protected storage for cursor visual state.
 * Guaranteed zero dynamic memory allocation and O(1) state transitions.
 */

#include "cursor_state.h"
#include <stddef.h>
#include "kernel/drivers/display/display.h"

/* Static singleton instance in kernel BSS segment */
CursorState g_cursor_state;

/* --- Internal Lock Helpers (Preserve Interrupt Flag) --- */
static inline uint64_t cursor_state_lock(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void cursor_state_unlock(uint64_t flags) {
    __asm__ volatile("push %0; popfq" : : "r"(flags) : "memory", "cc");
}

void cursor_state_init(uint32_t screen_w, uint32_t screen_h) {
    uint64_t flags = cursor_state_lock();
    g_cursor_state.screen_x = (int32_t)(screen_w / 2);
    g_cursor_state.screen_y = (int32_t)(screen_h / 2);
    g_cursor_state.visible = true;
    g_cursor_state.current_shape = CURSOR_SHAPE_ARROW;
    g_cursor_state.hotspot_x = 0;
    g_cursor_state.hotspot_y = 0;
    g_cursor_state.width = 32;
    g_cursor_state.height = 32;
    g_cursor_state.scale_percent = CURSOR_SCALE_100;
    g_cursor_state.layer = CURSOR_LAYER_DESKTOP;
    g_cursor_state.hw_capability = false;
    g_cursor_state.software_fallback = true;
    g_cursor_state.current_anim_frame = 0;
    g_cursor_state.screen_width = screen_w;
    g_cursor_state.screen_height = screen_h;
    cursor_state_unlock(flags);
}

void cursor_state_update_resolution(uint32_t screen_w, uint32_t screen_h) {
    uint64_t flags = cursor_state_lock();
    g_cursor_state.screen_width = screen_w;
    g_cursor_state.screen_height = screen_h;
    if (g_cursor_state.screen_x >= (int32_t)screen_w) g_cursor_state.screen_x = (int32_t)(screen_w - 1);
    if (g_cursor_state.screen_y >= (int32_t)screen_h) g_cursor_state.screen_y = (int32_t)(screen_h - 1);
    if (g_cursor_state.screen_x < 0) g_cursor_state.screen_x = 0;
    if (g_cursor_state.screen_y < 0) g_cursor_state.screen_y = 0;
    cursor_state_unlock(flags);
}

volatile uint64_t g_cursor_state_calls_count = 0;

void cursor_state_set_position(int32_t x, int32_t y) {
    g_cursor_state_calls_count++;
    uint64_t flags = cursor_state_lock();
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int32_t)g_cursor_state.screen_width && g_cursor_state.screen_width > 0) {
        x = (int32_t)(g_cursor_state.screen_width - 1);
    }
    if (y >= (int32_t)g_cursor_state.screen_height && g_cursor_state.screen_height > 0) {
        y = (int32_t)(g_cursor_state.screen_height - 1);
    }
    g_cursor_state.screen_x = x;
    g_cursor_state.screen_y = y;
    cursor_state_unlock(flags);
}

void cursor_state_get_position(int32_t* out_x, int32_t* out_y) {
    uint64_t flags = cursor_state_lock();
    if (out_x) *out_x = g_cursor_state.screen_x;
    if (out_y) *out_y = g_cursor_state.screen_y;
    cursor_state_unlock(flags);
}

void cursor_state_set_shape(CursorShape shape) {
    if (shape >= CURSOR_SHAPE_MAX) shape = CURSOR_SHAPE_ARROW;
    uint64_t flags = cursor_state_lock();
    if (g_cursor_state.current_shape != shape) {
        g_cursor_state.current_shape = shape;
        g_cursor_state.current_anim_frame = 0; /* Reset animation on shape change */
    }
    cursor_state_unlock(flags);
}

CursorShape cursor_state_get_shape(void) {
    uint64_t flags = cursor_state_lock();
    CursorShape shape = g_cursor_state.current_shape;
    cursor_state_unlock(flags);
    return shape;
}

void cursor_state_set_visible(bool visible) {
    uint64_t flags = cursor_state_lock();
    g_cursor_state.visible = visible;
    cursor_state_unlock(flags);
}

bool cursor_state_is_visible(void) {
    uint64_t flags = cursor_state_lock();
    bool vis = g_cursor_state.visible;
    cursor_state_unlock(flags);
    return vis;
}

void cursor_state_set_scale(uint32_t scale_percent) {
    if (scale_percent < 100) scale_percent = 100;
    if (scale_percent > 400) scale_percent = 400;
    uint64_t flags = cursor_state_lock();
    g_cursor_state.scale_percent = scale_percent;
    cursor_state_unlock(flags);
}

uint32_t cursor_state_get_scale(void) {
    uint64_t flags = cursor_state_lock();
    uint32_t scale = g_cursor_state.scale_percent;
    cursor_state_unlock(flags);
    return scale;
}

void cursor_state_set_dimensions(uint32_t w, uint32_t h, uint32_t hx, uint32_t hy) {
    uint64_t flags = cursor_state_lock();
    g_cursor_state.width = w;
    g_cursor_state.height = h;
    g_cursor_state.hotspot_x = hx;
    g_cursor_state.hotspot_y = hy;
    cursor_state_unlock(flags);
}

void cursor_state_get_dimensions(uint32_t* out_w, uint32_t* out_h, uint32_t* out_hx, uint32_t* out_hy) {
    uint64_t flags = cursor_state_lock();
    if (out_w) *out_w = g_cursor_state.width;
    if (out_h) *out_h = g_cursor_state.height;
    if (out_hx) *out_hx = g_cursor_state.hotspot_x;
    if (out_hy) *out_hy = g_cursor_state.hotspot_y;
    cursor_state_unlock(flags);
}

void cursor_state_set_backend_mode(bool hw_capable, bool sw_fallback) {
    uint64_t flags = cursor_state_lock();
    g_cursor_state.hw_capability = hw_capable;
    g_cursor_state.software_fallback = sw_fallback;
    cursor_state_unlock(flags);
}

bool cursor_state_is_software_fallback(void) {
    uint64_t flags = cursor_state_lock();
    bool sw = g_cursor_state.software_fallback;
    cursor_state_unlock(flags);
    return sw;
}

void cursor_state_set_anim_frame(uint32_t frame) {
    uint64_t flags = cursor_state_lock();
    g_cursor_state.current_anim_frame = frame;
    cursor_state_unlock(flags);
}

uint32_t cursor_state_get_anim_frame(void) {
    uint64_t flags = cursor_state_lock();
    uint32_t f = g_cursor_state.current_anim_frame;
    cursor_state_unlock(flags);
    return f;
}

void cursor_state_get_snapshot(CursorState* out_snapshot) {
    if (!out_snapshot) return;
    uint64_t flags = cursor_state_lock();
    *out_snapshot = g_cursor_state;
    cursor_state_unlock(flags);
}
