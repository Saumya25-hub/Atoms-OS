#include "hida.h"
#include "kernel/core/vizier/include/vizier.h"
#include "kernel/drivers/display/display.h"
#include "ccte.h"

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
    g_hida_owner = HIDA_BACKEND_USB;
}

void hida_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint32_t max_x, uint32_t max_y, uint8_t buttons, int32_t scroll) {
    hida_arbitrate(backend_id);
    
    uint32_t auth_owner = vizier_get_authoritative_owner(VIZIER_CAP_INPUT_POINTER_RAW);
    if (auth_owner != 0 && auth_owner != 110) {
        return; // Suppressed by Vizier Governance (another subsystem owns it)
    }
    if (g_hida_owner != HIDA_BACKEND_NONE && g_hida_owner != backend_id) {
        return; // Suppressed by HIDA Arbitration
    }
    
    extern uint64_t timer_get_ticks(void);
    g_hida_last_event_tick = timer_get_ticks();
    
    // Route directly to Canonical Coordinate Transform Engine (Phase D)
    ccte_push_absolute(backend_id, x, y, max_x, max_y, buttons, scroll);
}

void hida_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll) {
    /*
    display_print("[HIDA] push_relative from backend ");
    if (backend_id == HIDA_BACKEND_USB) display_print("USB\n");
    else if (backend_id == HIDA_BACKEND_VMMOUSE) display_print("VMMOUSE\n");
    else display_print("PS2\n");
    */
    
    hida_arbitrate(backend_id);
    
    uint32_t auth_owner = vizier_get_authoritative_owner(VIZIER_CAP_INPUT_POINTER_RAW);
    if (auth_owner != 0 && auth_owner != 110) {
        return; // Suppressed by Vizier Governance (another subsystem owns it)
    }
    if (g_hida_owner != HIDA_BACKEND_NONE && g_hida_owner != backend_id) {
        return; // Suppressed by HIDA Arbitration
    }
    
    extern uint64_t timer_get_ticks(void);
    g_hida_last_event_tick = timer_get_ticks();
    
    // Route directly to Canonical Coordinate Transform Engine (Phase D)
    ccte_push_relative(backend_id, dx, dy, buttons, scroll);
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
