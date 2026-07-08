#ifndef ATOMS_OS_INPUT_CURSOR_BACKEND_H
#define ATOMS_OS_INPUT_CURSOR_BACKEND_H

/**
 * @file cursor_backend.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Backend Abstraction Layer
 * @section PURPOSE
 * Abstracts hardware (BSPE Cursor Plane / Bochs VGA / VBE registers) and software cursor
 * rendering backends behind an identical API. Seamlesly activates fallback if hardware fails.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CURSOR_BACKEND_NONE = 0,
    CURSOR_BACKEND_HARDWARE = 1,
    CURSOR_BACKEND_SOFTWARE = 2
} CursorBackendType;

/* --- Lifecycle & Initialization --- */
void cursor_backend_init(void);
void cursor_backend_shutdown(void);

/* --- Active Backend Probing --- */
CursorBackendType cursor_backend_get_type(void);
bool cursor_backend_is_hardware(void);

/* --- Unified Backend Control APIs --- */
void cursor_backend_set_position(int32_t x, int32_t y);
void cursor_backend_set_image(const uint32_t* argb_bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y);
void cursor_backend_set_visibility(bool visible);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_BACKEND_H
