#ifndef BMDE_H
#define BMDE_H

#include <stdint.h>
#include <stdbool.h>

// Uncomment to enable the BOS Mouse Diagnostic Engine
#define BMDE_DEBUG 1

#ifdef BMDE_DEBUG

#define BMDE_HISTORY_SIZE 512

typedef struct {
    uint8_t bytes[3];
    int32_t raw_dx;
    int32_t raw_dy;
    bool overflow_x;
    bool overflow_y;
    int32_t filtered_dx;
    int32_t filtered_dy;
    int32_t pre_clamp_x;
    int32_t pre_clamp_y;
    int32_t post_clamp_x;
    int32_t post_clamp_y;
    bool clamped;
} BMDE_Packet;

typedef struct {
    // Module 1: Hardware Diagnostics
    bool port_ok;
    bool mouse_present;
    bool streaming_enabled;
    uint32_t sample_rate;
    uint32_t resolution;
    uint32_t scaling;
    const char* last_hardware_error;

    // Module 2: IRQ Diagnostics
    bool irq_registered;
    bool irq_enabled;
    uint64_t irq_count;
    uint32_t irq_per_sec;
    uint64_t last_irq_time;
    uint32_t missed_irqs;
    bool irq_stalled;

    // Module 3: Packet Decoder
    uint8_t current_bytes[3];
    bool btn_left;
    bool btn_right;
    bool btn_middle;
    bool sign_x_pos;
    bool sign_y_pos;
    bool overflow_x;
    bool overflow_y;

    // Module 4: Movement
    int32_t dx;
    int32_t dy;
    int32_t abs_x;
    int32_t abs_y;
    uint32_t velocity;
    uint32_t acceleration;

    // Module 5: Packet History
    BMDE_Packet history[BMDE_HISTORY_SIZE];
    uint32_t history_head;

    // Module 6: Error Recovery
    uint32_t sync_errors;
    uint32_t packet_loss;
    uint32_t missing_bytes;
    uint32_t overflows;
    uint32_t invalid_acks;
    uint32_t timeouts;
    uint32_t unexpected_responses;
    uint32_t recoveries;

    // Module 7: Cursor Diagnostics
    bool cursor_visible;
    uint32_t cursor_draw_calls;
    bool background_restore_ok;
    const char* cursor_state;
    int32_t last_drawn_x;
    int32_t last_drawn_y;

    // Module 8: Queue Diagnostics
    uint32_t queue_size;
    uint32_t dropped_events;
    uint32_t peak_queue;
    uint32_t avg_queue;
    bool queue_overflow;

    // Module 9: Performance Profiler (in ticks)
    uint64_t perf_irq;
    uint64_t perf_decode;
    uint64_t perf_queue;
    uint64_t perf_cursor;

    // Module 10: Driver Statistics
    uint64_t boot_time_ms;
    uint64_t total_packets;

} BMDE_State;

extern BMDE_State bmde_state;

void bmde_init(void);

#endif // BMDE_DEBUG

#endif // BMDE_H
