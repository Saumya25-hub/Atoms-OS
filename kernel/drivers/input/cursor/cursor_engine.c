/**
 * @file cursor_engine.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Engine Coordinator Implementation
 * @section PURPOSE
 * Coordinates visual presentation modules and integrates as Tier 1 Event Dispatcher consumer.
 */

#include "cursor_engine.h"
#include <stddef.h>

static uint32_t g_consumer_id = 0;

void cursor_engine_init(uint32_t screen_width, uint32_t screen_height) {
    /* 1. Initialize core state and theme registries */
    cursor_state_init(screen_width, screen_height);
    cursor_theme_init();
    cursor_animation_init();
    cursor_diag_init();
    
    /* 2. Proactively probe and initialize hardware/software backends */
    cursor_backend_init();
    cursor_renderer_init();

    /* 3. Register as Tier 1 Consumer in Phase 4 Event Dispatcher */
    uint32_t event_mask = (1 << INPUT_EVENT_TYPE_MOTION_ABSOLUTE) |
                          (1 << INPUT_EVENT_TYPE_MOTION_RELATIVE) |
                          (1 << INPUT_EVENT_TYPE_BUTTON);

    g_consumer_id = dispatcher_consumers_register(
        "CursorEngine",
        DISPATCH_TIER_1_CURSOR_ENGINE,
        event_mask,
        cursor_engine_dispatch_cb,
        NULL
    );
}

void cursor_engine_update_resolution(uint32_t screen_width, uint32_t screen_height) {
    cursor_state_update_resolution(screen_width, screen_height);
}

DispatchResult cursor_engine_dispatch_cb(const DispatcherEvent* event, void* context) {
    (void)context;
    if (!event) return DISPATCH_CONTINUE;

    bool moved = false;

    if (event->type == INPUT_EVENT_TYPE_MOTION_ABSOLUTE) {
        cursor_state_set_position(event->data.motion_abs.x, event->data.motion_abs.y);
        moved = true;
    } else if (event->type == INPUT_EVENT_TYPE_MOTION_RELATIVE) {
        int32_t cur_x = 0, cur_y = 0;
        cursor_state_get_position(&cur_x, &cur_y);
        cursor_state_set_position(cur_x + event->data.motion_rel.dx, cur_y + event->data.motion_rel.dy);
        moved = true;
    } else if (event->type == INPUT_EVENT_TYPE_BUTTON) {
        /* Button transitions logged; can trigger active drag shape themes in future */
        cursor_diag_log_update(false);
    }

    if (moved) {
        cursor_diag_log_update(true);
        cursor_renderer_update(true);
    }

    /* CRITICAL ARCHITECTURAL RULE:
     * Cursor Engine is Tier 1 (Visual Presentation). It MUST NEVER consume pointer events.
     * Events must continue propagating to Tier 2 (Desktop), Tier 3 (WM), Tier 4 (Widgets),
     * and Tier 5 (Apps) for hit-testing and interactivity.
     */
    return DISPATCH_CONTINUE;
}

void cursor_engine_render_overlay(const void* fb_ptr) {
    /* Forward presentation request to deterministic software renderer */
    cursor_renderer_draw_software(fb_ptr);
}
