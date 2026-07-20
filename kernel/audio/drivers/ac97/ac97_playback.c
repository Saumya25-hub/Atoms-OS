#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "kernel/audio/core/audio_core.h"
#include "kernel/audio/drivers/ac97/ac97_dma.h"
#include "kernel/audio/drivers/ac97/ac97_registers.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/forensic/audio_forensic.h"
#include "kernel/drivers/display/display.h"

static Ac97PlaybackState g_pb_state = AC97_PB_STATE_UNINITIALIZED;
static Ac97PlaybackTelemetry g_pb_telemetry = {0};

// === MICRO-STUTTER FLIGHT RECORDER ===
#define FLIGHT_RECORDER_SIZE 128

typedef struct {
    uint64_t timestamp_us;
    uint8_t  previous_civ;
    uint8_t  current_civ;
    uint8_t  lvi;
    uint8_t  civ_jump;
    uint16_t sr;              // AC97 status register
    bool     dch;
    uint32_t ring_avail_before;
    uint32_t ring_avail_after;
    uint32_t mixer_returned;
    uint32_t gap_since_last_us;
    uint32_t producer_refills;
    uint32_t producer_bytes;
    uint32_t silence_total;
} StutterFlightSample;

static StutterFlightSample g_flight_buf[FLIGHT_RECORDER_SIZE];
static uint32_t g_flight_head = 0;
static bool g_flight_frozen = false;
static uint64_t g_flight_last_transition_us = 0;
static bool g_forensic_suppress_printing = false;

// === FORENSIC AGGREGATE STATS (silent, in-memory only) ===
static uint32_t g_forensic_ring_min = 0xFFFFFFFF;
static uint32_t g_forensic_ring_max = 0;
static uint64_t g_forensic_ring_sum = 0;
static uint32_t g_forensic_ring_samples = 0;
static uint32_t g_forensic_ring_below_4k = 0;
static uint32_t g_forensic_ring_below_16k = 0;
static uint32_t g_forensic_ring_below_64k = 0;

static void flight_recorder_dump(const char* reason) {
    if (g_flight_frozen || g_forensic_suppress_printing) return;
    g_flight_frozen = true;
    
    display_print("\n=== AUDIO MICRO-STUTTER FIRST FAILURE SNAPSHOT ===\n");
    display_print("Trigger: "); display_print(reason); display_print("\n\n");
    
    for (int i = 0; i < FLIGHT_RECORDER_SIZE; i++) {
        uint32_t idx = (g_flight_head + i) % FLIGHT_RECORDER_SIZE;
        StutterFlightSample* s = &g_flight_buf[idx];
        if (s->timestamp_us == 0) continue;
        
        display_print("T="); display_print_dec((uint32_t)s->timestamp_us);
        display_print(" pCIV="); display_print_dec(s->previous_civ);
        display_print(" cCIV="); display_print_dec(s->current_civ);
        display_print(" LVI="); display_print_dec(s->lvi);
        display_print(" J="); display_print_dec(s->civ_jump);
        if (s->dch) display_print(" [DCH!]");
        display_print(" RB="); display_print_dec(s->ring_avail_before);
        display_print(" RA="); display_print_dec(s->ring_avail_after);
        display_print(" Mix="); display_print_dec(s->mixer_returned);
        display_print(" Gap="); display_print_dec(s->gap_since_last_us);
        display_print("us Sil="); display_print_dec(s->silence_total);
        display_print("\n");
    }
    display_print("===================================================\n");
}

// Keep old struct name for compile compat but unused
static uint32_t g_desc_generation[AC97_BDL_ENTRIES] = {0};

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
    
    // Enable forensic silent mode: suppress all serial printing during playback
    g_forensic_suppress_printing = true;
    g_flight_frozen = false;
    g_flight_head = 0;
    g_forensic_ring_min = 0xFFFFFFFF;
    g_forensic_ring_max = 0;
    g_forensic_ring_sum = 0;
    g_forensic_ring_samples = 0;
    g_forensic_ring_below_4k = 0;
    g_forensic_ring_below_16k = 0;
    g_forensic_ring_below_64k = 0;
    
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

// Forward declaration for forensic dump (defined after telemetry counters)
static void ac97_forensic_session_dump(void);

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
    
    // One-time forensic summary dump after playback ends
    g_forensic_suppress_printing = false;
    ac97_forensic_session_dump();
    
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

