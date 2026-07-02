#ifndef POINTER_DIAG_H
#define POINTER_DIAG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t raw_dx;
    int32_t raw_dy;
    int32_t filtered_dx;
    int32_t filtered_dy;
    int32_t abs_x;
    int32_t abs_y;
    int32_t prev_x;
    int32_t prev_y;
    uint64_t packet_count;
    uint32_t dropped_packets;
    uint32_t overflow_packets;
    uint32_t clamp_count;
    uint32_t movement_rate;
    bool sync_good;
} PointerDiagnostics;

void pointer_diag_init(void);
void pointer_diag_update_raw(int32_t dx, int32_t dy);
void pointer_diag_update_filtered(int32_t dx, int32_t dy);
void pointer_diag_update_abs(int32_t x, int32_t y, int32_t px, int32_t py);
void pointer_diag_inc_packet(void);
void pointer_diag_inc_drop(void);
void pointer_diag_inc_overflow(void);
void pointer_diag_inc_clamp(void);
void pointer_diag_set_sync(bool good);
const PointerDiagnostics* pointer_diag_get(void);

#endif
