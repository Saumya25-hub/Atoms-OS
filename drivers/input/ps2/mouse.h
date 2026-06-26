#ifndef KERNEL_PS2_MOUSE_H
#define KERNEL_PS2_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t irq_count;
    uint32_t packet_count;
    uint32_t sync_errors;
    uint32_t ack_failures;
} PS2MouseDiagnostics;

void ps2_mouse_init(void);

void ps2_mouse_get_diagnostics(PS2MouseDiagnostics* out_diag);

#endif // KERNEL_PS2_MOUSE_H
