#include "kernel/audio/forensic/audio_forensic.h"
#include "kernel/debug/step14_telemetry.h"

#define MAX_EVENTS 100000
#define TEN_SECONDS_US 120000000ULL

typedef struct {
    uint64_t timestamp_us;
    AudioEventType type;
    uint32_t data1;
    uint32_t data2;
} AudioTelemetryEvent;

static AudioTelemetryEvent g_events[MAX_EVENTS];
static uint32_t g_event_count = 0;
static bool g_dumped = false;
static uint64_t g_start_time = 0;

void audio_forensic_init(void) {
    g_event_count = 0;
    g_dumped = false;
    g_start_time = 0;
}

void audio_forensic_reset(void) {
    g_event_count = 0;
    g_dumped = false;
    g_start_time = 0; // Will be set on first event after reset
}

void audio_forensic_record(AudioEventType type, uint32_t data1, uint32_t data2) {
    if (g_dumped) return;

    uint64_t now_us = step14_cycles_to_us(step14_rdtsc());
    if (g_start_time == 0) {
        g_start_time = now_us;
    }
    
    if (g_event_count < MAX_EVENTS) {
        g_events[g_event_count].timestamp_us = now_us;
        g_events[g_event_count].type = type;
        g_events[g_event_count].data1 = data1;
        g_events[g_event_count].data2 = data2;
        g_event_count++;
    }

    // Dump if 10 seconds of playback have elapsed OR buffer is full
    if (g_event_count >= MAX_EVENTS || (now_us - g_start_time) >= TEN_SECONDS_US) {
        audio_forensic_dump();
    }
}

static void serial_write_forensic(char c) {
    extern void io_out8(uint16_t port, uint8_t data);
    extern uint8_t io_in8(uint16_t port);
    while ((io_in8(0x3F8 + 5) & 0x20) == 0);
    io_out8(0x3F8, c);
}
static void serial_print_forensic(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_write_forensic(str[i]);
    }
}
static void serial_print_dec_forensic(uint64_t val) {
    if (val == 0) { serial_write_forensic('0'); return; }
    char buf[20];
    int i = 0;
    while (val > 0) {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0) {
        serial_write_forensic(buf[--i]);
    }
}

