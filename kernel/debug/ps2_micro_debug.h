#ifndef PS2_MICRO_DEBUG_H
#define PS2_MICRO_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

// 8042 Status Register (0x64) Decoded Snapshot
typedef struct {
    uint8_t  raw_status;
    bool     obf;       // Bit 0: Output Buffer Full
    bool     ibf;       // Bit 1: Input Buffer Full
    bool     system;    // Bit 2: System Flag (POST passed)
    bool     cmd_data;  // Bit 3: Command/Data (0=data, 1=cmd)
    bool     kbd_lock;  // Bit 4: Keyboard Lock (1=uninhibited)
    bool     aux;       // Bit 5: AUX Data (0=kbd, 1=mouse)
    bool     timeout;   // Bit 6: Timeout Error
    bool     parity;    // Bit 7: Parity Error
    uint64_t tsc;
    uint64_t time_us;
} PS2StatusSnapshot;

// Microsecond Event Log Item
typedef struct {
    uint64_t   time_us;
    uint8_t    status;
    uint8_t    byte;
    const char *event_name;
    bool       is_mouse;
} PS2MicroEvent;

// Controller Self-Test (0xAA to 0x64)
typedef struct {
    uint8_t  cmd;
    uint8_t  result_byte;
    bool     passed;          // 0x55
    uint8_t  status_after;
    bool     timeout_cleared; // Bit 6 cleared
    uint32_t latency_us;
} PS2SelfTestResult;

// Keyboard Interface Test (0xAB to 0x64)
typedef struct {
    uint8_t  cmd;
    uint8_t  result_byte;     // 0x00 = OK, 0x01=clk low, 0x02=clk hi, 0x03=data low, 0x04=data hi
    const char *result_str;
    bool     passed;
    uint32_t latency_us;
} PS2InterfaceTestResult;

// Keyboard Reset & BAT Explicit State Machine
typedef enum {
    KBD_STATE_IDLE = 0,
    KBD_STATE_DRAIN = 1,
    KBD_STATE_RESET_SENT = 2,
    KBD_STATE_WAIT_ACK = 3,
    KBD_STATE_ACK_RECEIVED = 4,
    KBD_STATE_WAIT_BAT = 5,
    KBD_STATE_BAT_RECEIVED = 6,
    KBD_STATE_BAT_FAILED = 7,
    KBD_STATE_RESEND = 8,
    KBD_STATE_TIMEOUT = 9,
    KBD_STATE_READY = 10
} KbdResetState;

// Single Attempt of Reset 0xFF (R0, R1, R2)
typedef struct {
    uint8_t  retry_num;
    uint64_t tx_time_us;
    uint8_t  status_before_tx;
    uint8_t  status_after_tx;
    
    // RX #1 (Expected ACK 0xFA or RESEND 0xFE)
    uint64_t rx1_time_us;
    uint8_t  rx1_byte;
    uint8_t  rx1_status;
    bool     rx1_aux;
    bool     rx1_is_ack;      // 0xFA
    bool     rx1_is_resend;   // 0xFE
    bool     rx1_timeout;
    uint32_t rx1_latency_us;

    // RX #2 (Expected BAT 0xAA)
    uint64_t rx2_time_us;
    uint8_t  rx2_byte;
    uint8_t  rx2_status;
    bool     rx2_aux;
    bool     rx2_is_bat_ok;   // 0xAA
    bool     rx2_is_bat_fail; // 0xFC
    bool     rx2_timeout;
    uint32_t rx2_latency_us;
} KbdResetAttempt;

// Keyboard Device Reset (0xFF) State Machine
typedef struct {
    KbdResetState   current_state;
    const char      *state_str;
    
    // Pre-reset drain capture (to verify if 0xFE is stale)
    uint8_t         drained_count;
    uint8_t         drained_bytes[8];
    uint8_t         drained_status[8];
    bool            drained_aux[8];

    // Status transition tracking
    uint8_t         status_before_reset;
    uint8_t         status_after_ack;
    uint8_t         status_after_bat;

    // Attempts (up to 3 retries)
    KbdResetAttempt attempts[3];
    uint8_t         attempt_count;

    // Final Reset & BAT Verdict
    bool            reset_ack_pass;  // Got 0xFA
    bool            bat_pass;        // Got 0xAA
    uint32_t        total_reset_us;
} PS2KbdResetSM;

