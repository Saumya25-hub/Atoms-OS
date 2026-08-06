#ifndef BOS_DISPLAY_H
#define BOS_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BOS_DISPLAY_OK = 0,
    BOS_DISPLAY_ERR_INVALID_PARAM = -1,
    BOS_DISPLAY_ERR_NO_MEMORY = -2,
    BOS_DISPLAY_ERR_NOT_FOUND = -3,
    BOS_DISPLAY_ERR_NOT_SUPPORTED = -4,
    BOS_DISPLAY_ERR_ATOMIC_REJECT = -5,
    BOS_DISPLAY_ERR_HARDWARE_FAIL = -6
} bos_display_status_t;

typedef uint32_t bos_display_id_t;
typedef uint32_t bos_connector_id_t;
typedef uint32_t bos_monitor_id_t;

typedef enum {
    BOS_DISPLAY_STATE_DISCONNECTED = 0,
    BOS_DISPLAY_STATE_CONNECTED,
    BOS_DISPLAY_STATE_ACTIVE,
    BOS_DISPLAY_STATE_SUSPENDED,
    BOS_DISPLAY_STATE_ERROR
} bos_display_state_t;

typedef enum {
    BOS_MULTI_MONITOR_SINGLE = 0,
    BOS_MULTI_MONITOR_EXTEND,
    BOS_MULTI_MONITOR_CLONE,
    BOS_MULTI_MONITOR_MIRROR
} bos_multi_monitor_mode_t;

#endif /* BOS_DISPLAY_H */
