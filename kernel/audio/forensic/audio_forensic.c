#include "kernel/audio/forensic/audio_forensic.h"
#include "kernel/drivers/display/display.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/debug/step14_telemetry.h"

extern uint64_t timer_get_ticks(void);
extern uint32_t g_ProducerRefillCalls;
extern uint64_t g_ProducerBytesRead;
extern uint64_t g_MixerSilenceInjectedBytes;
extern uint32_t g_active_streams;

extern uint64_t g_last_vfs_read_us;
extern uint64_t g_max_vfs_read_us;
extern uint32_t g_last_read_requested;
extern uint32_t g_last_read_returned;

#define FORENSIC_RING_SIZE 64

static AudioForensicSample g_forensic_ring[FORENSIC_RING_SIZE];
static int g_forensic_head = 0;
static bool g_forensic_frozen = false;
static uint64_t g_last_sample_time = 0;

void audio_forensic_init(void) {
    for (int i = 0; i < FORENSIC_RING_SIZE; i++) {
        g_forensic_ring[i].timestamp = 0;
    }
    g_forensic_head = 0;
    g_forensic_frozen = false;
    g_last_sample_time = 0;
}

void audio_forensic_reset(void) {
    audio_forensic_init();
}

void audio_forensic_record_sample(uint8_t civ, uint8_t lvi, bool dch, uint32_t desc_checksum) {
    if (g_forensic_frozen) return;

    uint64_t now = timer_get_ticks();
    
    // Throttle to 100ms unless DCH occurs
    if (!dch && (now - g_last_sample_time < 100)) {
        return;
    }
    g_last_sample_time = now;

    AudioForensicSample* sample = &g_forensic_ring[g_forensic_head];
    sample->timestamp = now;
    sample->civ = civ;
    sample->lvi = lvi;
    sample->dch = dch;
    
    sample->software_ring_available = audio_stream_available(0); // Stream 0 is main
    
    uint32_t bytes_played = 0;
    sample->bytes_played = bytes_played;
    
    sample->producer_refill_calls = g_ProducerRefillCalls;
    sample->producer_bytes_read = (uint32_t)g_ProducerBytesRead;
    
    sample->mixer_active_streams = 1; // Assuming 1
    sample->mixer_silence_bytes = (uint32_t)g_MixerSilenceInjectedBytes;
    
    sample->descriptor_checksum = desc_checksum;

    // vfs_read telemetry
    sample->last_vfs_read_us = g_last_vfs_read_us;
    sample->max_vfs_read_us = g_max_vfs_read_us;
    sample->last_read_requested = g_last_read_requested;
    sample->last_read_returned = g_last_read_returned;

    g_forensic_head = (g_forensic_head + 1) % FORENSIC_RING_SIZE;
}

void audio_forensic_dump(const char* reason) {
    if (g_forensic_frozen) return;
    g_forensic_frozen = true;

    display_print("\n=== AUDIO FIRST CORRUPTION BOUNDARY ===\n");
    display_print("Reason: ");
    display_print(reason);
    display_print("\n\n");

    int idx = g_forensic_head;
    for (int i = 0; i < FORENSIC_RING_SIZE; i++) {
        AudioForensicSample* s = &g_forensic_ring[idx];
        if (s->timestamp != 0) {
            display_print("T="); display_print_dec((uint32_t)s->timestamp);
            display_print(" CIV="); display_print_dec(s->civ);
            display_print(" LVI="); display_print_dec(s->lvi);
            if (s->dch) display_print(" [DCH!]");
            display_print(" SW_Avail="); display_print_dec(s->software_ring_available);
            display_print(" Refills="); display_print_dec(s->producer_refill_calls);
            display_print(" P_Read="); display_print_dec(s->producer_bytes_read);
            display_print(" Silence="); display_print_dec(s->mixer_silence_bytes);
            display_print(" Cksum="); display_print_hex(s->descriptor_checksum);
            display_print(" vfs_us="); display_print_dec((uint32_t)s->last_vfs_read_us);
            display_print(" max_vfs="); display_print_dec((uint32_t)s->max_vfs_read_us);
            display_print("\n");
        }
        idx = (idx + 1) % FORENSIC_RING_SIZE;
    }
    display_print("=======================================\n");
}

// Stubs for old references
void audio_forensic_log_pcm_gen(const uint8_t* pcm_data, size_t bytes) { (void)pcm_data; (void)bytes; }
void audio_forensic_log_stream_write(const uint8_t* pcm_data, size_t bytes) { (void)pcm_data; (void)bytes; }
void audio_forensic_log_stream_read(const uint8_t* pcm_data, size_t bytes) { (void)pcm_data; (void)bytes; }
void audio_forensic_log_mixer_out(const uint8_t* pcm_data, size_t bytes) { (void)pcm_data; (void)bytes; }
void audio_forensic_log_dma_out(const uint8_t* pcm_data, size_t bytes) { (void)pcm_data; (void)bytes; }
void audio_forensic_dump_oscilloscope(void) {}
