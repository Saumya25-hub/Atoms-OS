#ifndef KEYBOARD_LED_DEBUG_H
#define KEYBOARD_LED_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "drivers/input/ps2/ps2.h"

typedef struct {
    // Hardware info
    uint16_t status_port;
    uint16_t data_port;
    uint8_t  irq_number;
    bool     controller_ready;

    // Logical lock states
    bool     caps_lock;
    bool     num_lock;
    bool     scroll_lock;
    uint8_t  current_led_mask;

    // Transaction metrics
    uint32_t commands_sent;
    uint32_t acks_received;
    uint32_t resends_received;
    uint32_t timeouts_occurred;
    uint8_t  last_command;
    uint8_t  last_mask_sent;
    uint8_t  last_response_byte;
    const char *last_transaction_state;

    // Stress test metrics
    struct {
        bool caps_cycle_pass;
        bool num_cycle_pass;
        bool combined_cycle_pass;
        bool stress_100_pass;
        uint32_t completed_cycles;
    } stress;

    // Raw Hardware Trace & Diagnostic
    PS2RawTrace raw_trace;
    PS2ControllerDiag controller_diag;

    // Diagnostic verdicts
    const char *ack_verdict_str;
    const char *led_verdict_str;
    const char *queue_verdict_str;
    const char *final_verdict_str;
} KeyboardLEDDebugStats;

void keyboard_led_debug_init(boot_info_t *boot_info);
void keyboard_led_debug_render(void);
void keyboard_led_debug_run(boot_info_t *boot_info);

#endif /* KEYBOARD_LED_DEBUG_H */
