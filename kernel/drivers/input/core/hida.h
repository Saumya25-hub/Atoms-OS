#ifndef HIDA_H
#define HIDA_H

#include <stdint.h>
#include <stdbool.h>

#define HIDA_BACKEND_NONE 0
#define HIDA_BACKEND_VMMOUSE 120
#define HIDA_BACKEND_USB 121
#define HIDA_BACKEND_PS2 122

typedef enum {
    HIDA_STATE_DETECTING = 0,
    HIDA_STATE_ACTIVE,
    HIDA_STATE_DEGRADED,
    HIDA_STATE_FAILED,
    HIDA_STATE_FALLBACK
} HidaState;

void hida_init(void);
void hida_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint32_t max_x, uint32_t max_y, uint8_t buttons, int32_t scroll);
void hida_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll);
void hida_dump_status(void);

#endif
