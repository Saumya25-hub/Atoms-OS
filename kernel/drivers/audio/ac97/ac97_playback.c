#include "ac97_playback.h"
#include "ac97_dma.h"
#include "ac97_registers.h"
#include "../../../../arch/x86_64/io/port_io.h"
#include "../../../audio/audio_mixer.h"
#include "../../display/display.h"

static Ac97PlaybackState g_pb_state = AC97_PB_STATE_UNINITIALIZED;
static Ac97PlaybackTelemetry g_pb_telemetry = {0};

static uint8_t g_last_civ = 0;
static uint8_t g_lvi = 0;
static Ac97DmaManager* g_dma_mgr = NULL;
static uint16_t g_nabm_base = 0;
static bool g_civ_printed = false;

static const AudioPcmFormat g_ac97_format = {
    .format = PCM_FORMAT_S16_LE,
    .sample_rate = 48000,
    .channels = 2,
    .bit_depth = 16,
    .is_signed = true
};

bool ac97_playback_init(void) {
    g_dma_mgr = ac97_dma_get_manager();
    g_nabm_base = ac97_dma_get_nabm_bar();
    
    if (!g_dma_mgr || !g_nabm_base) {
        g_pb_state = AC97_PB_STATE_ERROR;
        return false;
    }
    
    g_pb_state = AC97_PB_STATE_UNINITIALIZED;
    display_print("[AC97 PLAYBACK] Initialized\n");
    return true;
}

void ac97_playback_shutdown(void) {
    ac97_playback_stop();
    ac97_dma_shutdown();
    g_pb_state = AC97_PB_STATE_UNINITIALIZED;
}

bool ac97_playback_prepare(void) {
    if (g_pb_state == AC97_PB_STATE_RUNNING) return false;
    
    // Create a 128KB DMA buffer. This is 32 chunks of 4KB = 21.3ms per descriptor.
    if (!ac97_dma_prepare(131072)) {
        g_pb_state = AC97_PB_STATE_ERROR;
        return false;
    }
    
    g_pb_state = AC97_PB_STATE_PREPARED;
    return true;
}

bool ac97_playback_start(void) {
    if (g_pb_state != AC97_PB_STATE_PREPARED && g_pb_state != AC97_PB_STATE_STOPPED) {
        return false;
    }
    
    g_pb_state = AC97_PB_STATE_STARTING;
    
    size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
    
    // Fill all DMA descriptors from mixer (which reads from ring buffer)
    for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
        uint8_t* target = g_dma_mgr->pcm_buffer + (i * chunk_bytes);
        audio_mixer_process(target, chunk_bytes, &g_ac97_format);
    }
    
    g_lvi = AC97_BDL_ENTRIES - 1;
    io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);
    
    g_last_civ = io_in8(g_nabm_base + AC97_NABM_PO_CIV);
    
    // Start DMA playback
    uint8_t cr = io_in8(g_nabm_base + AC97_NABM_PO_CR);
    cr |= 0x01; // RPBM
    io_out8(g_nabm_base + AC97_NABM_PO_CR, cr);
    
    g_pb_state = AC97_PB_STATE_RUNNING;
    
    display_print("[AC97 PLAYBACK] DMA Started\n");
    
    return true;
}

void ac97_playback_stop(void) {
    if (g_pb_state != AC97_PB_STATE_RUNNING) return;
    
    g_pb_state = AC97_PB_STATE_STOPPING;
    
    uint8_t cr = io_in8(g_nabm_base + AC97_NABM_PO_CR);
    cr &= ~0x01; // Clear RPBM
    io_out8(g_nabm_base + AC97_NABM_PO_CR, cr);
    
    uint32_t timeout = 10000;
    while (timeout--) {
        uint16_t sr = io_in16(g_nabm_base + AC97_NABM_PO_SR);
        if (sr & 0x02) { // DCH
            break;
        }
    }
    
    g_pb_state = AC97_PB_STATE_STOPPED;
    
    static bool printed_stop = false;
    if (!printed_stop) {
        display_print("[AC97 PLAYBACK] Playback Stopped\n");
        printed_stop = true;
    }
}

#define TRACE_MAX 2000
typedef struct {
    uint64_t timestamp;
    uint32_t bdbar;
    uint8_t civ;
    uint8_t lvi;
    uint16_t sr;
    uint8_t cr;
    uint16_t picb;
    uint32_t event_type; // 0=poll, 1=rotation, 2=lvi_update
    uint32_t latency;
} PlaybackTrace;

static PlaybackTrace g_traces[TRACE_MAX];
static uint32_t g_trace_count = 0;
extern uint64_t timer_get_ticks(void);
static uint64_t g_last_poll_time = 0;

// ===== Telemetry Counters (accumulated silently, dumped at end) =====
static uint32_t g_tel_total_polls = 0;
static uint32_t g_tel_rotations = 0;
static uint32_t g_tel_dch_halts = 0;
static uint32_t g_tel_max_poll_delta = 0;
static uint32_t g_tel_min_poll_delta = 0xFFFFFFFF;
static uint32_t g_tel_max_descriptors_behind = 0;
static uint32_t g_tel_stream_starve_count = 0;  // mixer got 0 bytes from stream
static uint64_t g_tel_total_mixer_bytes = 0;

