#include "../include/bcm.h"
#include "../include/bcm_internal.h"
#include "kernel/wm/bwe/include/bwe.h"

/* Static core state instance in BSS (Zero dynamic heap allocation) */
BCM_CoreState g_bcm_state = {0};

/* External graphics resolution, timer, and diagnostic hooks */
extern uint32_t BOVISUAL_Graphics_GetWidth(void);
extern uint32_t BOVISUAL_Graphics_GetHeight(void);
extern uint64_t timer_get_ticks(void);
extern void com1_puts(const char* s);
extern void serial_write_direct(const char* str);
extern void serial_write_dec_direct(int val);

/* Helper to check CPU Interrupt Flag (IF) */
static inline bool bcm_is_interrupt_enabled(void) {
    uint64_t rflags;
    __asm__ volatile("pushfq; popq %0" : "=r"(rflags));
    return (rflags & (1ULL << 9)) != 0; /* Bit 9 = Interrupt Enable Flag */
}

/* ========================================================================= */
/* BCM Lifecycle APIs                                                        */
/* ========================================================================= */

bcm_error_t BCM_Init(void) {
    g_bcm_state.initialized = false;
    g_bcm_state.state = BCM_STATE_IDLE;
    g_bcm_state.is_composing = false;
    g_bcm_state.is_presenting = false;
    g_bcm_state.pending_damage = false;
    g_bcm_state.full_damage_requested = false;
    g_bcm_state.dirty_count = 0;

    g_bcm_state.next_dirty_count = 0;
    g_bcm_state.next_pending_damage = false;
    g_bcm_state.next_full_damage_requested = false;

    g_bcm_state.current_frame_id = 0;
    g_bcm_state.in_flight_frame_id = 0;
    g_bcm_state.last_completed_frame_id = 0;
    g_bcm_state.presentation_start_tick = 0;
    g_bcm_state.presentation_timeout_ms = BCM_DEFAULT_PRESENT_TIMEOUT_MS;
    
    g_bcm_state.last_timer_tick = 0;
    g_bcm_state.last_compose_tick = 0;
    g_bcm_state.last_present_tick = 0;
    g_bcm_state.pacing_interval_ms = BCM_DEFAULT_PACING_INTERVAL_MS;
    g_bcm_state.min_inter_frame_ms = BCM_MIN_INTER_FRAME_GAP_MS;

    g_bcm_state.fps_window_start_tick = 0;
    g_bcm_state.fps_window_frame_count = 0;

    for (uint32_t i = 0; i < BCM_MAX_DIRTY_RECTS; i++) {
        g_bcm_state.dirty_rects[i].x = 0;
        g_bcm_state.dirty_rects[i].y = 0;
        g_bcm_state.dirty_rects[i].width = 0;
        g_bcm_state.dirty_rects[i].height = 0;

        g_bcm_state.next_dirty_rects[i].x = 0;
        g_bcm_state.next_dirty_rects[i].y = 0;
        g_bcm_state.next_dirty_rects[i].width = 0;
        g_bcm_state.next_dirty_rects[i].height = 0;
    }

    for (uint8_t* p = (uint8_t*)&g_bcm_state.telemetry; p < (uint8_t*)&g_bcm_state.telemetry + sizeof(BCM_Telemetry); p++) {
        *p = 0;
    }

    g_bcm_state.initialized = true;
    com1_puts("[BCM] INIT: BOS Composition Manager Initialized (Phase 1-7 Engine Active)\r\n");
    return BCM_OK;
}

/* ========================================================================= */
/* Coalescing & Geometric Helpers                                            */
/* ========================================================================= */

static inline bool bcm_rects_overlap_or_adjacent(const BCM_Rect* r1, const BCM_Rect* r2) {
    return !(r1->x + r1->width < r2->x ||
             r2->x + r2->width < r1->x ||
             r1->y + r1->height < r2->y ||
             r2->y + r2->height < r1->y);
}

