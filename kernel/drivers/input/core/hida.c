#include "hida.h"
#include "kernel/drivers/input/input_abstraction.h"
#include "kernel/core/vizier/include/vizier.h"
#include "kernel/drivers/display/display.h"

static uint32_t g_hida_owner = HIDA_BACKEND_NONE;
static HidaState g_hida_state = HIDA_STATE_DETECTING;
static uint64_t g_hida_last_event_tick = 0;
static uint32_t g_hida_fallback_count = 0;
static uint32_t g_hida_conflict_count = 0;

void hida_init(void) {
    g_hida_owner = HIDA_BACKEND_NONE;
    g_hida_state = HIDA_STATE_DETECTING;
    
    VizierContract c = {0};
    c.subsystem_name = "HIDA";
    c.subsystem_id = 110;
    vizier_register_subsystem(&c);
}

static void hida_arbitrate(uint32_t backend_id) {
    if (g_hida_owner == HIDA_BACKEND_NONE) {
        g_hida_owner = backend_id;
        g_hida_state = HIDA_STATE_ACTIVE;
        vizier_claim_authority(110, VIZIER_CAP_INPUT_POINTER_RAW);
    } else if (g_hida_owner != backend_id) {
        g_hida_conflict_count++;
        extern uint64_t timer_get_ticks(void);
        if (timer_get_ticks() - g_hida_last_event_tick > 5000) {
            g_hida_owner = backend_id;
            g_hida_fallback_count++;
            g_hida_state = HIDA_STATE_FALLBACK;
            vizier_report_violation(110, "HIDA Fallback Triggered");
        }
    }
}

void hida_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint8_t buttons, int32_t scroll) {
    hida_arbitrate(backend_id);
    if (g_hida_owner == backend_id) {
        extern uint64_t timer_get_ticks(void);
        g_hida_last_event_tick = timer_get_ticks();
        input_push_absolute(x, y, buttons, scroll);
    }
}

void hida_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll) {
    hida_arbitrate(backend_id);
    if (g_hida_owner == backend_id) {
        extern uint64_t timer_get_ticks(void);
        g_hida_last_event_tick = timer_get_ticks();
        input_push_relative(dx, dy, buttons, scroll);
    }
}

void hida_dump_status(void) {
    display_print("\nINPUT AUTHORITY:\nowner = ");
    if (g_hida_owner == HIDA_BACKEND_VMMOUSE) display_print("VMMOUSE\n");
    else if (g_hida_owner == HIDA_BACKEND_PS2) display_print("PS2\n");
    else if (g_hida_owner == HIDA_BACKEND_USB) display_print("USB\n");
    else display_print("NONE\n");
    
    display_print("health = ");
    if (g_hida_state == HIDA_STATE_ACTIVE) display_print("OK\n");
    else if (g_hida_state == HIDA_STATE_FALLBACK) display_print("FALLBACK\n");
    else if (g_hida_state == HIDA_STATE_DEGRADED) display_print("DEGRADED\n");
    else if (g_hida_state == HIDA_STATE_FAILED) display_print("FAILED\n");
    else display_print("DETECTING\n");
    
    display_print("last_event_tick = ");
    display_print_dec(g_hida_last_event_tick);
    display_print("\nfallback_count = ");
    display_print_dec(g_hida_fallback_count);
    display_print("\nconflict_count = ");
    display_print_dec(g_hida_conflict_count);
    display_print("\n");
}
