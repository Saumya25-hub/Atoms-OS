#ifndef BOS_TERMINAL_H
#define BOS_TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/BOSurface/Core/surface.h"

// Legacy application lifecycle (for standalone testing if needed)
bwe_error_t terminal_init(uint32_t* out_win);
void terminal_exit(void);

// ConHost integrated lifecycle
bwe_error_t terminal_init_for_session(void* session_ptr, const char* title);

// Event handler hook (attached to surface->on_event)
void terminal_handle_event(uint32_t surface_id, const BVEvent* event);

// Render hook (attached to surface->on_render)
void terminal_render_hook(BWE_Surface* surface);

#endif // BOS_TERMINAL_H