void ac97_playback_update(void) {
    if (g_pb_state != AC97_PB_STATE_RUNNING) return;
    
    uint64_t now = timer_get_ticks();
    uint32_t poll_delta = (uint32_t)(g_last_poll_time == 0 ? 0 : now - g_last_poll_time);
    g_last_poll_time = now;
    
    g_tel_total_polls++;
    if (poll_delta > 0 && poll_delta > g_tel_max_poll_delta) g_tel_max_poll_delta = poll_delta;
    if (poll_delta > 0 && poll_delta < g_tel_min_poll_delta) g_tel_min_poll_delta = poll_delta;
    
    uint8_t civ = io_in8(g_nabm_base + 0x14);
    uint16_t sr = io_in16(g_nabm_base + 0x16);
    uint8_t cr = io_in8(g_nabm_base + 0x1B);
    
    // Clear status bits (write-1-to-clear: BCIS, LVBCI, FIFO ERR)
    if (sr & 0x1C) {
        io_out16(g_nabm_base + 0x16, sr & 0x1C);
    }
    
    // If DMA halted (DCH set), restart it
    if (sr & 0x02) {
        g_tel_dch_halts++;
        g_pb_telemetry.underruns++;
        
        // Refill ALL descriptors from mixer before restarting
        size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
        for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
            uint8_t* target = g_dma_mgr->pcm_buffer + (i * chunk_bytes);
            audio_mixer_process(target, chunk_bytes, &g_ac97_format);
            g_tel_total_mixer_bytes += chunk_bytes;
        }
        g_last_civ = 0;
        g_lvi = AC97_BDL_ENTRIES - 1;
        io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);
        cr |= 0x01; // RPBM — restart
        io_out8(g_nabm_base + 0x1B, cr);
        g_pb_telemetry.restarts++;
        return;
    }
    
    if (civ == g_last_civ) {
        return; // No rotation
    }
    
    // Count how many descriptors the hardware consumed since last poll
    uint32_t descriptors_consumed;
    if (civ >= g_last_civ) {
        descriptors_consumed = civ - g_last_civ;
    } else {
        descriptors_consumed = (AC97_BDL_ENTRIES - g_last_civ) + civ;
    }
    if (descriptors_consumed > g_tel_max_descriptors_behind) {
        g_tel_max_descriptors_behind = descriptors_consumed;
    }
    
    g_tel_rotations++;
    
    size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
    
    // Refill all consumed descriptors with fresh mixer data
    uint8_t idx = g_last_civ;
    while (idx != civ) {
        uint8_t* target = g_dma_mgr->pcm_buffer + (idx * chunk_bytes);
        audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        
        g_pb_telemetry.bytes_sent += chunk_bytes;
        g_pb_telemetry.frames_played += chunk_bytes / 4;
        g_tel_total_mixer_bytes += chunk_bytes;
        
        idx = (idx + 1) % AC97_BDL_ENTRIES;
    }
    
    g_last_civ = civ;
    
    // LVI strategy: set LVI to (CIV - 1) mod 32
    // Per Intel ICH spec: hardware plays CIV through LVI then halts.
    // (CIV-1) mod 32 means hardware has full 31 descriptors of runway before halt.
    g_lvi = (civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
    io_out8(g_nabm_base + 0x15, g_lvi);
}

void ac97_playback_dump_trace(void) {
    // Removed old trace dump — replaced by telemetry counters below
}

void ac97_playback_status(void) {
    display_print("\n========================================\n");
    display_print("  AC97 PLAYBACK TELEMETRY\n");
    display_print("========================================\n");
    display_print("Total Polls       : "); display_print_dec(g_tel_total_polls); display_print("\n");
    display_print("CIV Rotations     : "); display_print_dec(g_tel_rotations); display_print("\n");
    display_print("DCH Halts         : "); display_print_dec(g_tel_dch_halts); display_print("\n");
    display_print("Underruns         : "); display_print_dec(g_pb_telemetry.underruns); display_print("\n");
    display_print("Restarts          : "); display_print_dec(g_pb_telemetry.restarts); display_print("\n");
    display_print("Max Poll Gap (ms) : "); display_print_dec(g_tel_max_poll_delta); display_print("\n");
    display_print("Min Poll Gap (ms) : "); display_print_dec(g_tel_min_poll_delta); display_print("\n");
    display_print("Max Desc Behind   : "); display_print_dec(g_tel_max_descriptors_behind); display_print("\n");
    display_print("Frames Played     : "); display_print_dec(g_pb_telemetry.frames_played); display_print("\n");
    display_print("Bytes Sent        : "); display_print_dec(g_pb_telemetry.bytes_sent); display_print("\n");
    display_print("Mixer Bytes Total : "); display_print_dec(g_tel_total_mixer_bytes); display_print("\n");
    display_print("========================================\n");
}

void ac97_playback_run_stress_test(void) {
    display_print("\n[AC97 PLAYBACK] Starting Stress Test (10000 Iterations)...\n");
    for (int i = 0; i < 10000; i++) {
        ac97_playback_prepare();
        ac97_playback_start();
        
        for (int t = 0; t < 10; t++) {
            ac97_playback_update(); 
        }
        
        ac97_playback_stop();
        ac97_playback_shutdown();
    }
    display_print("[AC97 PLAYBACK] SUCCESS\n");
}