static inline BCM_Rect bcm_merge_rects(const BCM_Rect* r1, const BCM_Rect* r2) {
    BCM_Rect m;
    int32_t x1 = (r1->x < r2->x) ? r1->x : r2->x;
    int32_t y1 = (r1->y < r2->y) ? r1->y : r2->y;
    int32_t x2 = (r1->x + r1->width > r2->x + r2->width) ? (r1->x + r1->width) : (r2->x + r2->width);
    int32_t y2 = (r1->y + r1->height > r2->y + r2->height) ? (r1->y + r1->height) : (r2->y + r2->height);
    m.x = x1;
    m.y = y1;
    m.width = x2 - x1;
    m.height = y2 - y1;
    return m;
}

void BCM_Internal_CoalesceDamage(void) {
    int32_t screen_w = (int32_t)BOVISUAL_Graphics_GetWidth();
    int32_t screen_h = (int32_t)BOVISUAL_Graphics_GetHeight();
    if (screen_w <= 0) screen_w = 1920;
    if (screen_h <= 0) screen_h = 1080;

    if (g_bcm_state.full_damage_requested) {
        g_bcm_state.dirty_count = 1;
        g_bcm_state.dirty_rects[0].x = 0;
        g_bcm_state.dirty_rects[0].y = 0;
        g_bcm_state.dirty_rects[0].width = screen_w;
        g_bcm_state.dirty_rects[0].height = screen_h;
        g_bcm_state.telemetry.last_damage_area = (uint32_t)((uint64_t)screen_w * (uint64_t)screen_h);
        return;
    }

    if (g_bcm_state.dirty_count <= 1) {
        if (g_bcm_state.dirty_count == 1) {
            g_bcm_state.telemetry.last_damage_area = (uint32_t)((uint64_t)g_bcm_state.dirty_rects[0].width * (uint64_t)g_bcm_state.dirty_rects[0].height);
        }
        return;
    }

    bool merged = true;
    while (merged) {
        merged = false;
        for (uint32_t i = 0; i < g_bcm_state.dirty_count; i++) {
            for (uint32_t j = i + 1; j < g_bcm_state.dirty_count; j++) {
                if (bcm_rects_overlap_or_adjacent(&g_bcm_state.dirty_rects[i], &g_bcm_state.dirty_rects[j])) {
                    BCM_Rect union_rect = bcm_merge_rects(&g_bcm_state.dirty_rects[i], &g_bcm_state.dirty_rects[j]);
                    uint64_t union_area = (uint64_t)union_rect.width * (uint64_t)union_rect.height;
                    uint64_t sum_area = (uint64_t)g_bcm_state.dirty_rects[i].width * (uint64_t)g_bcm_state.dirty_rects[i].height +
                                        (uint64_t)g_bcm_state.dirty_rects[j].width * (uint64_t)g_bcm_state.dirty_rects[j].height;

                    /* Merge if overlapping or if union area is not overly bloated (<= 140% of separate sum) */
                    if (union_area <= (sum_area * 140ULL) / 100ULL || g_bcm_state.dirty_count >= BCM_MAX_DIRTY_RECTS) {
                        g_bcm_state.dirty_rects[i] = union_rect;
                        for (uint32_t k = j; k < g_bcm_state.dirty_count - 1; k++) {
                            g_bcm_state.dirty_rects[k] = g_bcm_state.dirty_rects[k + 1];
                        }
                        g_bcm_state.dirty_count--;
                        g_bcm_state.telemetry.coalesced_damage_requests++;
                        merged = true;
                        break;
                    }
                }
            }
            if (merged) break;
        }
    }

    /* Calculate total damaged area and check 65% collapse threshold */
    uint64_t total_area = 0;
    for (uint32_t i = 0; i < g_bcm_state.dirty_count; i++) {
        total_area += (uint64_t)g_bcm_state.dirty_rects[i].width * (uint64_t)g_bcm_state.dirty_rects[i].height;
    }
    g_bcm_state.telemetry.last_damage_area = (uint32_t)total_area;
    uint64_t screen_area = (uint64_t)screen_w * (uint64_t)screen_h;
    if (total_area > (screen_area * (uint64_t)BCM_SCREEN_COLLAPSE_PERCENT) / 100ULL) {
        g_bcm_state.full_damage_requested = true;
        g_bcm_state.dirty_count = 1;
        g_bcm_state.dirty_rects[0].x = 0;
        g_bcm_state.dirty_rects[0].y = 0;
        g_bcm_state.dirty_rects[0].width = screen_w;
        g_bcm_state.dirty_rects[0].height = screen_h;
        g_bcm_state.telemetry.full_repaint_count++;
    }

    if (g_bcm_state.dirty_count > g_bcm_state.telemetry.max_rect_count_observed) {
        g_bcm_state.telemetry.max_rect_count_observed = g_bcm_state.dirty_count;
    }
}

