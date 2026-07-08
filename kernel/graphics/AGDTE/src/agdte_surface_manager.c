/**
 * @file agdte_surface_manager.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Surface Manager
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Manages the AGDTE Surface Train across all architectural layers:
 * Desktop, Windows, Cursor, Popup, Overlay, Notification, and prepares
 * slots for future Video, Hardware Cursor, and HDR planes without rendering.
 */

#include "../include/agdte.h"

/* --- Static Storage for Managed Surfaces --- */
static AGDTE_SurfaceDescriptor s_surfaces[AGDTE_MAX_MANAGED_SURFACES];
static uint32_t s_registered_surface_count = 0;

void agdte_surface_reset_all(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_MANAGED_SURFACES; i++) {
        s_surfaces[i].surface_id = i;
        s_surfaces[i].display_id = 0;
        s_surfaces[i].layer = AGDTE_LAYER_DESKTOP;
        s_surfaces[i].pos_x = 0;
        s_surfaces[i].pos_y = 0;
        s_surfaces[i].width = 0;
        s_surfaces[i].height = 0;
        s_surfaces[i].current_buffer_id = 0xFFFFFFFF;
        s_surfaces[i].visible = false;
        s_surfaces[i].opaque = true;
        s_surfaces[i].z_index = 0;
        for (uint32_t n = 0; n < 32; n++) {
            s_surfaces[i].name[n] = '\0';
        }
    }
    s_registered_surface_count = 0;
}

AGDTE_Error AGDTE_Surface_Register(uint32_t display_id, AGDTE_SurfaceLayer layer, uint32_t width, uint32_t height, const char* name, uint32_t* out_surface_id) {
    if (!out_surface_id) {
        return AGDTE_ERR_NULL_POINTER;
    }
    if (width == 0 || height == 0 || layer >= AGDTE_LAYER_COUNT) {
        return AGDTE_ERR_INVALID_STATE;
    }
    if (s_registered_surface_count >= AGDTE_MAX_MANAGED_SURFACES) {
        return AGDTE_ERR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < AGDTE_MAX_MANAGED_SURFACES; i++) {
        if (!s_surfaces[i].visible && s_surfaces[i].width == 0) {
            s_surfaces[i].surface_id = i;
            s_surfaces[i].display_id = display_id;
            s_surfaces[i].layer = layer;
            s_surfaces[i].pos_x = 0;
            s_surfaces[i].pos_y = 0;
            s_surfaces[i].width = width;
            s_surfaces[i].height = height;
            s_surfaces[i].current_buffer_id = 0xFFFFFFFF;
            s_surfaces[i].visible = true;
            s_surfaces[i].opaque = true;
            s_surfaces[i].z_index = (uint32_t)layer * 1000 + i;

            if (name) {
                for (uint32_t n = 0; n < 31 && name[n] != '\0'; n++) {
                    s_surfaces[i].name[n] = name[n];
                }
            } else {
                s_surfaces[i].name[0] = 'S';
                s_surfaces[i].name[1] = 'R';
                s_surfaces[i].name[2] = 'F';
                s_surfaces[i].name[3] = '\0';
            }

            *out_surface_id = i;
            s_registered_surface_count++;
            return AGDTE_OK;
        }
    }

    return AGDTE_ERR_OUT_OF_MEMORY;
}

AGDTE_Error AGDTE_Surface_Unregister(uint32_t surface_id) {
    if (surface_id >= AGDTE_MAX_MANAGED_SURFACES || s_surfaces[surface_id].width == 0) {
        return AGDTE_ERR_INVALID_SURFACE;
    }

    s_surfaces[surface_id].visible = false;
    s_surfaces[surface_id].width = 0;
    s_surfaces[surface_id].height = 0;
    s_surfaces[surface_id].current_buffer_id = 0xFFFFFFFF;
    s_surfaces[surface_id].name[0] = '\0';
    if (s_registered_surface_count > 0) {
        s_registered_surface_count--;
    }
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Surface_SetPosition(uint32_t surface_id, int32_t x, int32_t y) {
    if (surface_id >= AGDTE_MAX_MANAGED_SURFACES || s_surfaces[surface_id].width == 0) {
        return AGDTE_ERR_INVALID_SURFACE;
    }
    s_surfaces[surface_id].pos_x = x;
    s_surfaces[surface_id].pos_y = y;
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Surface_AssignBuffer(uint32_t surface_id, uint32_t buffer_id) {
    if (surface_id >= AGDTE_MAX_MANAGED_SURFACES || s_surfaces[surface_id].width == 0) {
        return AGDTE_ERR_INVALID_SURFACE;
    }
    if (buffer_id != 0xFFFFFFFF) {
        AGDTE_BufferDescriptor* buf = AGDTE_Buffer_GetDescriptor(buffer_id);
        if (!buf) {
            return AGDTE_ERR_INVALID_BUFFER;
        }
        buf->assigned_surface_id = surface_id;
        buf->owner = AGDTE_BUFFER_OWNER_AGDTE_QUEUE;
    }
    s_surfaces[surface_id].current_buffer_id = buffer_id;
    return AGDTE_OK;
}

AGDTE_SurfaceDescriptor* AGDTE_Surface_GetDescriptor(uint32_t surface_id) {
    if (surface_id >= AGDTE_MAX_MANAGED_SURFACES || s_surfaces[surface_id].width == 0) {
        return (AGDTE_SurfaceDescriptor*)0;
    }
    return &s_surfaces[surface_id];
}

AGDTE_Error AGDTE_Surface_GetLayerSurfaceID(AGDTE_SurfaceLayer layer, uint32_t* out_id) {
    if (!out_id || layer >= AGDTE_LAYER_COUNT) {
        return AGDTE_ERR_INVALID_STATE;
    }
    for (uint32_t i = 0; i < AGDTE_MAX_MANAGED_SURFACES; i++) {
        if (s_surfaces[i].width > 0 && s_surfaces[i].layer == layer) {
            *out_id = i;
            return AGDTE_OK;
        }
    }
    return AGDTE_ERR_INVALID_SURFACE;
}

uint32_t AGDTE_Surface_GetActiveCount(void) {
    return s_registered_surface_count;
}
