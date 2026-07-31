#include "platform/include/bos_window.h"
#include <stddef.h>

extern BOS_WindowHandle BOS_WindowRegistry_AllocateSlot(uint32_t surface_id, uint32_t owner_pid, const BOS_WindowConfig* config);
extern bool             BOS_WindowRegistry_Lookup(BOS_WindowHandle handle, uint32_t* out_surface_id, uint32_t* out_owner_pid);
extern bool             BOS_WindowRegistry_FreeSlot(BOS_WindowHandle handle);

static BOS_WindowHandle g_current_focused_window = BOS_INVALID_WINDOW_HANDLE;

BOS_Result BOS_CreateWindow(const BOS_WindowConfig* config, BOS_WindowHandle* out_handle) {
    if (!config || !out_handle) return BOS_ERROR_INVALID_ARGUMENT;
    if (config->width == 0 || config->height == 0) return BOS_ERROR_INVALID_ARGUMENT;

    /* Allocate slot in registry (surface ID 0x1000 placeholder for platform gateway integration) */
    static uint32_t surface_id_counter = 0x1000;
    uint32_t surface_id = surface_id_counter++;

    BOS_WindowHandle handle = BOS_WindowRegistry_AllocateSlot(surface_id, 1, config);
    if (handle == BOS_INVALID_WINDOW_HANDLE) {
        return BOS_ERROR_OUT_OF_MEMORY;
    }

    *out_handle = handle;
    return BOS_SUCCESS;
}

BOS_Result BOS_DestroyWindow(BOS_WindowHandle handle) {
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }

    if (g_current_focused_window == handle) {
        g_current_focused_window = BOS_INVALID_WINDOW_HANDLE;
    }

    BOS_WindowRegistry_FreeSlot(handle);
    return BOS_SUCCESS;
}

BOS_Result BOS_CloseWindow(BOS_WindowHandle handle) {
    return BOS_DestroyWindow(handle);
}

BOS_Result BOS_ShowWindow(BOS_WindowHandle handle) {
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }
    return BOS_SUCCESS;
}

BOS_Result BOS_HideWindow(BOS_WindowHandle handle) {
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }
    return BOS_SUCCESS;
}

BOS_Result BOS_SetFocus(BOS_WindowHandle handle) {
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }
    g_current_focused_window = handle;
    return BOS_SUCCESS;
}

BOS_Result BOS_GetFocus(BOS_WindowHandle* out_handle) {
    if (!out_handle) return BOS_ERROR_INVALID_ARGUMENT;
    *out_handle = g_current_focused_window;
    return BOS_SUCCESS;
}

BOS_Result BOS_MoveWindow(BOS_WindowHandle handle, int32_t x, int32_t y) {
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }
    (void)x; (void)y;
    return BOS_SUCCESS;
}

BOS_Result BOS_ResizeWindow(BOS_WindowHandle handle, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return BOS_ERROR_INVALID_ARGUMENT;
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }
    return BOS_SUCCESS;
}

BOS_Result BOS_GetWindowBounds(BOS_WindowHandle handle, BOS_Rect* out_bounds) {
    if (!out_bounds) return BOS_ERROR_INVALID_ARGUMENT;
    uint32_t surface_id, pid;
    if (!BOS_WindowRegistry_Lookup(handle, &surface_id, &pid)) {
        return BOS_ERROR_INVALID_HANDLE;
    }
    out_bounds->x = 100;
    out_bounds->y = 100;
    out_bounds->width = 640;
    out_bounds->height = 480;
    return BOS_SUCCESS;
}