void BCM_Internal_ResetDirtyRects(void) {
    g_bcm_state.dirty_count = 0;
    g_bcm_state.full_damage_requested = false;
}

void BCM_Internal_PromoteNextFrameDamage(void) {
    if (g_bcm_state.next_pending_damage) {
        g_bcm_state.dirty_count = g_bcm_state.next_dirty_count;
        for (uint32_t i = 0; i < g_bcm_state.next_dirty_count && i < BCM_MAX_DIRTY_RECTS; i++) {
            g_bcm_state.dirty_rects[i] = g_bcm_state.next_dirty_rects[i];
        }
        g_bcm_state.full_damage_requested = g_bcm_state.next_full_damage_requested;
        g_bcm_state.pending_damage = true;
        g_bcm_state.state = BCM_STATE_REQUESTED;

        /* Clear next-frame buffer */
        g_bcm_state.next_dirty_count = 0;
        g_bcm_state.next_pending_damage = false;
        g_bcm_state.next_full_damage_requested = false;

        BCM_Internal_CoalesceDamage();
    } else {
        BCM_Internal_ResetDirtyRects();
        g_bcm_state.pending_damage = false;
        g_bcm_state.state = BCM_STATE_IDLE;
    }
}

/* ========================================================================= */
/* IRQ-Safe Damage Ingestion APIs (Phases 3, 6, 7)                           */
/* ========================================================================= */

