#ifndef BOS_DISPLAY_MANAGER_H
#define BOS_DISPLAY_MANAGER_H

#include "kernel/graphics/display/include/bos_display.h"
#include "kernel/graphics/display/include/bos_connector.h"
#include "kernel/graphics/display/include/bos_mode.h"
#include "kernel/graphics/display/include/bos_atomic.h"

typedef struct {
    bos_display_id_t     id;
    char                 name[64];
    bos_display_state_t  state;
    bos_connector_t      connector;
    bos_display_mode_t   active_mode;
    bos_display_mode_t   preferred_mode;
    uint32_t             supported_mode_count;
    bos_display_mode_t   supported_modes[16];
    bool                 is_primary;
    bool                 is_active;
    uint32_t             frame_count;
    uint32_t             vblank_count;
} bos_display_device_t;

/* Central Display Subsystem Management API */
bos_display_status_t bos_display_init(void);
uint32_t             bos_display_enumerate(void);
bos_display_device_t* bos_display_get_primary(void);
bos_display_device_t* bos_display_get_device(uint32_t index);
bos_display_status_t bos_display_set_mode(bos_display_id_t id, const bos_display_mode_t* mode);
bos_display_status_t bos_display_set_multi_monitor(bos_multi_monitor_mode_t mode);
void                 bos_display_dump_diagnostics(void);
void                 bos_display_run_tests(void);

#endif /* BOS_DISPLAY_MANAGER_H */
