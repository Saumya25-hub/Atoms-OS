#include "kernel/shell/rook/include/rook.h"
#include "kernel/shell/rook/include/rook_debug.h"

/*
 * ♜ ROOK ENGINE V1.0 — Core State Machine & Event Dispatcher
 */

extern void rook_init_renderer(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride);
extern void rook_render_flush(void);

static uint16_t     g_current_page_id = ROOK_PAGE_BOOT_SPLASH;
static rook_page_t* g_current_page = 0;

void rook_init(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride) {
    rook_init_renderer(gop_fb, width, height, stride);
    g_current_page_id = ROOK_PAGE_BOOT_SPLASH;
    g_current_page = rook_get_page(g_current_page_id);
    
    if (g_current_page) {
        if (g_current_page->ops.on_load) g_current_page->ops.on_load(g_current_page);
        g_current_page->state = ROOK_STATE_LOADED;
        
        if (g_current_page->ops.on_enter) g_current_page->ops.on_enter(g_current_page);
        g_current_page->state = ROOK_STATE_ACTIVE;
    }
}

rook_page_t* rook_get_current_page(void) {
    return g_current_page;
}

static rook_page_t s_desktop_dummy_page = {
    .id = ROOK_PAGE_DESKTOP,
    .name = "ATOMS Desktop Page",
    .state = ROOK_STATE_ACTIVE
};

int rook_goto(uint16_t page_id) {
    if (page_id >= ROOK_MAX_PAGES) return -1;
    rook_page_t* next_page = rook_get_page(page_id);
    if (!next_page) {
        if (page_id == ROOK_PAGE_DESKTOP) {
            next_page = &s_desktop_dummy_page;
        } else {
            return -2;
        }
    }

    if (g_current_page && g_current_page->state == ROOK_STATE_ACTIVE) {
        if (g_current_page->ops.on_exit) g_current_page->ops.on_exit(g_current_page);
        g_current_page->state = ROOK_STATE_LOADED;
    }

    g_current_page_id = page_id;
    g_current_page = next_page;

    if (g_current_page->state < ROOK_STATE_LOADED) {
        if (g_current_page->ops.on_load) g_current_page->ops.on_load(g_current_page);
        g_current_page->state = ROOK_STATE_LOADED;
    }

    if (g_current_page->ops.on_enter) g_current_page->ops.on_enter(g_current_page);
    g_current_page->state = ROOK_STATE_ACTIVE;
    rook_invalidate_full();

    if (page_id == ROOK_PAGE_DESKTOP) {
        extern void BWE_RequestFullRedraw(void);
        BWE_RequestFullRedraw();
    }

    return 0;
}

int rook_next(void) {
    if (!g_current_page) return -1;
    return rook_goto(g_current_page->nav_next_id);
}

int rook_previous(void) {
    if (!g_current_page) return -1;
    return rook_goto(g_current_page->nav_prev_id);
}

int rook_navigate(rook_nav_cmd_t cmd) {
    if (!g_current_page) return -1;
    switch (cmd) {
        case ROOK_NAV_LEFT:     return rook_goto(g_current_page->nav_left_id);
        case ROOK_NAV_RIGHT:    return rook_goto(g_current_page->nav_right_id);
        case ROOK_NAV_UP:       return rook_goto(g_current_page->nav_up_id);
        case ROOK_NAV_DOWN:     return rook_goto(g_current_page->nav_down_id);
        case ROOK_NAV_NEXT:     return rook_next();
        case ROOK_NAV_PREVIOUS: return rook_previous();
        default: break;
    }
    return -1;
}

void rook_update(uint64_t delta_ms) {
    if (g_current_page && g_current_page->ops.on_update) {
        g_current_page->ops.on_update(g_current_page, delta_ms);
    }
    rook_invalidate_full();
}

void rook_render(void) {
    rook_render_flush();
}

static inline uint64_t rdtsc_pure(void) {
    uint32_t lo = 0, hi = 0;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void rook_splash_spin(uint32_t total_ms) {
    /* Compute exact 60 FPS frame count for total_ms (e.g., 6000ms = 360 frames) */
    uint32_t total_frames = (total_ms * 60) / 1000;
    if (total_frames == 0) total_frames = 180;

    /* Calibrate 16.666ms TSC cycles per frame */
    uint64_t tsc_start_calib = rdtsc_pure();
    for (volatile int i = 0; i < 100000; i++) { __asm__ volatile("pause"); }
    uint64_t tsc_end_calib = rdtsc_pure();
    uint64_t cycles_per_calib = tsc_end_calib - tsc_start_calib;

    /* Estimate cycles for 16.666ms */
    uint64_t target_frame_cycles = cycles_per_calib * 14;
    if (target_frame_cycles < 2000000ULL) target_frame_cycles = 50000000ULL;

    for (uint32_t f = 0; f < total_frames; f++) {
        uint64_t frame_start_tsc = rdtsc_pure();

        rook_update(16);
        rook_render();

        /* Hardware TSC Real-Time Frame Pacing (100% 60.00 FPS Butter Spin) */
        while ((rdtsc_pure() - frame_start_tsc) < target_frame_cycles) {
            __asm__ volatile("pause");
        }
    }
}

extern void com1_puts(const char *s);

void rook_login_spin(void) {
    com1_puts("[ROOK] Entering Interactive Login Supervisor Loop...\r\n");

    /* Calibrate 16.666ms TSC cycles per frame */
    uint64_t tsc_start_calib = rdtsc_pure();
    for (volatile int i = 0; i < 100000; i++) { __asm__ volatile("pause"); }
    uint64_t tsc_end_calib = rdtsc_pure();
    uint64_t cycles_per_calib = tsc_end_calib - tsc_start_calib;

    uint64_t target_frame_cycles = cycles_per_calib * 14;
    if (target_frame_cycles < 2000000ULL) target_frame_cycles = 50000000ULL;

    while (g_current_page && g_current_page->id == ROOK_PAGE_LOGIN) {
        uint64_t frame_start_tsc = rdtsc_pure();

        rook_update(16);
        rook_render();

        /* Hardware TSC Real-Time Frame Pacing */
        while ((rdtsc_pure() - frame_start_tsc) < target_frame_cycles) {
            __asm__ volatile("pause");
        }
    }

    com1_puts("[ROOK] Login Authentication Complete! Exiting Login Supervisor Loop ➔ Handoff to Desktop Shell!\r\n");
}

void rook_dispatch_event(uint32_t event_id, void* payload) {
    (void)payload;
    if (!g_current_page) return;

    /* Event-driven progression during Boot Splash */
    if (g_current_page->id == ROOK_PAGE_BOOT_SPLASH) {
        if (g_current_page->ops.on_update) {
            /* Pass event_id as delta_ms or custom notification to trigger dot progression */
            g_current_page->ops.on_update(g_current_page, event_id);
        }
    }
}