void BCM_RequestDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!g_bcm_state.initialized) return;

    if (width <= 0 || height <= 0) {
        g_bcm_state.telemetry.dropped_invalid_count++;
        return;
    }

    g_bcm_state.telemetry.total_damage_requests++;
    if (bcm_is_interrupt_enabled()) {
        g_bcm_state.telemetry.task_damage_requests++;
    } else {
        g_bcm_state.telemetry.irq_damage_requests++;
    }

    int32_t screen_w = (int32_t)BOVISUAL_Graphics_GetWidth();
    int32_t screen_h = (int32_t)BOVISUAL_Graphics_GetHeight();
    if (screen_w <= 0) screen_w = 1920;
    if (screen_h <= 0) screen_h = 1080;

    /* Screen Boundary Clamping */
    int32_t cx1 = (x > 0) ? x : 0;
    int32_t cy1 = (y > 0) ? y : 0;
    int32_t rx2 = x + width;
    int32_t ry2 = y + height;
    int32_t cx2 = (rx2 < screen_w) ? rx2 : screen_w;
    int32_t cy2 = (ry2 < screen_h) ? ry2 : screen_h;

    if (cx1 >= cx2 || cy1 >= cy2) {
        g_bcm_state.telemetry.dropped_invalid_count++;
        return;
    }

    BCM_Rect clipped = { cx1, cy1, cx2 - cx1, cy2 - cy1 };

    /* In-Flight Protection: If currently presenting, buffer in next-frame envelope */
    if (g_bcm_state.is_presenting) {
        if (g_bcm_state.next_dirty_count < BCM_MAX_DIRTY_RECTS) {
            g_bcm_state.next_dirty_rects[g_bcm_state.next_dirty_count++] = clipped;
        } else {
            g_bcm_state.next_full_damage_requested = true;
            g_bcm_state.next_dirty_count = 1;
            g_bcm_state.next_dirty_rects[0].x = 0;
            g_bcm_state.next_dirty_rects[0].y = 0;
            g_bcm_state.next_dirty_rects[0].width = screen_w;
            g_bcm_state.next_dirty_rects[0].height = screen_h;
        }
        g_bcm_state.next_pending_damage = true;
        g_bcm_state.telemetry.coalesced_damage_requests++;
        return;
    }

    /* If full damage is already active, coalesce immediately */
    if (g_bcm_state.full_damage_requested) {
        g_bcm_state.telemetry.coalesced_damage_requests++;
        g_bcm_state.pending_damage = true;
        if (g_bcm_state.state == BCM_STATE_IDLE) {
            g_bcm_state.state = BCM_STATE_REQUESTED;
            g_bcm_state.telemetry.frames_requested++;
        }
        return;
    }

    /* Duplicate & Complete Containment Check */
    for (uint32_t i = 0; i < g_bcm_state.dirty_count; i++) {
        BCM_Rect* r = &g_bcm_state.dirty_rects[i];
        if (clipped.x >= r->x && clipped.y >= r->y &&
            clipped.x + clipped.width <= r->x + r->width &&
            clipped.y + clipped.height <= r->y + r->height) {
            g_bcm_state.telemetry.coalesced_damage_requests++;
            g_bcm_state.pending_damage = true;
            return; /* Already completely covered */
        }
    }

    /* Check if new rect subsumes an existing rect */
    for (uint32_t i = 0; i < g_bcm_state.dirty_count; i++) {
        BCM_Rect* r = &g_bcm_state.dirty_rects[i];
        if (r->x >= clipped.x && r->y >= clipped.y &&
            r->x + r->width <= clipped.x + clipped.width &&
            r->y + r->height <= clipped.y + clipped.height) {
            *r = clipped;
            BCM_Internal_CoalesceDamage();
            g_bcm_state.telemetry.coalesced_damage_requests++;
            g_bcm_state.pending_damage = true;
            return;
        }
    }

    /* Add rectangle if capacity allows */
    if (g_bcm_state.dirty_count < BCM_MAX_DIRTY_RECTS) {
        g_bcm_state.dirty_rects[g_bcm_state.dirty_count++] = clipped;
        BCM_Internal_CoalesceDamage();
    } else {
        /* Capacity exceeded: coalesce aggressively */
        BCM_Internal_CoalesceDamage();
        if (g_bcm_state.dirty_count >= BCM_MAX_DIRTY_RECTS) {
            /* Find the two rectangles whose union creates minimal added area */
            uint32_t best_i = 0, best_j = 1;
            uint64_t min_added_area = 0xFFFFFFFFFFFFFFFFULL;
            for (uint32_t i = 0; i < g_bcm_state.dirty_count; i++) {
                for (uint32_t j = i + 1; j < g_bcm_state.dirty_count; j++) {
                    BCM_Rect u = bcm_merge_rects(&g_bcm_state.dirty_rects[i], &g_bcm_state.dirty_rects[j]);
                    uint64_t u_area = (uint64_t)u.width * (uint64_t)u.height;
                    uint64_t orig = (uint64_t)g_bcm_state.dirty_rects[i].width * (uint64_t)g_bcm_state.dirty_rects[i].height +
                                    (uint64_t)g_bcm_state.dirty_rects[j].width * (uint64_t)g_bcm_state.dirty_rects[j].height;
                    uint64_t diff = (u_area > orig) ? (u_area - orig) : 0;
                    if (diff < min_added_area) {
                        min_added_area = diff;
                        best_i = i;
                        best_j = j;
                    }
                }
            }
            g_bcm_state.dirty_rects[best_i] = bcm_merge_rects(&g_bcm_state.dirty_rects[best_i], &g_bcm_state.dirty_rects[best_j]);
            for (uint32_t k = best_j; k < g_bcm_state.dirty_count - 1; k++) {
                g_bcm_state.dirty_rects[k] = g_bcm_state.dirty_rects[k + 1];
            }
            g_bcm_state.dirty_count--;
            g_bcm_state.dirty_rects[g_bcm_state.dirty_count++] = clipped;
            BCM_Internal_CoalesceDamage();
        } else {
            g_bcm_state.dirty_rects[g_bcm_state.dirty_count++] = clipped;
        }
    }

    g_bcm_state.pending_damage = true;
    if (g_bcm_state.state == BCM_STATE_IDLE) {
        g_bcm_state.state = BCM_STATE_REQUESTED;
        g_bcm_state.telemetry.frames_requested++;
    }
}

