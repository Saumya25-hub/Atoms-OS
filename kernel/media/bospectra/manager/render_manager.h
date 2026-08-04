/*
 * BOSPECTRA V3 — Render Manager
 * kernel/media/bospectra/manager/render_manager.h
 *
 * Surface creation, renderer selection, present, resize, texture allocation.
 */

#ifndef BOSPECTRA_RENDER_MANAGER_H
#define BOSPECTRA_RENDER_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../render/include/bospectra_render.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t target_window_id;
    uint32_t width;
    uint32_t height;
    bospectra_render_session_id_t render_session_id;
    bool is_active;
} BOSPECTRA_RenderSurface;

void bospectra_render_manager_init(void);
void bospectra_render_manager_shutdown(void);

bospectra_error_t bospectra_render_create_target(uint32_t window_id, uint32_t width, uint32_t height, bospectra_render_session_id_t* out_render_id);
bospectra_error_t bospectra_render_present(bospectra_render_session_id_t render_id, const BOSFrame* frame, int32_t x, int32_t y, int32_t w, int32_t h);
bospectra_error_t bospectra_render_destroy_target(bospectra_render_session_id_t render_id);

#endif /* BOSPECTRA_RENDER_MANAGER_H */
