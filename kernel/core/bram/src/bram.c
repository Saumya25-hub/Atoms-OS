#include "kernel/core/bram/include/bram.h"

/*
 * ⚛️ BRAM — BOS RESOURCE AUTHORITY MANAGER V1.0 IMPLEMENTATION
 */

static bram_module_id_t g_bram_owners[BRAM_RESOURCE_COUNT];
static uint32_t         g_bram_violations_count = 0;

void bram_init(void) {
    for (int i = 0; i < BRAM_RESOURCE_COUNT; i++) {
        g_bram_owners[i] = BRAM_MODULE_NONE;
    }
    /* By default on startup, Kernel Core owns hardware until handoff */
    g_bram_owners[BRAM_RESOURCE_DISPLAY] = BRAM_MODULE_KERNEL_CORE;
    g_bram_owners[BRAM_RESOURCE_INPUT]   = BRAM_MODULE_KERNEL_CORE;
    g_bram_owners[BRAM_RESOURCE_NETWORK] = BRAM_MODULE_KERNEL_CORE;
    g_bram_owners[BRAM_RESOURCE_AUDIO]   = BRAM_MODULE_KERNEL_CORE;
    g_bram_owners[BRAM_RESOURCE_DMA]     = BRAM_MODULE_KERNEL_CORE;
    g_bram_violations_count = 0;
}

int bram_request_ownership(bram_resource_id_t resource_id, bram_module_id_t module_id) {
    if (resource_id >= BRAM_RESOURCE_COUNT) return BRAM_ERR_INVALID_RES;

    bram_module_id_t current = g_bram_owners[resource_id];
    if (current == module_id) return BRAM_SUCCESS;

    /* Kernel Core can always delegate or override ownership */
    if (current == BRAM_MODULE_NONE || current == BRAM_MODULE_KERNEL_CORE) {
        g_bram_owners[resource_id] = module_id;
        return BRAM_SUCCESS;
    }

    /* Deny unauthorized ownership transfer request */
    g_bram_violations_count++;
    return BRAM_ERR_DENIED;
}

int bram_release_ownership(bram_resource_id_t resource_id, bram_module_id_t module_id) {
    if (resource_id >= BRAM_RESOURCE_COUNT) return BRAM_ERR_INVALID_RES;

    if (g_bram_owners[resource_id] == module_id) {
        g_bram_owners[resource_id] = BRAM_MODULE_KERNEL_CORE;
        return BRAM_SUCCESS;
    }
    return BRAM_ERR_DENIED;
}

bool bram_has_ownership(bram_resource_id_t resource_id, bram_module_id_t module_id) {
    if (resource_id >= BRAM_RESOURCE_COUNT) return false;
    return (g_bram_owners[resource_id] == module_id);
}

bram_module_id_t bram_get_owner(bram_resource_id_t resource_id) {
    if (resource_id >= BRAM_RESOURCE_COUNT) return BRAM_MODULE_NONE;
    return g_bram_owners[resource_id];
}

const char* bram_get_module_name(bram_module_id_t module_id) {
    switch (module_id) {
        case BRAM_MODULE_NONE:                 return "NONE";
        case BRAM_MODULE_KERNEL_CORE:          return "KERNEL_CORE";
        case BRAM_MODULE_ROOK_ENGINE:          return "ROOK_ENGINE";
        case BRAM_MODULE_BOSURFACE_COMPOSITOR: return "BOSURFACE_COMPOSITOR";
        case BRAM_MODULE_ABDE_DIAGNOSTICS:     return "ABDE_DIAGNOSTICS";
        case BRAM_MODULE_XHCI_USB:             return "XHCI_USB";
        case BRAM_MODULE_REALTEK_LAN:          return "REALTEK_LAN";
        case BRAM_MODULE_AC97_AUDIO:           return "AC97_AUDIO";
        case BRAM_MODULE_CURSOR_CERT:          return "CURSOR_CERT";
        default:                               return "UNKNOWN";
    }
}