void BCM_RequestWindowDamage(uint32_t window_id) {
    if (!g_bcm_state.initialized || window_id == 0) return;
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;

    BCM_RequestDamage(
        win->screen_bounds.x,
        win->screen_bounds.y,
        win->screen_bounds.width,
        win->screen_bounds.height
    );
}

void BCM_RequestCursorDamage(int32_t old_x, int32_t old_y, int32_t new_x, int32_t new_y) {
    BCM_RequestDamage(old_x, old_y, 32, 32);
    BCM_RequestDamage(new_x, new_y, 32, 32);
}

void BCM_RequestFullRepaint(void) {
    if (!g_bcm_state.initialized) return;
    if (g_bcm_state.is_presenting) {
        g_bcm_state.next_full_damage_requested = true;
        g_bcm_state.next_pending_damage = true;
    } else {
        g_bcm_state.full_damage_requested = true;
        g_bcm_state.pending_damage = true;
    }
    g_bcm_state.telemetry.total_damage_requests++;
    g_bcm_state.telemetry.full_repaint_count++;
    if (g_bcm_state.state == BCM_STATE_IDLE) {
        g_bcm_state.state = BCM_STATE_REQUESTED;
        g_bcm_state.telemetry.frames_requested++;
    }
}

void BCM_NotifyTimerTick(uint64_t tick_count) {
    if (!g_bcm_state.initialized) return;
    g_bcm_state.last_timer_tick = tick_count;

    if (g_bcm_state.pending_damage && g_bcm_state.state == BCM_STATE_IDLE) {
        g_bcm_state.state = BCM_STATE_REQUESTED;
    }
}

/* ========================================================================= */
/* Frame Pacing & Scheduling APIs (Phase 4, 6, 7)                            */
/* ========================================================================= */

bool BCM_FrameDeadlineReached(void) {
    if (!g_bcm_state.pending_damage && g_bcm_state.state == BCM_STATE_IDLE) {
        return false;
    }

    uint64_t now = timer_get_ticks();
    if (g_bcm_state.last_compose_tick == 0) {
        return true; /* First frame executes immediately */
    }

    uint64_t elapsed = (now >= g_bcm_state.last_compose_tick) ? (now - g_bcm_state.last_compose_tick) : 0;
    return (elapsed >= g_bcm_state.pacing_interval_ms);
}

/* ========================================================================= */
/* Phase 6 & Phase 7 Presentation & Synchronization Implementation           */
/* ========================================================================= */

bcm_error_t BCM_SchedulePresentation(uint64_t frame_id) {
    if (!g_bcm_state.initialized) return BCM_ERR_NOT_INITIALIZED;
    if (frame_id == 0) return BCM_ERR_INVALID_PARAM;

    if (g_bcm_state.state != BCM_STATE_COMPOSED && g_bcm_state.state != BCM_STATE_COMPOSING) {
        g_bcm_state.telemetry.presentation_failures++;
        return BCM_ERR_INVALID_STATE;
    }

    g_bcm_state.state = BCM_STATE_PRESENT_QUEUED;
    g_bcm_state.telemetry.presentation_requests++;
    return BCM_OK;
}

