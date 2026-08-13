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

void rook_splash_spin(uint32_t total_ms) {
    /* Compute exact 60 FPS frame count for total_ms (e.g., 6000ms = 360 frames) */
    uint32_t total_frames = (total_ms * 60) / 1000;
    if (total_frames == 0) total_frames = 180;

    for (uint32_t f = 0; f < total_frames; f++) {
        rook_update(16);
        rook_render();

        /* Calibrated 60 FPS frame pacing delay */
        for (volatile int i = 0; i < 150000; i++) {
            __asm__ volatile("pause");
        }
    }
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
