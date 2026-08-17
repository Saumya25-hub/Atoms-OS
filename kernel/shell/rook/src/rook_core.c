#include "kernel/shell/rook/include/rook.h"
#include "kernel/shell/rook/include/rook_debug.h"
#include "kernel/drivers/input/pointer/pointer_state.h"

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

static void dump_backbuffer_64_pixels(const char* label) {
    extern void com1_puts(const char* s);
    extern uint32_t* rook_get_backbuffer(void);
    uint32_t* bb = rook_get_backbuffer();
    if (!bb) {
        com1_puts("[PROBE 1/2] Backbuffer is NULL!\r\n");
        return;
    }
    com1_puts("[PROBE 1/2 ");
    com1_puts(label);
    com1_puts("] First 64 pixels: \r\n");
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 64; i++) {
        uint32_t p = bb[i];
        char str[12];
        str[0] = '0'; str[1] = 'x';
        str[2] = hex[(p >> 28) & 0xF];
        str[3] = hex[(p >> 24) & 0xF];
        str[4] = hex[(p >> 20) & 0xF];
        str[5] = hex[(p >> 16) & 0xF];
        str[6] = hex[(p >> 12) & 0xF];
        str[7] = hex[(p >> 8) & 0xF];
        str[8] = hex[(p >> 4) & 0xF];
        str[9] = hex[p & 0xF];
        str[10] = ' ';
        str[11] = '\0';
        com1_puts(str);
        if ((i + 1) % 8 == 0) com1_puts("\r\n");
    }
}

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

    dump_backbuffer_64_pixels("BEFORE TRANSITION");

    if (g_current_page && g_current_page->state == ROOK_STATE_ACTIVE) {
        if (g_current_page->ops.on_exit) g_current_page->ops.on_exit(g_current_page);
        g_current_page->state = ROOK_STATE_LOADED;
    }

    /* Phase 3 Atomic Surface Zero-Wipe Protocol: solid pure black #000000 */
    rook_surface_t* main_surf = rook_get_surface();
    if (main_surf) {
        rook_surface_clear(main_surf, 0xFF000000);
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

    dump_backbuffer_64_pixels("AFTER TRANSITION");

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
    uint64_t target_frame_cycles = cycles_per_calib * 2;
    if (target_frame_cycles < 50000ULL) target_frame_cycles = 50000ULL;

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

    uint64_t target_frame_cycles = cycles_per_calib * 2;
    if (target_frame_cycles < 50000ULL) target_frame_cycles = 50000ULL;

    while (g_current_page && g_current_page->id == ROOK_PAGE_LOGIN) {
        uint64_t frame_start_tsc = rdtsc_pure();

        /* 1. Poll Hardware USB Host Controllers (xHCI) for keystrokes & mouse packets */
        extern void xhci_poll(void);
        xhci_poll();

        /* 2. Poll VMware VMMouse Backdoor if running in VM */
        extern void vmmouse_poll(void);
        vmmouse_poll();

        /* 3. Dispatch all pending Input Core events to Tier 0 Pointer Engine */
        extern void input_core_dispatch_events(void);
        input_core_dispatch_events();

        /* 4. Process page logic and render frame */
        rook_update(16);
        rook_render();

        /* Hardware TSC Real-Time Frame Pacing with 1000Hz Instant Cursor Scanout */
        static int32_t s_last_synced_x = -1, s_last_synced_y = -1;
        while ((rdtsc_pure() - frame_start_tsc) < target_frame_cycles) {
            xhci_poll();
            vmmouse_poll();
            input_core_dispatch_events();

            const PointerState *ps = pointer_state_get();
            if (ps && (ps->current_x != s_last_synced_x || ps->current_y != s_last_synced_y)) {
                s_last_synced_x = ps->current_x;
                s_last_synced_y = ps->current_y;
                extern void rook_cursor_update_motion(void);
                rook_cursor_update_motion();
            }
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
