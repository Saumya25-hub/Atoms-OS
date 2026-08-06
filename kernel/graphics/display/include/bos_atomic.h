#ifndef BOS_ATOMIC_H
#define BOS_ATOMIC_H

#include "kernel/graphics/display/include/bos_display.h"
#include "kernel/graphics/display/include/bos_connector.h"
#include "kernel/graphics/display/include/bos_mode.h"

typedef struct {
    bos_display_id_t   display_id;
    bos_display_state_t state;
    bos_display_mode_t mode;
    uint32_t           crtc_id;
    uint32_t           plane_id;
    uint64_t           framebuffer_phys;
    uint32_t           cursor_x;
    uint32_t           cursor_y;
    bool               cursor_visible;
    bool               dirty;
} bos_display_state_snapshot_t;

typedef struct {
    uint32_t                     num_snapshots;
    bos_display_state_snapshot_t snapshots[8];
    bool                         checked;
    bool                         committed;
} bos_atomic_state_t;

bos_display_status_t bos_atomic_state_init(bos_atomic_state_t* state);
bos_display_status_t bos_atomic_state_add(bos_atomic_state_t* state, const bos_display_state_snapshot_t* snapshot);
bos_display_status_t bos_atomic_check(bos_atomic_state_t* state);
bos_display_status_t bos_atomic_commit(bos_atomic_state_t* state);
bos_display_status_t bos_atomic_rollback(bos_atomic_state_t* state);

#endif /* BOS_ATOMIC_H */
