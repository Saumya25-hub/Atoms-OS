#ifndef BRAM_H
#define BRAM_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ⚛️ BRAM — BOS RESOURCE AUTHORITY MANAGER V1.0
 * Kernel-Level Hardware Resource Ownership & Access Arbitration Registry.
 * Guarantees Single-Owner Authority across Display, Audio, Input, Network, and DMA.
 */

#define BRAM_SUCCESS              0
#define BRAM_ERR_DENIED          -1
#define BRAM_ERR_INVALID_RES     -2
#define BRAM_ERR_ALREADY_OWNED   -3

/* Resource Identifier Enum */
typedef enum {
    BRAM_RESOURCE_DISPLAY = 0,
    BRAM_RESOURCE_AUDIO,
    BRAM_RESOURCE_INPUT,
    BRAM_RESOURCE_NETWORK,
    BRAM_RESOURCE_DMA,
    BRAM_RESOURCE_COUNT
} bram_resource_id_t;

/* Subsystem Module Identifier Enum */
typedef enum {
    BRAM_MODULE_NONE = 0,
    BRAM_MODULE_KERNEL_CORE,
    BRAM_MODULE_ROOK_ENGINE,
    BRAM_MODULE_BOSURFACE_COMPOSITOR,
    BRAM_MODULE_ABDE_DIAGNOSTICS,
    BRAM_MODULE_XHCI_USB,
    BRAM_MODULE_REALTEK_LAN,
    BRAM_MODULE_AC97_AUDIO,
    BRAM_MODULE_CURSOR_CERT
} bram_module_id_t;

/* Core Authority APIs */
void        bram_init(void);
int         bram_request_ownership(bram_resource_id_t resource_id, bram_module_id_t module_id);
int         bram_release_ownership(bram_resource_id_t resource_id, bram_module_id_t module_id);
bool        bram_has_ownership(bram_resource_id_t resource_id, bram_module_id_t module_id);
bram_module_id_t bram_get_owner(bram_resource_id_t resource_id);
const char* bram_get_module_name(bram_module_id_t module_id);

#endif /* BRAM_H */
