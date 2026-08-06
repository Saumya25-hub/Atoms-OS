/**
 * @file bos_cursor_diag.h
 * @brief Forensic Diagnostics Engine for BCE V1.0
 */

#ifndef BOS_CURSOR_DIAG_H
#define BOS_CURSOR_DIAG_H

#include "bos_cursor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     current_theme[64];
    uint32_t active_type;
    uint32_t current_frame;
    bool     is_animating;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    bool     is_hardware_overlay;
    char     gpu_driver_name[64];
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t decode_time_us;
    uint32_t upload_time_us;
    uint32_t memory_used_bytes;
} bce_diag_report_t;

void bos_cursor_diag_init(void);
void bos_cursor_diag_log_decode(uint32_t decode_us);
void bos_cursor_diag_log_upload(uint32_t upload_us);
void bos_cursor_diag_get_report(bce_diag_report_t* out_report);
void bos_cursor_diag_print_autopsy(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_DIAG_H */
