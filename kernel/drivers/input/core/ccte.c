#include "ccte.h"
#include "kernel/core/vizier/include/vizier.h"
#include "kernel/drivers/input/core/input_core.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/input/pointer/pointer_velocity.h"

// FP16 base canonical accumulator
static int64_t g_ccte_accum_x = (CCTE_VIRTUAL_MAX / 2) << 16;
static int64_t g_ccte_accum_y = (CCTE_VIRTUAL_MAX / 2) << 16;

void ccte_init(void) {
    VizierContract c = {0};
    c.subsystem_name = "CCTE";
    c.subsystem_id = VIZIER_SUBSYSTEM_CCTE;
    vizier_register_subsystem(&c);
    display_print("[CCTE] Canonical Coordinate Transform Engine Initialized.\n");
}

void ccte_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint32_t max_x, uint32_t max_y, uint8_t buttons, int32_t scroll) {
    if (max_x == 0) max_x = 1;
    if (max_y == 0) max_y = 1;

    // Convert raw absolute to canonical 0-65535 space
    int64_t canon_x = ((int64_t)x * CCTE_VIRTUAL_MAX) / max_x;
    int64_t canon_y = ((int64_t)y * CCTE_VIRTUAL_MAX) / max_y;

    // Direct write to canonical accumulators
    g_ccte_accum_x = canon_x << 16;
    g_ccte_accum_y = canon_y << 16;

    InputCoreEvent ev = {0};
    ev.device_id = backend_id;
    ev.type = INPUT_EVENT_TYPE_MOTION_ABSOLUTE;
    ev.timestamp_us = timer_get_ticks() * 1000;
    ev.data.motion_abs.x = (int32_t)canon_x;
    ev.data.motion_abs.y = (int32_t)canon_y;
    ev.data.motion_abs.max_x = CCTE_VIRTUAL_MAX;
    ev.data.motion_abs.max_y = CCTE_VIRTUAL_MAX;
    ev.data.motion_abs.buttons = buttons;
    
    input_core_push_event(&ev);
    input_core_dispatch_events();
    
    if (scroll != 0) {
        InputCoreEvent sev = {0};
        sev.device_id = backend_id;
        sev.type = INPUT_EVENT_TYPE_SCROLL;
        sev.timestamp_us = ev.timestamp_us;
        sev.data.scroll.delta_y = scroll;
        input_core_push_event(&sev);
        input_core_dispatch_events();
    }
}

void ccte_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll) {
    uint64_t now_us = timer_get_ticks() * 1000;
    int32_t vel_raw = 0;
    
    // Stage 3: Ballistic Acceleration Calculation (Optional now, but kept for signature)
    int32_t accel_fp16 = pointer_velocity_calculate(dx, dy, now_us, &vel_raw);
    
    // For ATOMS OS, 1 pixel on a 1024-width screen in 65535-space is exactly 64 units.
    // To ensure "butter smooth" 1:1 pixel mapping without fractional loss causing 10FPS lag,
    // we bypass the floating-point decelerator and map directly.
    int64_t canon_dx = (int64_t)dx * 64;
    int64_t canon_dy = (int64_t)dy * 64;
    
    // Optional: apply light acceleration if moving fast, but keep base 1:1
    if (accel_fp16 > 65536) {
        canon_dx = (canon_dx * accel_fp16) >> 16;
        canon_dy = (canon_dy * accel_fp16) >> 16;
    }
    
    int32_t before_x = (int32_t)(g_ccte_accum_x >> 16);
    int32_t before_y = (int32_t)(g_ccte_accum_y >> 16);

    g_ccte_accum_x += canon_dx << 16;
    g_ccte_accum_y += canon_dy << 16;
    
    // Clamp to Canonical Bounding Box
    if (g_ccte_accum_x < 0) g_ccte_accum_x = 0;
    if (g_ccte_accum_x > ((int64_t)CCTE_VIRTUAL_MAX << 16)) g_ccte_accum_x = ((int64_t)CCTE_VIRTUAL_MAX << 16);
    
    if (g_ccte_accum_y < 0) g_ccte_accum_y = 0;
    if (g_ccte_accum_y > ((int64_t)CCTE_VIRTUAL_MAX << 16)) g_ccte_accum_y = ((int64_t)CCTE_VIRTUAL_MAX << 16);
    
    InputCoreEvent ev = {0};
    ev.device_id = backend_id;
    ev.type = INPUT_EVENT_TYPE_MOTION_ABSOLUTE;
    ev.timestamp_us = now_us;
    ev.data.motion_abs.x = (int32_t)(g_ccte_accum_x >> 16);
    ev.data.motion_abs.y = (int32_t)(g_ccte_accum_y >> 16);
    ev.data.motion_abs.max_x = CCTE_VIRTUAL_MAX;
    ev.data.motion_abs.max_y = CCTE_VIRTUAL_MAX;
    ev.data.motion_abs.buttons = buttons;
    
    input_core_push_event(&ev);
    
    if (scroll != 0) {
        InputCoreEvent sev = {0};
        sev.device_id = backend_id;
        sev.type = INPUT_EVENT_TYPE_SCROLL;
        sev.timestamp_us = now_us;
        sev.data.scroll.delta_y = scroll;
        input_core_push_event(&sev);
    }
}

void ccte_dump_status(void) {
    display_print("[CCTE] Canonical X: ");
    extern void display_print_dec(uint64_t);
    display_print_dec((uint64_t)(g_ccte_accum_x >> 16));
    display_print(" Y: ");
    display_print_dec((uint64_t)(g_ccte_accum_y >> 16));
    display_print("\n");
}