void audio_forensic_dump(void) {
    if (g_dumped) return;
    g_dumped = true;

    // Analyze events for summary statistics
    uint32_t underruns = 0;
    uint32_t starvations = 0;
    uint64_t max_dma_gap_us = 0;
    uint64_t last_dma_refill_us = 0;
    uint64_t max_sched_gap_us = 0;
    uint64_t last_sched_us = 0;
    uint64_t max_present_us = 0;
    uint64_t present_start_us = 0;
    uint64_t max_vfs_read_us = 0;
    uint64_t vfs_read_start_us = 0;
    uint64_t max_audio_thread_us = 0;
    uint64_t audio_thread_start_us = 0;

    for (uint32_t i = 0; i < g_event_count; i++) {
        AudioTelemetryEvent* ev = &g_events[i];
        if (ev->type == EV_DMA_UNDERRUN) underruns++;
        if (ev->type == EV_STREAM_STARVED) starvations++;

        if (ev->type == EV_DMA_REFILL_START) {
            if (last_dma_refill_us > 0) {
                uint64_t gap = ev->timestamp_us - last_dma_refill_us;
                if (gap > max_dma_gap_us) max_dma_gap_us = gap;
            }
            last_dma_refill_us = ev->timestamp_us;
        }

        if (ev->type == EV_SCHEDULER_WAKE) {
            if (last_sched_us > 0) {
                uint64_t gap = ev->timestamp_us - last_sched_us;
                if (gap > max_sched_gap_us) max_sched_gap_us = gap;
            }
            last_sched_us = ev->timestamp_us;
        }

        if (ev->type == EV_PRESENT_START) present_start_us = ev->timestamp_us;
        if (ev->type == EV_PRESENT_END && present_start_us > 0) {
            uint64_t dur = ev->timestamp_us - present_start_us;
            if (dur > max_present_us) max_present_us = dur;
            present_start_us = 0;
        }

        if (ev->type == EV_VFS_READ_START) vfs_read_start_us = ev->timestamp_us;
        if (ev->type == EV_VFS_READ_END && vfs_read_start_us > 0) {
            uint64_t dur = ev->timestamp_us - vfs_read_start_us;
            if (dur > max_vfs_read_us) max_vfs_read_us = dur;
            vfs_read_start_us = 0;
        }

        if (ev->type == EV_AUDIO_THREAD_START) audio_thread_start_us = ev->timestamp_us;
        if (ev->type == EV_AUDIO_THREAD_END && audio_thread_start_us > 0) {
            uint64_t dur = ev->timestamp_us - audio_thread_start_us;
            if (dur > max_audio_thread_us) max_audio_thread_us = dur;
            audio_thread_start_us = 0;
        }
    }

    serial_print_forensic("\n=======================================================\n");
    serial_print_forensic("     AC97 FORENSIC LATENCY ANALYSIS (10 SEC DUMP)      \n");
    serial_print_forensic("=======================================================\n");
    serial_print_forensic("Total Captured Events   : "); serial_print_dec_forensic(g_event_count); serial_print_forensic("\n");
    serial_print_forensic("DMA Underruns (DCH Halt): "); serial_print_dec_forensic(underruns); serial_print_forensic("\n");
    serial_print_forensic("Stream Starvations      : "); serial_print_dec_forensic(starvations); serial_print_forensic("\n");
    serial_print_forensic("Max DMA Refill Gap (us) : "); serial_print_dec_forensic(max_dma_gap_us); serial_print_forensic("\n");
    serial_print_forensic("Max Sched Wake Gap (us) : "); serial_print_dec_forensic(max_sched_gap_us); serial_print_forensic("\n");
    serial_print_forensic("Max Present Duration(us): "); serial_print_dec_forensic(max_present_us); serial_print_forensic("\n");
    serial_print_forensic("Max AudioThread Dur (us): "); serial_print_dec_forensic(max_audio_thread_us); serial_print_forensic("\n");
    serial_print_forensic("Max VFS Read Dur (us)   : "); serial_print_dec_forensic(max_vfs_read_us); serial_print_forensic("\n");
    serial_print_forensic("=======================================================\n");
    serial_print_forensic("                 DETAILED EVENT TIMELINE               \n");
    serial_print_forensic("=======================================================\n");

    for (uint32_t i = 0; i < g_event_count; i++) {
        AudioTelemetryEvent* ev = &g_events[i];
        
        serial_print_dec_forensic(ev->timestamp_us - g_start_time);
        serial_print_forensic(" us | ");

        switch (ev->type) {
            case EV_PIT_TICK: serial_print_forensic("PIT_TICK"); break;
            case EV_SCHEDULER_WAKE: serial_print_forensic("SCHEDULER_WAKE"); break;
            case EV_AUDIO_THREAD_START: serial_print_forensic("AUDIO_THREAD_START"); break;
            case EV_AUDIO_THREAD_END: serial_print_forensic("AUDIO_THREAD_END"); break;
            case EV_VFS_READ_START: serial_print_forensic("VFS_READ_START"); break;
            case EV_VFS_READ_END: serial_print_forensic("VFS_READ_END"); break;
            case EV_STREAM_WRITE: serial_print_forensic("STREAM_WRITE"); break;
            case EV_STREAM_READ: serial_print_forensic("STREAM_READ"); break;
            case EV_STREAM_STARVED: serial_print_forensic("!!! STREAM_STARVED !!!"); break;
            case EV_MIXER_START: serial_print_forensic("MIXER_START"); break;
            case EV_MIXER_END: serial_print_forensic("MIXER_END"); break;
            case EV_DMA_REFILL_START: serial_print_forensic("DMA_REFILL_START"); break;
            case EV_DMA_REFILL_END: serial_print_forensic("DMA_REFILL_END"); break;
            case EV_DMA_UNDERRUN: serial_print_forensic("!!! DMA_UNDERRUN !!!"); break;
            case EV_PRESENT_START: serial_print_forensic("PRESENT_START"); break;
            case EV_PRESENT_END: serial_print_forensic("PRESENT_END"); break;
            default: serial_print_forensic("UNKNOWN"); break;
        }

        serial_print_forensic(" | D1: ");
        serial_print_dec_forensic(ev->data1);
        serial_print_forensic(" | D2: ");
        serial_print_dec_forensic(ev->data2);
        serial_print_forensic("\n");
    }

    serial_print_forensic("=======================================================\n");
    serial_print_forensic("                 END OF LATENCY DUMP                   \n");
    serial_print_forensic("=======================================================\n");
    extern void ac97_playback_dump_trace(void);
    ac97_playback_dump_trace();
    extern void horse_shutdown(void);
    horse_shutdown();
}

// Stubs for old oscilloscope code
void audio_forensic_log_pcm_gen(const uint8_t* pcm_data, size_t bytes) {}
void audio_forensic_log_stream_write(const uint8_t* pcm_data, size_t bytes) {}
void audio_forensic_log_stream_read(const uint8_t* pcm_data, size_t bytes) {}
void audio_forensic_log_mixer_out(const uint8_t* pcm_data, size_t bytes) {}
void audio_forensic_log_dma_out(const uint8_t* pcm_data, size_t bytes) {}
void audio_forensic_dump_oscilloscope(void) {}
