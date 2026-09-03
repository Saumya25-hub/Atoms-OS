#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "kernel/drivers/keyboard/include/keyboard.h"

#define PS2_RESP_ACK     0
#define PS2_RESP_RESEND  1
#define PS2_RESP_ERROR   2
#define PS2_RESP_TIMEOUT 3

typedef struct {
    uint8_t status_before_drain;
    uint8_t drained_count;
    uint8_t drained_bytes[8];
    uint8_t status_after_drain;
    
    // Command 0xED Phase
    uint8_t status_before_ed;
    uint8_t status_after_ed_write;
    bool    tx_ed_success;
    uint8_t status_while_waiting1;
    bool    aux_mouse_encountered1;
    uint8_t last_mouse_byte1;
    bool    rx_byte1_valid;
    uint8_t rx_byte1;
    bool    obf_bit1;
    bool    aux_bit1;
    bool    ack1_is_fa;
    bool    timeout1;
    
    // Mask Phase
    uint8_t status_before_mask;
    uint8_t status_after_mask_write;
    uint8_t mask_sent;
    uint8_t status_while_waiting2;
    bool    aux_mouse_encountered2;
    uint8_t last_mouse_byte2;
    bool    rx_byte2_valid;
    uint8_t rx_byte2;
    bool    obf_bit2;
    bool    aux_bit2;
    bool    ack2_is_fa;
    bool    timeout2;
    uint8_t resends_observed;
    
    bool    final_transaction_pass;
} PS2RawTrace;

typedef struct {
    uint8_t  controller_config_byte;
    bool     translation_enabled;
    bool     kbd_clock_enabled;
    bool     mouse_clock_enabled;
    bool     irq1_enabled;
    bool     irq12_enabled;
    
    // Command 0xEE (Echo) Test
    uint8_t  echo_status_before;
    bool     echo_rx_valid;
    uint8_t  echo_rx_byte;
    bool     echo_received;
    bool     echo_timeout;
    
    // Command 0xF4 (Enable Scanning) Test
    uint8_t  f4_status_before;
    bool     f4_rx_valid;
    uint8_t  f4_rx_byte;
    bool     f4_ack_received;
    bool     f4_timeout;
    
    // Command 0xED (Set LEDs) Single Test
    PS2RawTrace ed_trace;
} PS2ControllerDiag;

// Public driver instance for the PS/2 Keyboard
extern KeyboardDriver ps2_keyboard_driver;

// Hardware PS/2 LED control
bool ps2_keyboard_set_leds(uint8_t led_mask);
bool ps2_keyboard_trace_transaction(uint8_t led_mask, PS2RawTrace *trace);
void ps2_run_controller_diag(PS2ControllerDiag *diag);
