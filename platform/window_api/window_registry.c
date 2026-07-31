#include "platform/include/bos_window.h"
#include "kernel/core/lib/include/string.h"

#define BOS_MAX_PLATFORM_WINDOWS 512U

typedef struct {
    BOS_WindowHandle handle;
    uint32_t         surface_id;
    uint32_t         owner_pid;
    BOS_WindowConfig config;
    bool             is_active;
    bool             is_visible;
    bool             is_focused;
} BOS_WindowEntry;

static BOS_WindowEntry g_window_registry[BOS_MAX_PLATFORM_WINDOWS];
static uint16_t g_window_generation = 1;

void BOS_WindowRegistry_Init(void) {
    memset(g_window_registry, 0, sizeof(g_window_registry));
}

BOS_WindowHandle BOS_WindowRegistry_AllocateSlot(uint32_t surface_id, uint32_t owner_pid, const BOS_WindowConfig* config) {
    for (uint32_t i = 0; i < BOS_MAX_PLATFORM_WINDOWS; i++) {
        if (!g_window_registry[i].is_active) {
            uint16_t gen = g_window_generation++;
            if (gen == 0) gen = 1;

            BOS_WindowHandle handle = (uint32_t)i | ((uint32_t)gen << 16);
            g_window_registry[i].handle = handle;
            g_window_registry[i].surface_id = surface_id;
            g_window_registry[i].owner_pid = owner_pid;
            if (config) {
                g_window_registry[i].config = *config;
            }
            g_window_registry[i].is_active = true;
            g_window_registry[i].is_visible = false;
            g_window_registry[i].is_focused = false;
            return handle;
        }
    }
    return BOS_INVALID_WINDOW_HANDLE;
}

bool BOS_WindowRegistry_Lookup(BOS_WindowHandle handle, uint32_t* out_surface_id, uint32_t* out_owner_pid) {
    if (handle == BOS_INVALID_WINDOW_HANDLE) return false;

    uint32_t slot = handle & 0xFFFFU;
    if (slot >= BOS_MAX_PLATFORM_WINDOWS) return false;

    if (g_window_registry[slot].is_active && g_window_registry[slot].handle == handle) {
        if (out_surface_id) *out_surface_id = g_window_registry[slot].surface_id;
        if (out_owner_pid) *out_owner_pid = g_window_registry[slot].owner_pid;
        return true;
    }
    return false;
}

bool BOS_WindowRegistry_FreeSlot(BOS_WindowHandle handle) {
    uint32_t slot = handle & 0xFFFFU;
    if (slot >= BOS_MAX_PLATFORM_WINDOWS) return false;

    if (g_window_registry[slot].is_active && g_window_registry[slot].handle == handle) {
        g_window_registry[slot].is_active = false;
        g_window_registry[slot].handle = BOS_INVALID_WINDOW_HANDLE;
        return true;
    }
    return false;
}