static uint32_t g_CIVMaxJump = 0;
static uint32_t g_DescriptorsRecoveredAfterCIVJump = 0;
static uint32_t g_MixerShortFills = 0;
uint64_t g_MixerSilenceInjectedBytes = 0;
static uint64_t g_AudioWorkerMaxGapUs = 0;
static uint64_t g_last_ac97_update_ticks = 0;

// === ONE-TIME FORENSIC SESSION DUMP (defined here after all counters) ===
static void ac97_forensic_session_dump(void) {
    display_print("\n====================================================\n");
    display_print("  FORENSIC PLAYBACK SESSION SUMMARY (ONE-TIME)\n");
    display_print("====================================================\n");
    
    display_print("--- Ring Buffer Health ---\n");
    display_print("RingMinAvailable     : "); display_print_dec(g_forensic_ring_min == 0xFFFFFFFF ? 0 : g_forensic_ring_min); display_print(" bytes\n");
    display_print("RingMaxAvailable     : "); display_print_dec(g_forensic_ring_max); display_print(" bytes\n");
    if (g_forensic_ring_samples > 0) {
        display_print("RingAvgAvailable     : "); display_print_dec((uint32_t)(g_forensic_ring_sum / g_forensic_ring_samples)); display_print(" bytes\n");
    }
    display_print("RingBelow4K events   : "); display_print_dec(g_forensic_ring_below_4k); display_print("\n");
    display_print("RingBelow16K events  : "); display_print_dec(g_forensic_ring_below_16k); display_print("\n");
    display_print("RingBelow64K events  : "); display_print_dec(g_forensic_ring_below_64k); display_print("\n");
    display_print("CIV Transition Count : "); display_print_dec(g_forensic_ring_samples); display_print("\n");
    
    display_print("--- Mixer ---\n");
    display_print("MixerShortFills      : "); display_print_dec(g_MixerShortFills); display_print("\n");
    display_print("MixerSilenceBytes    : "); display_print_dec((uint32_t)g_MixerSilenceInjectedBytes); display_print("\n");
    
    display_print("--- AC97 Hardware ---\n");
    display_print("CIVMaxJump           : "); display_print_dec(g_CIVMaxJump); display_print("\n");
    display_print("DCHHalts             : "); display_print_dec(g_tel_dch_halts); display_print("\n");
    display_print("CIVRotations         : "); display_print_dec(g_tel_rotations); display_print("\n");
    display_print("TotalPolls           : "); display_print_dec(g_tel_total_polls); display_print("\n");
    display_print("MaxAC97UpdateGapUs   : "); display_print_dec((uint32_t)g_AudioWorkerMaxGapUs); display_print(" us\n");
    
    display_print("--- Producer ---\n");
    extern uint32_t g_ProducerRefillCalls;
    extern uint64_t g_ProducerBytesRead;
    extern uint32_t g_ProducerMaxServiceGapMs;
    display_print("ProducerRefillCalls  : "); display_print_dec(g_ProducerRefillCalls); display_print("\n");
    display_print("ProducerBytesRead    : "); display_print_dec((uint32_t)g_ProducerBytesRead); display_print("\n");
    display_print("ProdMaxServiceGapMs  : "); display_print_dec(g_ProducerMaxServiceGapMs); display_print(" ms\n");
    
    display_print("--- Flight Recorder (last 128 CIV transitions) ---\n");
    uint32_t printed = 0;
    for (int i = 0; i < FLIGHT_RECORDER_SIZE; i++) {
        uint32_t idx = (g_flight_head + i) % FLIGHT_RECORDER_SIZE;
        StutterFlightSample* s = &g_flight_buf[idx];
        if (s->timestamp_us == 0) continue;
        
        display_print("T="); display_print_dec((uint32_t)s->timestamp_us);
        display_print(" pCIV="); display_print_dec(s->previous_civ);
        display_print(" cCIV="); display_print_dec(s->current_civ);
        display_print(" J="); display_print_dec(s->civ_jump);
        if (s->dch) display_print(" [DCH]");
        display_print(" RB="); display_print_dec(s->ring_avail_before);
        display_print(" RA="); display_print_dec(s->ring_avail_after);
        display_print(" Mix="); display_print_dec(s->mixer_returned);
        display_print(" Gap="); display_print_dec(s->gap_since_last_us);
        display_print("us\n");
        printed++;
    }
    display_print("FlightEntriesPrinted : "); display_print_dec(printed); display_print("\n");
    display_print("====================================================\n");
}


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
    
    extern size_t audio_stream_available(uint32_t stream_id);
    size_t avail = audio_stream_available(0);
    audio_forensic_record_sample(civ, g_lvi, (sr & AC97_SR_DCH) != 0, 0);
    
    // DCH Recovery
    if (sr & AC97_SR_DCH) {
        g_tel_dch_halts++;
        g_pb_telemetry.underruns++;
        g_pb_telemetry.restarts++;
        
        // Flight recorder: record DCH event
        if (!g_flight_frozen) {
            extern uint64_t step14_rdtsc(void);
            extern uint64_t step14_cycles_to_us(uint64_t);
            extern uint32_t g_ProducerRefillCalls;
            extern uint64_t g_ProducerBytesRead;
            StutterFlightSample* s = &g_flight_buf[g_flight_head];
            s->timestamp_us = step14_cycles_to_us(step14_rdtsc());
            s->previous_civ = g_last_civ;
            s->current_civ = civ;
            s->lvi = g_lvi;
            s->civ_jump = 0;
            s->sr = sr;
            s->dch = true;
            s->ring_avail_before = (uint32_t)avail;
            s->ring_avail_after = 0;
            s->mixer_returned = 0;
            s->gap_since_last_us = 0;
            s->producer_refills = g_ProducerRefillCalls;
            s->producer_bytes = (uint32_t)g_ProducerBytesRead;
            s->silence_total = (uint32_t)g_MixerSilenceInjectedBytes;
            g_flight_head = (g_flight_head + 1) % FLIGHT_RECORDER_SIZE;
            
            flight_recorder_dump("AC97 DCH Halt");
        }
        
        size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
        for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
            uint8_t* target = g_dma_mgr->pcm_buffer + (i * chunk_bytes);
            audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        }
        
        cr |= 0x01;
        io_out8(g_nabm_base + AC97_NABM_PO_CR, cr);
        
        uint8_t real_civ = io_in8(g_nabm_base + AC97_NABM_PO_CIV) % AC97_BDL_ENTRIES;
        g_last_civ = real_civ;
        g_lvi = (real_civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
        io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);
        return;
    }
    
    if (civ == g_last_civ) {
        return; // No rotation
    }
    
    // CIV transition detected — record flight sample
    size_t chunk_bytes = g_dma_mgr->pcm_buffer_size / AC97_BDL_ENTRIES;
    uint32_t frames = chunk_bytes / 4;
    
    uint8_t idx = g_last_civ;
    uint32_t jump_size = (civ + AC97_BDL_ENTRIES - g_last_civ) % AC97_BDL_ENTRIES;
    if (jump_size > g_CIVMaxJump && jump_size < AC97_BDL_ENTRIES) {
        g_CIVMaxJump = jump_size;
    }
    if (jump_size > 1) {
        g_DescriptorsRecoveredAfterCIVJump += jump_size;
    }
    
    extern uint64_t step14_rdtsc(void);
    extern uint64_t step14_cycles_to_us(uint64_t);
    uint64_t now_us = step14_cycles_to_us(step14_rdtsc());
    uint32_t gap_us = 0;
    if (g_flight_last_transition_us != 0) {
        gap_us = (uint32_t)(now_us - g_flight_last_transition_us);
    }
    g_flight_last_transition_us = now_us;

    uint64_t now_update = timer_get_ticks();
    if (g_last_ac97_update_ticks != 0) {
        uint64_t gap = step14_cycles_to_us(now_update - g_last_ac97_update_ticks);
        if (gap > g_AudioWorkerMaxGapUs) {
            g_AudioWorkerMaxGapUs = gap;
        }
    }
    g_last_ac97_update_ticks = now_update;

    // Snapshot ring BEFORE mixer reads
    size_t ring_before = audio_stream_available(0);
    
    // Forensic aggregate stats (silent, no printing)
    if ((uint32_t)ring_before < g_forensic_ring_min) g_forensic_ring_min = (uint32_t)ring_before;
    if ((uint32_t)ring_before > g_forensic_ring_max) g_forensic_ring_max = (uint32_t)ring_before;
    g_forensic_ring_sum += (uint64_t)ring_before;
    g_forensic_ring_samples++;
    if (ring_before < 4096) g_forensic_ring_below_4k++;
    if (ring_before < 16384) g_forensic_ring_below_16k++;
    if (ring_before < 65536) g_forensic_ring_below_64k++;
    
    // Refill descriptors
    uint32_t total_mixer_returned = 0;
    while (idx != civ) {
        uint8_t next_idx = (idx + 1) % AC97_BDL_ENTRIES;
        uint8_t* target = g_dma_mgr->pcm_buffer + (idx * chunk_bytes);
        
        size_t returned_bytes = audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        total_mixer_returned += returned_bytes;
        
        if (returned_bytes < chunk_bytes) {
            g_MixerShortFills++;
            size_t missing = chunk_bytes - returned_bytes;
            g_MixerSilenceInjectedBytes += missing;
            for (size_t i = returned_bytes; i < chunk_bytes; i++) {
                target[i] = 0;
            }
        }
        
        g_tel_rotations++;
        g_pb_telemetry.bytes_sent += chunk_bytes;
        g_pb_telemetry.frames_played += chunk_bytes / 4;
        g_tel_total_mixer_bytes += chunk_bytes;
        
        idx = next_idx;
    }
    g_last_civ = civ;
    
    // Snapshot ring AFTER mixer reads
    size_t ring_after = audio_stream_available(0);
    
    __asm__ volatile("" ::: "memory");
    g_lvi = (civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
    io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);
    
    // === FLIGHT RECORDER: record this CIV transition ===
    if (!g_flight_frozen) {
        extern uint32_t g_ProducerRefillCalls;
        extern uint64_t g_ProducerBytesRead;
        
        StutterFlightSample* s = &g_flight_buf[g_flight_head];
        s->timestamp_us = now_us;
        s->previous_civ = g_last_civ;
        s->current_civ = civ;
        s->lvi = g_lvi;
        s->civ_jump = (uint8_t)jump_size;
        s->sr = sr;
        s->dch = false;
        s->ring_avail_before = (uint32_t)ring_before;
        s->ring_avail_after = (uint32_t)ring_after;
        s->mixer_returned = total_mixer_returned;
        s->gap_since_last_us = gap_us;
        s->producer_refills = g_ProducerRefillCalls;
        s->producer_bytes = (uint32_t)g_ProducerBytesRead;
        s->silence_total = (uint32_t)g_MixerSilenceInjectedBytes;
        g_flight_head = (g_flight_head + 1) % FLIGHT_RECORDER_SIZE;
        
        // Trigger conditions
        if (ring_before < chunk_bytes) {
            flight_recorder_dump("Ring starvation before mix (< 1 descriptor)");
        } else if (jump_size > 2) {
            flight_recorder_dump("CIV jumped >2 positions (missed descriptors)");
        } else if (total_mixer_returned < chunk_bytes * jump_size) {
            flight_recorder_dump("Mixer short fill during CIV transition");
        } else if (gap_us > 50000) {
            flight_recorder_dump("CIV transition gap >50ms");
        }
    }
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
    display_print("CIVMaxJump        : "); display_print_dec(g_CIVMaxJump); display_print("\n");
    display_print("DescRecovered     : "); display_print_dec(g_DescriptorsRecoveredAfterCIVJump); display_print("\n");
    display_print("MixerShortFills   : "); display_print_dec(g_MixerShortFills); display_print("\n");
    display_print("MixerSilenceBytes : "); display_print_dec(g_MixerSilenceInjectedBytes); display_print("\n");
    display_print("AudioWorkerMaxGap : "); display_print_dec(g_AudioWorkerMaxGapUs); display_print(" us\n");
    extern void audio_player_print_telemetry(void);
    audio_player_print_telemetry();
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

uint8_t ac97_get_civ(void) {
    if (!g_dma_mgr) return 0;
    extern uint8_t io_in8(uint16_t port);
    return io_in8(g_nabm_base + 0x14); // AC97_NABM_PO_CIV
}

uint8_t ac97_get_lvi(void) {
    if (!g_dma_mgr) return 0;
    extern uint8_t io_in8(uint16_t port);
    return io_in8(g_nabm_base + 0x15); // AC97_NABM_PO_LVI
}
