#ifndef KERNEL_PS2_MOUSE_H
#define KERNEL_PS2_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool controller_init;
    bool self_test_pass;
    bool port_test_pass;
    bool mouse_reset_pass;
    bool streaming_enabled;
    uint32_t ack_count;
    uint32_t ack_failures;
    uint32_t irq_count;
    uint32_t packet_count;
    uint32_t sync_errors;
} PS2MouseDiagnostics;

void ps2_mouse_init(void);
void ps2_mouse_handle_byte(uint8_t byte);

void ps2_mouse_get_diagnostics(PS2MouseDiagnostics* out_diag);

#endif // KERNEL_PS2_MOUSE_H
