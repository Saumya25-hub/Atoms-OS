#include "master_clock.h"
#include "kernel/core/lib/include/string.h"

static bospectra_clock_source_t g_clock_source = BOSPECTRA_CLOCK_SOURCE_AUDIO;
static uint64_t g_audio_master_pts_us = 0;
static uint64_t g_clock_base_ticks = 0;
static uint64_t g_clock_base_pts_us = 0;
static uint32_t g_playback_speed_x100 = 100; // 1.0x
static bool     g_master_clock_initialized = false;

extern uint64_t timer_get_ticks(void);

void master_clock_init(void) {
    g_clock_source = BOSPECTRA_CLOCK_SOURCE_AUDIO;
    g_audio_master_pts_us = 0;
    g_clock_base_ticks = timer_get_ticks();
    g_clock_base_pts_us = 0;
    g_playback_speed_x100 = 100;
    g_master_clock_initialized = true;
}

void master_clock_shutdown(void) {
    g_master_clock_initialized = false;
}

void master_clock_set_source(bospectra_clock_source_t source) {
    g_clock_source = source;
}

bospectra_clock_source_t master_clock_get_source(void) {
    return g_clock_source;
}

void master_clock_update_audio_pts(uint64_t audio_pts_us) {
    g_audio_master_pts_us = audio_pts_us;
}

uint64_t master_clock_get_time_us(void) {
    if (!g_master_clock_initialized) return 0;

    if (g_clock_source == BOSPECTRA_CLOCK_SOURCE_AUDIO) {
        return (g_audio_master_pts_us * g_playback_speed_x100) / 100;
    }

    /* Monotonic Wall-Clock System Timer (1000 Hz IRQ0 ticks -> us) */
    uint64_t now_ticks = timer_get_ticks();
    uint64_t elapsed_us = (now_ticks >= g_clock_base_ticks) ?
        (now_ticks - g_clock_base_ticks) * 1000ULL : 0;
    return g_clock_base_pts_us + (elapsed_us * g_playback_speed_x100) / 100;
}

void master_clock_set_speed(uint32_t speed_x100) {
    if (speed_x100 > 0 && speed_x100 <= 400) { // Max 4.0x
        g_playback_speed_x100 = speed_x100;
    }
}

uint32_t master_clock_get_speed(void) {
    return g_playback_speed_x100;
}

void master_clock_reset(uint64_t start_pts_us) {
    g_audio_master_pts_us = start_pts_us;
    g_clock_base_ticks = timer_get_ticks();
    g_clock_base_pts_us = start_pts_us;
}

void master_clock_seek(uint64_t target_pts_us) {
    master_clock_reset(target_pts_us);
}
