#ifndef ATOMS_APP_WINDOW_API_H
#define ATOMS_APP_WINDOW_API_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Application Window API
// ============================================================

bwe_error_t ATOMS_CreateWindowForApp(uint32_t app_id, int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_win_id);
bwe_error_t ATOMS_CloseWindowForApp(uint32_t app_id, uint32_t win_id);
bwe_error_t ATOMS_SetWindowBounds(uint32_t win_id, int32_t x, int32_t y, int32_t width, int32_t height);
bwe_error_t ATOMS_SetWindowTitle(uint32_t win_id, const char* title);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_WINDOW_API_H
