#include "mouse_telemetry.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"

static volatile uint64_t g_mouse_seq_counter = 0;
static volatile uint64_t g_last_logged_tick = 0;

uint64_t mouse_telemetry_next_seq(void) {
    return ++g_mouse_seq_counter;
}

uint64_t mouse_telemetry_current_seq(void) {
    return g_mouse_seq_counter;
}

void mouse_telemetry_log(uint32_t stage, int32_t x, int32_t y, uint32_t buttons, const char* tag) {
    uint64_t tick = timer_get_ticks();
    uint64_t seq = g_mouse_seq_counter;
    
    // Non-blocking diagnostic serial print
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);

    display_print("[MOUSE_TEL] seq=");
    display_print_dec(seq);
    display_print(" tick=");
    display_print_dec(tick);
    display_print(" stage=");
    display_print_dec(stage);
    display_print(" (");
    if (tag) display_print(tag);
    display_print(") x=");
    display_print_dec((uint32_t)x);
    display_print(" y=");
    display_print_dec((uint32_t)y);
    display_print(" btns=");
    display_print_dec(buttons);
    display_print("\n");
}
