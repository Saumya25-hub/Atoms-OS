#include "kernel/audio/diagnostics/audio_test_mode.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/session/audio_player.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/hal/audio_hal.h"
#include "kernel/audio/core/audio_realtime_worker.h"
#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "kernel/drivers/display/display.h"
#include "arch/x86_64/io/port_io.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================
// Global Audio Test Mode Telemetry State
// ============================================================
static uint32_t g_atm_tick_count = 0;
static uint32_t g_atm_update_calls = 0;
static uint32_t g_atm_update_failures = 0;
static bool     g_atm_playback_started = false;

// ============================================================
// Helper: Print hex inline
// ============================================================
static void atm_print_hex(uint32_t val) {
    display_print("0x");
    display_print_hex((uint64_t)val);
}

// ============================================================
// audio_test_mode_entry()
// Called from kernel_main AFTER all essential subsystems are up.
// Performs audio init and starts DEMO1.WAV with full telemetry.
// ============================================================
void audio_test_mode_entry(void) {
    display_print("\n");
    display_print("========================================================\n");
    display_print("  AUDIO TEST MODE - ISOLATED AUDIO DEBUGGING\n");
    display_print("  All GUI/Desktop/Shell services DISABLED\n");
    display_print("========================================================\n\n");

    // Step 1: Init core audio
    display_print("[ATM-1] audio_init()...\n");
    audio_init();
    display_print("[ATM-1] DONE\n");

    // Step 2: Init mixer
    display_print("[ATM-2] audio_mixer_init()...\n");
    audio_mixer_init();
    display_print("[ATM-2] DONE\n");

    // Step 3: Init HAL (registers AC97 driver, probes PCI)
    display_print("[ATM-3] audio_hal_init()...\n");
    audio_hal_init();

    // Verify HAL driver was found
    extern audio_hal_driver_t* audio_hal_get_active_driver(void);
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (!drv) {
        display_print("[ATM-3] FATAL: audio_hal_get_active_driver() returned NULL!\n");
        display_print("[ATM] No AC97 hardware found. Cannot continue.\n");
        display_print("[ATM] System Halted.\n");
        while (1) __asm__ volatile("cli; hlt");
    }
    display_print("[ATM-3] Active driver: ");
    display_print(drv->name);
    display_print("\n");

    // Step 4: Open WAV file
    display_print("\n[ATM-4] audio_player_open(\"/DEMO1.WAV\")...\n");
    audio_player_open("/DEMO1.WAV");

    // Verify open succeeded
    uint32_t sid = 0, bp = 0, ds = 0;
    uint64_t rtt = 0;
    audio_player_get_diag_info(&sid, &bp, &ds, &rtt);

    if (sid == 0) {
        display_print("[ATM-4] FATAL: audio_player_open failed! stream_id=0\n");
        display_print("[ATM] Possible causes:\n");
        display_print("  - /DEMO1.WAV not found on FAT32 partition\n");
        display_print("  - WAV format not 48kHz 16-bit stereo PCM\n");
        display_print("  - VFS mount failed\n");
        display_print("[ATM] System Halted.\n");
        while (1) __asm__ volatile("cli; hlt");
    }

    display_print("[ATM-4] stream_id = ");
    display_print_dec(sid);
    display_print(", data_size = ");
    display_print_dec(ds);
    display_print(" bytes\n");

    // Step 5: Start playback (prefill + DMA start)
    display_print("\n[ATM-5] audio_player_play()...\n");
    audio_player_play();

    if (!audio_player_is_playing()) {
        display_print("[ATM-5] FATAL: audio_player_play() did not transition to PLAYING!\n");
        display_print("[ATM] System Halted.\n");
        while (1) __asm__ volatile("cli; hlt");
    }

    display_print("[ATM-5] Playback STARTED successfully!\n");
    g_atm_playback_started = true;

    // Print initial pipeline state
    display_print("\n========================================================\n");
    display_print("  AUDIO PIPELINE ACTIVE — Continuous Telemetry Starting\n");
    display_print("========================================================\n\n");
}

