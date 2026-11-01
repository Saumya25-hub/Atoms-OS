#ifndef ATOMS_MOUSE_DEBUG_H
#define ATOMS_MOUSE_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

#define MOUSE_DIAG_RING_SIZE 12

typedef struct {
    uint64_t timestamp;
    uint8_t slot;
    uint8_t port;
    uint8_t iface;
    uint8_t protocol;
    uint8_t dev_protocol;
    uint32_t length;
    uint8_t data[8];
    bool is_keyboard;
    int16_t dx;
    int16_t dy;
    int8_t wheel;
    uint8_t buttons;
} MouseDiagRecord;

typedef struct {
    uint64_t total_packets;
    uint64_t mouse_packets;
    uint64_t keyboard_packets;
    int32_t accum_x;
    int32_t accum_y;
    uint8_t cur_buttons;
    uint32_t head;
    MouseDiagRecord records[MOUSE_DIAG_RING_SIZE];
} MouseDiagTelemetry;

extern volatile MouseDiagTelemetry g_mouse_diag_telemetry;

void mouse_debug_record_packet(uint8_t slot, uint8_t port, uint8_t iface, uint8_t protocol,
                               uint8_t dev_protocol, uint32_t length, const uint8_t* report,
                               bool is_keyboard);

void mouse_debug_render_frame(void);

#endif /* ATOMS_MOUSE_DEBUG_H */