bcm_error_t BCM_BeginPresentation(uint64_t frame_id) {
    if (!g_bcm_state.initialized) return BCM_ERR_NOT_INITIALIZED;

    /* Context Firewall: Must run in Task Context (IF=1) */
    if (!bcm_is_interrupt_enabled()) {
        com1_puts("[BCM][SECURITY] PRESENTATION BLOCKED: IF=0 in BCM_BeginPresentation\r\n");
        g_bcm_state.telemetry.reentrancy_blocks++;
        return BCM_ERR_INVALID_STATE;
    }

    /* In-Flight Protection Guard */
    if (g_bcm_state.is_presenting || g_bcm_state.in_flight_frame_id != 0) {
        g_bcm_state.telemetry.dropped_presentations++;
        g_bcm_state.telemetry.reentrancy_blocks++;
        return BCM_ERR_BUSY;
    }

    g_bcm_state.is_presenting = true;
    g_bcm_state.in_flight_frame_id = frame_id;
    g_bcm_state.presentation_start_tick = timer_get_ticks();
    g_bcm_state.state = BCM_STATE_PRESENTING;
    g_bcm_state.telemetry.presentation_submissions++;
    g_bcm_state.telemetry.in_flight_frame_id = frame_id;
    g_bcm_state.telemetry.is_in_flight = true;

    return BCM_OK;
}

bcm_error_t BCM_CompletePresentation(uint64_t frame_id, bcm_error_t status) {
    if (!g_bcm_state.initialized) return BCM_ERR_NOT_INITIALIZED;

    if (g_bcm_state.in_flight_frame_id != frame_id || !g_bcm_state.is_presenting) {
        g_bcm_state.telemetry.presentation_failures++;
        return BCM_ERR_STALE_FRAME;
    }

    uint64_t t_end = timer_get_ticks();
    uint32_t pres_duration_ms = (uint32_t)(t_end >= g_bcm_state.presentation_start_tick ? (t_end - g_bcm_state.presentation_start_tick) : 0);
    uint32_t pres_duration_us = pres_duration_ms * 1000U;
    if (pres_duration_us == 0) pres_duration_us = 100U;

    g_bcm_state.telemetry.last_present_time_us = pres_duration_us;
    if (pres_duration_us > g_bcm_state.telemetry.max_present_time_us) {
        g_bcm_state.telemetry.max_present_time_us = pres_duration_us;
    }

    if (status == BCM_OK) {
        g_bcm_state.state = BCM_STATE_PRESENT_COMPLETE;
        g_bcm_state.last_completed_frame_id = frame_id;
        g_bcm_state.telemetry.last_completed_frame_id = frame_id;
        g_bcm_state.telemetry.presentation_completions++;
        g_bcm_state.telemetry.frames_presented++;
    } else {
        g_bcm_state.telemetry.presentation_failures++;
    }

    /* Frame Retired: Release in-flight lock */
    g_bcm_state.in_flight_frame_id = 0;
    g_bcm_state.is_presenting = false;
    g_bcm_state.state = BCM_STATE_PRESENTED;
    g_bcm_state.telemetry.in_flight_frame_id = 0;
    g_bcm_state.telemetry.is_in_flight = false;
    g_bcm_state.last_present_tick = t_end;

    /* Promote any next-frame damage buffered during presentation */
    BCM_Internal_PromoteNextFrameDamage();

    return status;
}

bool BCM_IsFrameInFlight(void) {
    return g_bcm_state.is_presenting || (g_bcm_state.in_flight_frame_id != 0);
}

uint64_t BCM_GetInFlightFrameID(void) {
    return g_bcm_state.in_flight_frame_id;
}

