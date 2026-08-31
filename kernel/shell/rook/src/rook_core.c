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

    /* Phase 3 Atomic Surface Zero-Wipe Protocol: solid pure black #000000 (skip for Desktop to keep Wallpaper + Loading) */
    if (page_id != ROOK_PAGE_DESKTOP) {
        rook_surface_t* main_surf = rook_get_surface();
        if (main_surf) {
            rook_surface_clear(main_surf, 0xFF000000);
        }
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
}

void rook_render(void) {
    rook_render_flush();
}

#include "arch/x86_64/io/port_io.h"

static inline uint64_t rdtsc_pure(void) {
    uint32_t lo = 0, hi = 0;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t s_tsc_ticks_per_ms = 0;

uint64_t rook_get_tsc_per_ms(void) {
    if (s_tsc_ticks_per_ms != 0) {
        return s_tsc_ticks_per_ms;
    }

    /*
     * Hardware PIT Channel 2 (Port 0x61 / Port 0x42 / Port 0x43) Calibration.
     * Hardware PIT crystal frequency = 1,193,182 Hz across all x86 PC chipsets.
     * Count for 10.0 ms = 1193182 / 100 = 11932 ticks.
     */
    uint8_t prev_port61 = io_in8(0x61);

    // Gate PIT Channel 2: bit 0 high (enable clock gate), bit 1 low (speaker off)
    io_out8(0x61, (prev_port61 & ~0x02) | 0x01);

    // Set PIT Channel 2: Mode 0 (interrupt on terminal count), binary, 16-bit
    io_out8(0x43, 0xB0);

    uint16_t pit_count = 11932; // 10.0 ms interval
    io_out8(0x42, (uint8_t)(pit_count & 0xFF));
    io_out8(0x42, (uint8_t)((pit_count >> 8) & 0xFF));

    // Read start TSC
    uint64_t tsc_start = rdtsc_pure();

    // Wait until PIT Channel 2 output (Port 0x61, bit 5) transitions to high
    uint32_t timeout = 5000000;
    while ((io_in8(0x61) & 0x20) == 0 && --timeout > 0) {
        __asm__ volatile("pause");
    }

    uint64_t tsc_end = rdtsc_pure();

    // Restore Port 0x61
    io_out8(0x61, prev_port61);

    if (timeout > 0 && tsc_end > tsc_start) {
        uint64_t cycles_for_10ms = tsc_end - tsc_start;
        s_tsc_ticks_per_ms = cycles_for_10ms / 10ULL;
    }

    // Sanity range check: 500 MHz to 8.0 GHz (500,000 to 8,000,000 cycles/ms)
    // Fallback if PIT Channel 2 not responding: 3.4 GHz (3,400,000 cycles/ms)
    if (s_tsc_ticks_per_ms < 500000ULL || s_tsc_ticks_per_ms > 8000000ULL) {
        s_tsc_ticks_per_ms = 3400000ULL;
    }

    return s_tsc_ticks_per_ms;
}

extern void com1_puts(const char *s);
#include "kernel/ame/include/ame.h"

static void log_boot_anim_telemetry(uint32_t frame, uint32_t angle_deg) {
    char buf[96];
    char* p = buf;
    const char* p1 = "[BOOT_ANIM] frame=";
    while (*p1) *p++ = *p1++;

    char num[12]; int nidx = 0; uint32_t v = frame;
    if (v == 0) *p++ = '0';
    else { while (v > 0) { num[nidx++] = '0' + (v % 10); v /= 10; } while (nidx > 0) *p++ = num[--nidx]; }

    const char* p2 = " angle=";
    while (*p2) *p++ = *p2++;

    v = angle_deg; nidx = 0;
    if (v == 0) *p++ = '0';
    else { while (v > 0) { num[nidx++] = '0' + (v % 10); v /= 10; } while (nidx > 0) *p++ = num[--nidx]; }

    const char* p3 = " render=";
    while (*p3) *p++ = *p3++;

    v = frame + 1; nidx = 0;
    while (v > 0) { num[nidx++] = '0' + (v % 10); v /= 10; } while (nidx > 0) *p++ = num[--nidx];

    const char* p4 = " invalidate=1 present=1\r\n";
    while (*p4) *p++ = *p4++;
    *p = '\0';

    com1_puts(buf);
}

void rook_splash_spin(uint32_t total_ms) {
    /* Compute exact 60 FPS frame count for total_ms (e.g., 3000ms = 180 frames) */
    uint32_t total_frames = (total_ms * 60) / 1000;
    if (total_frames == 0) total_frames = 60;

    /* Hardware-calibrated 16.666ms TSC cycles per frame */
    uint64_t tsc_per_ms = rook_get_tsc_per_ms();
    uint64_t target_frame_cycles = (tsc_per_ms * 1000) / 60;

    for (uint32_t f = 0; f < total_frames; f++) {
        uint64_t frame_start_tsc = rdtsc_pure();

        rook_update(16);
        rook_render();

        /* Forensic Telemetry every 15 frames (~4 times per sec at 60 FPS) */
        if (f % 15 == 0 || f == total_frames - 1) {
            AME_Spinner* sp = AME_GetBootSpinner();
            uint32_t cur_angle = (sp ? ((sp->base_angle / 256) % 360) : 0);
            log_boot_anim_telemetry(f, cur_angle);
        }

        /* Hardware TSC Real-Time Frame Pacing (100% True 60.00 FPS Butter Spin) */
        while ((rdtsc_pure() - frame_start_tsc) < target_frame_cycles) {
            __asm__ volatile("pause");
        }
    }
}

void rook_login_spin(void) {
    com1_puts("[ROOK] Entering Interactive Login Supervisor Loop...\r\n");

    /* Hardware-calibrated 60 FPS Frame Pacing (~16.6ms) */
    uint64_t tsc_per_ms = rook_get_tsc_per_ms();
    uint64_t target_frame_cycles = (tsc_per_ms * 1000) / 60;

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

void rook_shutdown_spin(bool is_restart) {
    com1_puts("[ROOK] Entering Graceful Power Transition Supervisor...\r\n");

    extern void rook_page_shutdown_set_mode(bool is_restart);
    rook_page_shutdown_set_mode(is_restart);

    rook_goto(ROOK_PAGE_SHUTDOWN);

    /* Quiesce inputs */
    extern volatile bool g_system_power_transitioning;
    g_system_power_transitioning = true;

    /* Hardware-calibrated 60 FPS Frame Pacing (~16.6ms) */
    uint64_t tsc_per_ms = rook_get_tsc_per_ms();
    uint64_t target_frame_cycles = (tsc_per_ms * 1000) / 60;

    /* 60 frames = 1000 ms of smooth transition and graceful teardown */
    uint32_t total_frames = 60;
    for (uint32_t f = 0; f < total_frames; f++) {
        uint64_t frame_start_tsc = rdtsc_pure();

        /* 1. Poll Hardware USB Host Controllers & Input */
        extern void xhci_poll(void);
        xhci_poll();

        extern void vmmouse_poll(void);
        vmmouse_poll();

        extern void input_core_dispatch_events(void);
        input_core_dispatch_events();

        /* 2. Orderly Service Teardown at specific stage milestones */
        if (f == 15) {
            /* Mute and shut down Audio DMA cleanly */
            extern void audio_hal_shutdown(void);
            audio_hal_shutdown();
            com1_puts("[POWER_TEARDOWN] Audio HAL Quiesced.\r\n");
        }

        /* 3. Render frame */
        rook_update(16);
        rook_render();

        /* 4. Hardware TSC Real-Time Frame Pacing with smooth cursor updating */
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

    com1_puts("[ROOK] Visual Power Transition Complete. Executing Hardware Power Action...\r\n");

    if (is_restart) {
        extern void system_reboot(void);
        system_reboot();
    } else {
        extern void system_shutdown(void);
        system_shutdown();
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
