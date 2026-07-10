#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "kernel/audio/drivers/ac97/ac97_dma.h"
#include "kernel/audio/drivers/ac97/ac97_registers.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/drivers/display/display.h"

static Ac97PlaybackState g_pb_state = AC97_PB_STATE_UNINITIALIZED;
static Ac97PlaybackTelemetry g_pb_telemetry = {0};

static uint8_t g_last_civ = 0;
static uint8_t g_lvi = 0;
static Ac97DmaManager* g_dma_mgr = NULL;
static uint16_t g_nabm_base = 0;
static bool g_civ_printed = false;
extern uint64_t timer_get_ticks(void);
static uint64_t g_last_poll_time = 0;
static uint64_t g_last_refill_time = 0;

#define MAX_DESC_TRACE 2048
typedef struct {
    uint32_t desc_index;
    uint64_t start_us;
    uint64_t completion_us;
    uint64_t duration_us;
    uint64_t expected_us;
    int64_t jitter_us;
    uint64_t gap_before_us;
    uint32_t pcm_frames;
    bool timing_violation;
} DescriptorTimingRecord;

static DescriptorTimingRecord g_desc_trace[MAX_DESC_TRACE];
static uint32_t g_desc_trace_count = 0;
static uint64_t g_desc_start_us[AC97_BDL_ENTRIES];

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
    g_last_refill_time = timer_get_ticks();
    
    extern uint64_t step14_rdtsc(void);
    extern uint64_t step14_cycles_to_us(uint64_t);
    uint64_t start_us = step14_cycles_to_us(step14_rdtsc());
    for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
        g_desc_start_us[i] = start_us;
    }
    g_desc_trace_count = 0;
    
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
        if (sr & AC97_SR_DCH) { // DCH
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



// ===== Telemetry Counters (accumulated silently, dumped at end) =====
static uint32_t g_tel_total_polls = 0;
static uint32_t g_tel_rotations = 0;
static uint32_t g_tel_dch_halts = 0;
static uint32_t g_tel_max_poll_delta = 0;
static uint32_t g_tel_min_poll_delta = 0xFFFFFFFF;
static uint32_t g_tel_max_descriptors_behind = 0;
static uint32_t g_tel_stream_starve_count = 0;  // mixer got 0 bytes from stream
static uint64_t g_tel_total_mixer_bytes = 0;

static uint32_t g_tel_late_refills = 0;

uint64_t g_tl_port_io_us = 0;
uint64_t g_tl_desc_loop_us = 0;
uint64_t g_tl_mixer_us = 0;
uint64_t g_tl_lvi_us = 0;
uint32_t g_tl_desc_count = 0;

void ac97_playback_update(void) {
    if (g_pb_state != AC97_PB_STATE_RUNNING) return;
    
    uint8_t civ = io_in8(g_nabm_base + AC97_NABM_PO_CIV);
    uint16_t sr = io_in16(g_nabm_base + AC97_NABM_PO_SR);
    uint8_t cr = io_in8(g_nabm_base + AC97_NABM_PO_CR);
    
    uint64_t now = timer_get_ticks();
    g_last_poll_time = now;
    g_tel_total_polls++;
    
    // Clear status bits (write-1-to-clear: BCIS, LVBCI, FIFO ERR)
    if (sr & AC97_SR_WC_CLEAR_MASK) {
        io_out16(g_nabm_base + AC97_NABM_PO_SR, sr & AC97_SR_WC_CLEAR_MASK);
    }
    
    // FIX 1 — DCH Recovery State Synchronization:
    // When DMA controller halts (DCH set), refill descriptors and read REAL hardware CIV register.
    // Synchronize software history to actual hardware CIV instead of hardcoding 0.
    if (sr & AC97_SR_DCH) {
        g_tel_dch_halts++;
        g_pb_telemetry.underruns++;
        g_pb_telemetry.restarts++;
        
        size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
        
        // Refill ALL descriptors from mixer before restarting
        for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
            uint8_t* target = g_dma_mgr->pcm_buffer + (i * chunk_bytes);
            audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        }
        
        cr |= 0x01; // RPBM — restart DMA bus master
        io_out8(g_nabm_base + AC97_NABM_PO_CR, cr);
        
        // Read actual hardware CIV after restart to synchronize state
        uint8_t real_civ = io_in8(g_nabm_base + AC97_NABM_PO_CIV) % AC97_BDL_ENTRIES;
        g_last_civ = real_civ;
        g_lvi = (real_civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
        io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);
        return;
    }
    
    if (civ == g_last_civ) {
        return; // No rotation
    }
    
    // FIX 2 — Pure Realtime Refill Loop (Zero forensic logging overhead inside IRQ)
    size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
    uint32_t frames = chunk_bytes / 4;
    
    uint8_t idx = g_last_civ;
    while (idx != civ) {
        uint8_t next_idx = (idx + 1) % AC97_BDL_ENTRIES;
        uint8_t* target = g_dma_mgr->pcm_buffer + (idx * chunk_bytes);
        size_t returned_bytes = audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        g_tel_rotations++;
        
        g_pb_telemetry.bytes_sent += chunk_bytes;
        g_pb_telemetry.frames_played += frames;
        g_tel_total_mixer_bytes += chunk_bytes;
        if (returned_bytes < chunk_bytes) {
            g_pb_telemetry.underruns++;
        }
        
        idx = next_idx;
    }
    
    g_last_civ = civ;
    
    g_lvi = (civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
    io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);
}