uint64_t BCM_GetCurrentFrameID(void) {
    return g_bcm_state.current_frame_id;
}

uint64_t BCM_GetLastCompletedFrameID(void) {
    return g_bcm_state.last_completed_frame_id;
}

bcm_error_t BCM_CheckPresentationTimeout(uint64_t timeout_ms) {
    if (!g_bcm_state.initialized) return BCM_ERR_NOT_INITIALIZED;
    if (!g_bcm_state.is_presenting || g_bcm_state.in_flight_frame_id == 0) return BCM_OK;

    uint64_t now = timer_get_ticks();
    uint64_t elapsed = (now >= g_bcm_state.presentation_start_tick) ? (now - g_bcm_state.presentation_start_tick) : 0;
    if (elapsed > timeout_ms) {
        com1_puts("[BCM_WARN] Presentation Timeout Detected! Initiating Controlled Recovery...\r\n");
        g_bcm_state.telemetry.presentation_timeouts++;

        /* Non-panicking recovery */
        g_bcm_state.is_presenting = false;
        g_bcm_state.in_flight_frame_id = 0;
        g_bcm_state.telemetry.in_flight_frame_id = 0;
        g_bcm_state.telemetry.is_in_flight = false;
        g_bcm_state.full_damage_requested = true;
        g_bcm_state.state = BCM_STATE_IDLE;

        return BCM_ERR_TIMEOUT;
    }

    return BCM_OK;
}

/* ========================================================================= */
/* Core Compositor Execution Pipeline (Phases 1-7 Unified)                   */
/* ========================================================================= */

bcm_error_t BCM_Process(void) {
    if (!g_bcm_state.initialized) return BCM_ERR_NOT_INITIALIZED;

    /* Execution Firewall: Must run strictly in Preemptible Task Context (IF=1) */
    if (!bcm_is_interrupt_enabled()) {
        com1_puts("[BCM][SECURITY] COMPOSITION BLOCKED: IF=0\r\n");
        g_bcm_state.telemetry.reentrancy_blocks++;
        return BCM_ERR_INVALID_STATE;
    }

    /* Timeout watchdog for previous presentation pass if any */
    BCM_CheckPresentationTimeout(g_bcm_state.presentation_timeout_ms);

    /* Re-entrancy Guard */
    if (g_bcm_state.is_composing || g_bcm_state.is_presenting) {
        g_bcm_state.telemetry.reentrancy_blocks++;
        return BCM_ERR_BUSY;
    }

    if (!g_bcm_state.pending_damage && g_bcm_state.state == BCM_STATE_IDLE) {
        return BCM_OK; /* Idle */
    }

    uint64_t t_start = timer_get_ticks();
    uint64_t frame_id = ++g_bcm_state.current_frame_id;
    g_bcm_state.telemetry.current_frame_id = frame_id;

    /* Transition state: REQUESTED -> SCHEDULED */
    g_bcm_state.state = BCM_STATE_SCHEDULED;
    g_bcm_state.telemetry.frames_scheduled++;

    /* Coalesce dirty regions before composition */
    BCM_Internal_CoalesceDamage();

    /* SCHEDULED -> COMPOSING */
    g_bcm_state.state = BCM_STATE_COMPOSING;
    g_bcm_state.is_composing = true;

    /* Execute real composition pass in Task Context (IF=1) */
    extern BVFramebuffer* vbe_get_framebuffer(void);
    extern void BWE_ComposeFrame(const BVFramebuffer* hw_fb);
    
    /* Pre-presentation scheduling gate */
    BCM_SchedulePresentation(frame_id);
    BCM_BeginPresentation(frame_id);

    BWE_ComposeFrame(vbe_get_framebuffer());

    /* COMPOSING -> COMPOSED */
    g_bcm_state.is_composing = false;
    g_bcm_state.telemetry.frames_composed++;

    /* Complete presentation and retire frame */
    BCM_CompletePresentation(frame_id, BCM_OK);

    uint64_t t_end = timer_get_ticks();
    uint32_t duration_ms = (uint32_t)(t_end >= t_start ? (t_end - t_start) : 0);
    uint32_t duration_us = duration_ms * 1000U;
    if (duration_us == 0) duration_us = 250U; /* Nominal sub-ms pass */

    g_bcm_state.telemetry.last_frame_duration_us = duration_us;
    g_bcm_state.telemetry.last_compose_time_us = duration_us;
    if (duration_us > g_bcm_state.telemetry.worst_frame_duration_us) {
        g_bcm_state.telemetry.worst_frame_duration_us = duration_us;
        g_bcm_state.telemetry.max_compose_time_us = duration_us;
    }

    /* Rolling average frame duration */
    if (g_bcm_state.telemetry.frames_composed > 1) {
        g_bcm_state.telemetry.average_frame_duration_us = (uint32_t)(
            ((uint64_t)g_bcm_state.telemetry.average_frame_duration_us * 7ULL + (uint64_t)duration_us) / 8ULL
        );
    } else {
        g_bcm_state.telemetry.average_frame_duration_us = duration_us;
    }

    /* Rolling 1-Second FPS Meter */
    g_bcm_state.last_compose_tick = t_end;
    if (g_bcm_state.fps_window_start_tick == 0) {
        g_bcm_state.fps_window_start_tick = t_end;
    }
    g_bcm_state.fps_window_frame_count++;
    if (t_end - g_bcm_state.fps_window_start_tick >= 1000) {
        g_bcm_state.telemetry.current_fps = g_bcm_state.fps_window_frame_count;
        g_bcm_state.fps_window_frame_count = 0;
        g_bcm_state.fps_window_start_tick = t_end;
    }

    return BCM_OK;
}

