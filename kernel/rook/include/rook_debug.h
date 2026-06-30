#ifndef ROOK_DEBUG_H
#define ROOK_DEBUG_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ♜ ROOK ENGINE V1.0 — Debug & Diagnostics HUD
 */

void rook_toggle_debug_overlay(bool enable);
bool rook_is_debug_overlay_enabled(void);
void rook_debug_render_overlay(uint32_t* framebuffer, uint32_t width, uint32_t height, uint32_t stride);
void rook_debug_log_event(const char* event_msg);

#endif /* ROOK_DEBUG_H */
