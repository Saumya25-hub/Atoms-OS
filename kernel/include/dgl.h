#ifndef DGL_H
#define DGL_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/bram/include/bram.h"

/*
 * ⚛️ DGL — DISPLAY GOVERNANCE LAYER V1.0
 * Exclusive Framebuffer Protection, Dynamic Stride Geometry, & Display Owner Governance.
 */

#define DGL_SUCCESS             0
#define DGL_ERR_DENIED         -1
#define DGL_ERR_INVALID_PARAM  -2

typedef enum {
    DGL_STATE_BOOT = 0,
    DGL_STATE_LOGIN,
    DGL_STATE_DESKTOP,
    DGL_STATE_RECOVERY
} dgl_display_state_t;

/* Geometry Info */
typedef struct {
    uint32_t phys_width;
    uint32_t phys_height;
    uint32_t pitch_bytes;
    uint32_t stride_pixels;
    uint32_t bytes_per_pixel;
    bool     quiet_boot_enabled;
} dgl_geometry_t;

void           dgl_init(uint32_t phys_w, uint32_t phys_h, uint32_t pitch_bytes);
int            dgl_set_state(dgl_display_state_t state);
dgl_display_state_t dgl_get_state(void);
const dgl_geometry_t* dgl_get_geometry(void);
bool           dgl_can_draw(bram_module_id_t module_id);
void           dgl_set_quiet_boot(bool quiet);
bool           dgl_is_quiet_boot(void);

#endif /* DGL_H */