// ============================================================
// audio_test_mode_telemetry_tick()
// Called every iteration of the AudioSvc background loop.
// Prints detailed pipeline state every 50 calls (roughly 1/sec).
// ============================================================
static void atm_serial_write_str(const char* str) {
    extern void io_out8(uint16_t port, uint8_t data);
    extern uint8_t io_in8(uint16_t port);
    for (int i = 0; str[i] != '\0'; i++) {
        while ((io_in8(0x3F8 + 5) & 0x20) == 0);
        io_out8(0x3F8, str[i]);
    }
}
static void atm_serial_write_dec(uint64_t val) {
    char buf[32];
    int i = 0;
    if (val == 0) { atm_serial_write_str("0"); return; }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        char c[2] = { buf[--i], '\0' };
        atm_serial_write_str(c);
    }
}
static void atm_serial_write_hex(uint32_t val) {
    const char* hex_chars = "0123456789ABCDEF";
    atm_serial_write_str("0x");
    for (int i = (sizeof(val) * 2) - 1; i >= 0; i--) {
        char c[2] = { hex_chars[(val >> (i * 4)) & 0xF], '\0' };
        atm_serial_write_str(c);
    }
}

void audio_test_mode_telemetry_tick(void) {
    g_atm_tick_count++;
    g_atm_update_calls++;

    // Print every 5 ticks (~100ms at 20ms sleep)
    if (g_atm_tick_count < 5) return;
    g_atm_tick_count = 0;

    extern uint64_t timer_get_ticks(void);
    uint64_t now = timer_get_ticks();

    atm_serial_write_str("\n--- ATM TELEMETRY @ tick ");
    atm_serial_write_dec(now);
    atm_serial_write_str(" ---\n");

    // === PLAYER STATE ===
    uint32_t sid = 0, bytes_played = 0, data_size = 0;
    uint64_t read_time = 0;
    audio_player_get_diag_info(&sid, &bytes_played, &data_size, &read_time);

    atm_serial_write_str("PLAYER: played=");
    atm_serial_write_dec(bytes_played);
    atm_serial_write_str("/");
    atm_serial_write_dec(data_size);
    atm_serial_write_str(" read_time=");
    atm_serial_write_dec(read_time);
    atm_serial_write_str("ms playing=");
    atm_serial_write_str(audio_player_is_playing() ? "YES" : "NO");
    atm_serial_write_str("\n");

    // === STREAM / RING BUFFER STATE ===
    if (sid > 0) {
        size_t avail = audio_stream_available(sid);
        size_t cap   = audio_stream_capacity(sid);
        size_t free_b = (cap > avail) ? (cap - avail) : 0;
        uint32_t occ = (cap > 0) ? (uint32_t)((avail * 100) / cap) : 0;

        atm_serial_write_str("STREAM: avail=");
        atm_serial_write_dec(avail);
        atm_serial_write_str(" free=");
        atm_serial_write_dec(free_b);
        atm_serial_write_str(" cap=");
        atm_serial_write_dec(cap);
        atm_serial_write_str(" occ=");
        atm_serial_write_dec(occ);
        atm_serial_write_str("%\n");
    }

    // === MIXER STATE ===
    uint64_t mix_calls = 0, mix_req = 0, mix_ret = 0, mix_silence = 0;
    extern void audio_mixer_get_diag_counters(uint64_t*, uint64_t*, uint64_t*, uint64_t*);
    audio_mixer_get_diag_counters(&mix_calls, &mix_req, &mix_ret, &mix_silence);
    uint32_t mix_streams = 0;
    uint64_t frames_mixed = 0, clips = 0;
    int32_t peak = 0;
    extern void audio_mixer_get_stats(uint32_t*, uint64_t*, uint64_t*, int32_t*);
    audio_mixer_get_stats(&mix_streams, &frames_mixed, &clips, &peak);

    atm_serial_write_str("MIXER: calls=");
    atm_serial_write_dec(mix_calls);
    atm_serial_write_str(" req=");
    atm_serial_write_dec(mix_req);
    atm_serial_write_str(" ret=");
    atm_serial_write_dec(mix_ret);
    atm_serial_write_str(" silence=");
    atm_serial_write_dec(mix_silence);
    atm_serial_write_str(" streams=");
    atm_serial_write_dec(mix_streams);
    atm_serial_write_str("\n");

    // === DMA/AC97 STATE (Read hardware registers directly) ===
    extern uint16_t ac97_dma_get_nabm_bar(void);
    uint16_t nabm = ac97_dma_get_nabm_bar();
    if (nabm) {
        uint8_t  civ  = io_in8(nabm + 0x14);  // PO CIV
        uint8_t  lvi  = io_in8(nabm + 0x15);  // PO LVI
        uint16_t sr   = io_in16(nabm + 0x16);  // PO SR
        uint16_t picb = io_in16(nabm + 0x18);  // PO PICB
        uint8_t  cr   = io_in8(nabm + 0x1B);   // PO CR

        atm_serial_write_str("DMA: CIV=");
        atm_serial_write_dec(civ);
        atm_serial_write_str(" LVI=");
        atm_serial_write_dec(lvi);
        atm_serial_write_str(" PICB=");
        atm_serial_write_dec(picb);
        atm_serial_write_str(" SR=");
        atm_serial_write_hex(sr);
        atm_serial_write_str(" CR=");
        atm_serial_write_hex(cr);

        // Decode status bits
        if (sr & 0x02) atm_serial_write_str(" [DCH]");
        if (sr & 0x01) atm_serial_write_str(" [DMAEN]");
        if (sr & 0x08) atm_serial_write_str(" [BCIS]");
        if (sr & 0x10) atm_serial_write_str(" [FIFOE]");
        atm_serial_write_str("\n");
    }

    // === REALTIME WORKER STATE ===
    AudioRealtimeTelemetry rt = {0};
    audio_realtime_worker_get_telemetry(&rt);

    atm_serial_write_str("RT_WORKER: pumps=");
    atm_serial_write_dec(rt.total_pumps);
    atm_serial_write_str(" last=");
    atm_serial_write_dec(rt.last_pump_duration_us);
    atm_serial_write_str("us max=");
    atm_serial_write_dec(rt.max_pump_duration_us);
    atm_serial_write_str("us violations=");
    atm_serial_write_dec(rt.timing_violations);
    atm_serial_write_str("\n");

    // === AC97 PLAYBACK COUNTERS ===
    uint32_t rotations = ac97_playback_get_rotations();
    uint32_t dch_halts = ac97_playback_get_dch_halts();
    uint32_t underruns = ac97_playback_get_underruns();
    uint32_t bytes_sent = ac97_playback_get_bytes_sent();
    uint32_t frames_played = ac97_playback_get_frames_played();
    uint32_t late = ac97_playback_get_late_refills();

    atm_serial_write_str("AC97: rot=");
    atm_serial_write_dec(rotations);
    atm_serial_write_str(" dch=");
    atm_serial_write_dec(dch_halts);
    atm_serial_write_str(" under=");
    atm_serial_write_dec(underruns);
    atm_serial_write_str(" sent=");
    atm_serial_write_dec(bytes_sent);
    atm_serial_write_str(" frames=");
    atm_serial_write_dec(frames_played);
    atm_serial_write_str(" late=");
    atm_serial_write_dec(late);
    atm_serial_write_str("\n");

    // === HEALTH CHECK ===
    if (dch_halts > 0 && rotations == 0) {
        atm_serial_write_str("!!! WARNING: DMA halted but NO rotations — DMA never ran!\n");
    }
    if (mix_silence > 0 && mix_silence == mix_ret) {
        atm_serial_write_str("!!! WARNING: Mixer producing 100% silence — ring buffer empty!\n");
    }
    if (bytes_played == 0 && g_atm_update_calls > 100) {
        atm_serial_write_str("!!! WARNING: bytes_played stuck at 0 — VFS reads failing!\n");
    }
    if (rt.total_pumps == 0 && g_atm_update_calls > 100) {
        atm_serial_write_str("!!! WARNING: RT worker never pumped — timer IRQ not firing!\n");
    }
}