/* ========================================================================= */
/* Query & Telemetry APIs                                                    */
/* ========================================================================= */

BCM_FrameState BCM_GetState(void) {
    return g_bcm_state.state;
}

void BCM_GetTelemetry(BCM_Telemetry* out_telemetry) {
    if (out_telemetry) {
        g_bcm_state.telemetry.current_dirty_count = g_bcm_state.dirty_count;
        g_bcm_state.telemetry.current_state = g_bcm_state.state;
        g_bcm_state.telemetry.in_flight_frame_id = g_bcm_state.in_flight_frame_id;
        g_bcm_state.telemetry.is_in_flight = g_bcm_state.is_presenting;
        g_bcm_state.telemetry.last_completed_frame_id = g_bcm_state.last_completed_frame_id;
        g_bcm_state.telemetry.current_frame_id = g_bcm_state.current_frame_id;
        *out_telemetry = g_bcm_state.telemetry;
    }
}

bool BCM_HasPendingDamage(void) {
    return g_bcm_state.pending_damage || g_bcm_state.next_pending_damage || (g_bcm_state.state != BCM_STATE_IDLE);
}

uint32_t BCM_GetDirtyRectCount(void) {
    return g_bcm_state.dirty_count;
}

const BCM_Rect* BCM_GetDirtyRects(void) {
    return g_bcm_state.dirty_rects;
}

void BCM_SetPacingInterval(uint32_t interval_ms) {
    if (interval_ms < BCM_MIN_INTER_FRAME_GAP_MS) {
        interval_ms = BCM_MIN_INTER_FRAME_GAP_MS;
    }
    g_bcm_state.pacing_interval_ms = interval_ms;
}

void BCM_GetPacingMetrics(uint32_t* out_fps, uint32_t* out_frame_time_us, uint32_t* out_missed_deadlines) {
    if (out_fps) *out_fps = g_bcm_state.telemetry.current_fps;
    if (out_frame_time_us) *out_frame_time_us = g_bcm_state.telemetry.last_frame_duration_us;
    if (out_missed_deadlines) *out_missed_deadlines = (uint32_t)g_bcm_state.telemetry.missed_deadlines;
}