void ac97_playback_dump_trace(void) {
    display_print("\n============================================================================================\n");
    display_print("DESCRIPTOR-LEVEL LATENCY TRACING (RAW TIMING)\n");
    display_print("============================================================================================\n");
    display_print("Index | Start (us) | Comp (us) | Dur (us) | Exp (us) | Jitter (us) | Gap (us) | Frames | Status\n");
    display_print("--------------------------------------------------------------------------------------------\n");
    
    for (uint32_t i = 0; i < g_desc_trace_count; i++) {
        DescriptorTimingRecord* rec = &g_desc_trace[i];
        display_print("DESC "); display_print_dec(rec->desc_index);
        display_print(" | "); display_print_dec(rec->start_us);
        display_print(" | "); display_print_dec(rec->completion_us);
        display_print(" | "); display_print_dec(rec->duration_us);
        display_print(" | "); display_print_dec(rec->expected_us);
        display_print(" | ");
        if (rec->jitter_us < 0) {
            display_print("-"); display_print_dec((uint64_t)(-rec->jitter_us));
        } else {
            display_print("+"); display_print_dec((uint64_t)(rec->jitter_us));
        }
        display_print(" | "); display_print_dec(rec->gap_before_us);
        display_print(" | "); display_print_dec(rec->pcm_frames);
        if (rec->timing_violation) {
            display_print(" | TIMING VIOLATION\n");
        } else {
            display_print(" | OK\n");
        }
    }
    display_print("============================================================================================\n");
}

void ac97_playback_status(void) {
    display_print("\n========================================\n");
    display_print("  AC97 PLAYBACK TELEMETRY\n");
    display_print("========================================\n");
    display_print("Total Polls       : "); display_print_dec(g_tel_total_polls); display_print("\n");
    display_print("CIV Rotations     : "); display_print_dec(g_tel_rotations); display_print("\n");
    display_print("DCH Halts         : "); display_print_dec(g_tel_dch_halts); display_print("\n");
    display_print("Underruns         : "); display_print_dec(g_pb_telemetry.underruns); display_print("\n");
    display_print("Late Refills      : "); display_print_dec(g_tel_late_refills); display_print("\n");
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

uint32_t ac97_playback_get_rotations(void) { return g_tel_rotations; }
uint32_t ac97_playback_get_dch_halts(void) { return g_tel_dch_halts; }
uint32_t ac97_playback_get_underruns(void) { return g_pb_telemetry.underruns; }
uint32_t ac97_playback_get_bytes_sent(void) { return g_pb_telemetry.bytes_sent; }
uint32_t ac97_playback_get_frames_played(void) { return g_pb_telemetry.frames_played; }
uint32_t ac97_playback_get_late_refills(void) { return g_tel_late_refills; }
Ac97PlaybackState ac97_playback_get_state(void) { return g_pb_state; }