// Keyboard Enable Scanning (0xF4) State Machine
typedef struct {
    uint8_t  retry_count;
    uint8_t  status_before;
    uint8_t  status_after_tx;
    uint8_t  rx_byte;
    uint8_t  status_at_rx;
    bool     aux;
    bool     is_ack;       // 0xFA
    bool     is_resend;    // 0xFE
    bool     timeout;
    bool     skipped_no_bat;
    uint32_t latency_us;
    bool     passed;
} PS2ScanSM;

// Echo Probe (0xEE)
typedef struct {
    uint8_t  status_before;
    uint8_t  status_after_tx;
    uint8_t  rx_byte;
    uint8_t  status_at_rx;
    bool     aux;
    bool     is_echo;      // 0xEE or 0xFA
    bool     is_resend;    // 0xFE
    bool     timeout;
    uint32_t latency_us;
} PS2EchoSM;

// Retry / Resend Transaction Record for 0xED
typedef struct {
    uint8_t  retry_num;
    uint64_t tx_time_us;
    uint8_t  status_before_tx;
    uint8_t  status_after_tx;
    uint64_t obf_time_us;
    uint64_t rx_time_us;
    uint8_t  rx_byte;
    uint8_t  status_at_rx;
    bool     aux;
    bool     is_ack;       // 0xFA
    bool     is_resend;    // 0xFE
    bool     is_timeout;
    uint32_t latency_us;
} PS2RetryTrace;

// 8042 Controller Configuration
typedef struct {
    uint8_t raw_config;
    bool    translation;
    bool    kbd_clock;
    bool    mouse_clock;
    bool    irq1;
    bool    irq12;
} PS2ControllerConfig;

// Bit 6 (Timeout Error) Origin
typedef enum {
    BIT6_NEVER_SET = 0,
    BIT6_BEFORE_TX = 1,
    BIT6_AFTER_TX = 2,
    BIT6_WHILE_WAITING = 3,
    BIT6_AFTER_RX = 4
} Bit6SetOrigin;

// LAN Statistics
typedef struct {
    bool     link_up;
    uint16_t link_speed_mbps;
    uint8_t  irq;
    uint8_t  mac[6];
    uint64_t rx_frames;
    uint64_t tx_frames;
    uint64_t rx_drops;
    uint64_t tx_drops;
} PS2LanStats;

// Full Micro-Debug Engine State
typedef struct {
    PS2ControllerConfig    config_initial;
    PS2ControllerConfig    config_post_selftest;
    PS2ControllerConfig    config_post_reconfig;
    PS2StatusSnapshot      status_t0_boot;
    PS2StatusSnapshot      status_after_selftest;
    PS2StatusSnapshot      status_after_iface;
    PS2StatusSnapshot      status_after_reconfig;
    PS2StatusSnapshot      status_final;
    Bit6SetOrigin          bit6_origin;

    // Bus Diagnostic Results
    PS2SelfTestResult      self_test;
    PS2InterfaceTestResult iface_test;
    PS2InterfaceTestResult aux_iface_test;
    
    // Explicit Keyboard State Machine
    PS2KbdResetSM          kbd_reset;
    PS2ScanSM              kbd_scan;
    PS2EchoSM              cmd_ee;

    // 0xED Command Retries & Mask (Only executed if channel healthy)
    bool                   ed_skipped_unhealthy;
    PS2RetryTrace          ed_retries[4];
    uint8_t                ed_retry_count;
    bool                   ed_ack_received;
    uint8_t                mask_sent;
    PS2RetryTrace          mask_retries[4];
    uint8_t                mask_retry_count;
    bool                   mask_ack_received;
    uint32_t               total_transaction_us;

    bool                   all_commands_resend;
    bool                   ed_only_resend;
    bool                   physical_led_toggle_verified;

    // Live LAN Telemetry
    PS2LanStats            lan;

    // Event Timeline
    PS2MicroEvent          events[32];
    uint8_t                event_count;

    // Verdict String
    const char             *final_verdict_str;
    bool                   final_verdict_pass;
} PS2MicroDebug;

extern PS2MicroDebug g_ps2_micro_debug;

void ps2_micro_debug_init(boot_info_t *boot_info);
void ps2_micro_debug_run_test_suite(void);
void ps2_micro_debug_render(void);
void ps2_micro_debug_run(boot_info_t *boot_info);

#endif /* PS2_MICRO_DEBUG_H */
