#include "kernel/audio/session/audio_player.h"
#include "kernel/audio/core/audio_core.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/hal/audio_hal.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

extern uint64_t timer_get_ticks(void);
extern int vfs_seek(int fd, uint64_t offset, int whence);
extern void audio_forensic_reset(void);
extern void audio_realtime_worker_start(void);
extern void audio_realtime_worker_stop(void);

#pragma pack(push, 1)
typedef struct {
    char riff_id[4];
    uint32_t riff_size;
    char wave_id[4];
} RiffHeader;

typedef struct {
    char id[4];
    uint32_t size;
} ChunkHeader;

typedef struct {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} FmtChunk;
#pragma pack(pop)

// ----------------------------------------------------
// Global Audio Session
// ----------------------------------------------------
typedef struct {
    AudioPlayerState state;
    int fd;
    uint32_t stream_id;
    AudioPcmFormat format;
    uint32_t data_size;
    uint32_t bytes_played;
    uint64_t data_offset;
} AudioSession;

static AudioSession g_audio_session = {
    .state = PLAYER_STATE_STOPPED,
    .fd = -1,
    .stream_id = 0,
    .data_size = 0,
    .bytes_played = 0,
    .data_offset = 44
};

static uint8_t g_read_buffer[32768];
static uint64_t g_last_read_time_ticks = 0;

static uint32_t g_AudioRingMinAvailable = 0xFFFFFFFF;
uint32_t g_ProducerRefillCalls = 0;
uint64_t g_ProducerBytesRead = 0;
uint32_t g_ProducerMaxServiceGapMs = 0;
static uint64_t g_last_producer_ticks = 0;

// === DIAGNOSTIC RAM-ONLY TEST ===
#define RAM_CHUNK_SIZE 262144 // 256KB
#define NUM_RAM_CHUNKS 24     // 6.14MB
static uint8_t* g_ram_audio_chunks[NUM_RAM_CHUNKS] = {0};
bool g_RAM_Only_Test_Active = false;
static uint32_t g_ram_audio_total_bytes = 0;
static uint32_t g_ram_audio_cursor = 0;
uint32_t g_RAMAudioBytesProduced = 0;
uint32_t g_RAMAudioRefillCalls = 0;
uint32_t g_AudioVFSReadsAfterStart = 0;
// ================================

void audio_player_print_telemetry(void) {
    display_print("RingMinAvailable  : "); display_print_dec(g_AudioRingMinAvailable); display_print("\\n");
    display_print("ProducerRefillCall: "); display_print_dec(g_ProducerRefillCalls); display_print("\n");
    display_print("ProducerBytesRead : "); display_print_dec(g_ProducerBytesRead); display_print("\n");
    display_print("ProdMaxServiceGap : "); display_print_dec(g_ProducerMaxServiceGapMs); display_print(" ms\n");
}


static bool audio_player_read_exact(int fd, void* buffer, uint32_t size, uint64_t* cursor) {
    int read_bytes = vfs_read(fd, buffer, size);
    if (read_bytes != (int)size) {
        return false;
    }
    if (cursor) {
        *cursor += size;
    }
    return true;
}

static bool audio_player_skip_bytes(int fd, uint32_t size, uint64_t* cursor) {
    uint8_t dummy;
    for (uint32_t i = 0; i < size; i++) {
        if (!audio_player_read_exact(fd, &dummy, 1, cursor)) {
            return false;
        }
    }
    return true;
}

void audio_player_open(const char* path) {
    display_print("[003] Entered audio_player_open()\n");
    if (g_audio_session.state != PLAYER_STATE_STOPPED) {
        audio_player_close();
    }
    
    display_print("[004] Calling vfs_open(\"/DEMO1.WAV\")\n");
    int fd = vfs_open(path);
    if (fd < 0) {
        display_print("[AUDIO_PLAYER] vfs_open failed! Result: fd < 0\n");
        return;
    }
    display_print("[005] vfs_open() returned successfully\n");
    
    display_print("[006] Reading RIFF header\n");
    RiffHeader riff;
    uint64_t file_cursor = 0;
    if (!audio_player_read_exact(fd, &riff, sizeof(RiffHeader), &file_cursor)) {
        display_print("[AUDIO_PLAYER] RIFF read failed!\n");
        vfs_close(fd);
        return;
    }
    display_print("[007] RIFF header valid\n");
    
    if (riff.riff_id[0] != 'R' || riff.riff_id[1] != 'I' || riff.riff_id[2] != 'F' || riff.riff_id[3] != 'F' ||
        riff.wave_id[0] != 'W' || riff.wave_id[1] != 'A' || riff.wave_id[2] != 'V' || riff.wave_id[3] != 'E') {
        display_print("[AUDIO_PLAYER] Invalid RIFF/WAVE signature!\n");
        vfs_close(fd);
        return;
    }
    
    display_print("[008] Reading fmt and data chunks\n");
    FmtChunk fmt = {0};
    bool found_fmt = false;
    uint32_t data_size = 0;
    uint64_t data_offset = 0;
    
    while (1) {
        ChunkHeader chunk;
        if (!audio_player_read_exact(fd, &chunk, sizeof(ChunkHeader), &file_cursor)) {
            break;
        }
        
        if (chunk.id[0] == 'f' && chunk.id[1] == 'm' && chunk.id[2] == 't' && chunk.id[3] == ' ') {
            if (!audio_player_read_exact(fd, &fmt, sizeof(FmtChunk), &file_cursor)) break;
            found_fmt = true;
            if (chunk.size > sizeof(FmtChunk)) {
                if (!audio_player_skip_bytes(fd, chunk.size - sizeof(FmtChunk), &file_cursor)) break;
            }
        } else if (chunk.id[0] == 'd' && chunk.id[1] == 'a' && chunk.id[2] == 't' && chunk.id[3] == 'a') {
            data_size = chunk.size;
            data_offset = file_cursor;
            break; 
        } else {
            if (!audio_player_skip_bytes(fd, chunk.size, &file_cursor)) break;
        }
    }
    
    if (!found_fmt || data_size == 0) {
        display_print("[AUDIO_PLAYER] fmt or data chunk missing!\n");
        vfs_close(fd);
        return;
    }
    
    
    display_print("\nAUDIO_FORMAT:\n");
    display_print("channels="); display_print_dec(fmt.num_channels); display_print("\n");
    display_print("sample_rate="); display_print_dec(fmt.sample_rate); display_print("\n");
    display_print("bits_per_sample="); display_print_dec(fmt.bits_per_sample); display_print("\n");
    display_print("block_align="); display_print_dec(fmt.block_align); display_print("\n");
    display_print("data_offset="); display_print_dec(data_offset); display_print("\n");
    display_print("data_size="); display_print_dec(data_size); display_print("\n");
    display_print("data_size % block_align="); display_print_dec(data_size % fmt.block_align); display_print("\n");

    if (fmt.audio_format != 1 || fmt.num_channels != 2 || fmt.sample_rate != 48000 || fmt.bits_per_sample != 16) {
        display_print("[AUDIO_PLAYER] Unsupported format (must be 48kHz 16-bit stereo PCM)!\n");
        vfs_close(fd);
        return;
    }
    
    if (audio_hal_get_active_driver() == NULL) {
        vfs_close(fd);
        return;
    }
    
    display_print("[009] Found valid WAV format (48kHz 16-bit Stereo PCM)\n");
    display_print("[010] Calling audio_stream_create(0)\n");
    uint32_t stream_id = audio_stream_create(0);
    display_print("[011] Stream created successfully\n");
    audio_set_volume(stream_id, 255);
    audio_mixer_add_stream(stream_id);
    display_print("[012] Added stream to mixer and set format\n");
    
    AudioPcmFormat format = {
        .format = PCM_FORMAT_S16_LE,
        .sample_rate = 48000,
        .channels = 2,
        .bit_depth = 16,
        .is_signed = true
    };
    audio_stream_set_format(stream_id, &format);
    
    g_audio_session.fd = fd;
    g_audio_session.stream_id = stream_id;
    g_audio_session.format = format;
    g_audio_session.data_size = data_size;
    g_audio_session.bytes_played = 0;
    g_audio_session.data_offset = data_offset;
    g_audio_session.state = PLAYER_STATE_OPENED;
}

void audio_player_play(void) {
    display_print("[015] Entered audio_player_play()\n");
    if (g_audio_session.state == PLAYER_STATE_OPENED) {
        
        // === DIAGNOSTIC RAM-ONLY TEST PRELOAD ===
        g_RAM_Only_Test_Active = true;
        g_ram_audio_total_bytes = 0;
        g_ram_audio_cursor = 0;
        extern void* kmalloc(size_t size);
        for (int i = 0; i < NUM_RAM_CHUNKS; i++) {
            if (!g_ram_audio_chunks[i]) {
                g_ram_audio_chunks[i] = kmalloc(RAM_CHUNK_SIZE);
            }
            if (g_ram_audio_chunks[i]) {
                int r = vfs_read(g_audio_session.fd, g_ram_audio_chunks[i], RAM_CHUNK_SIZE);
                if (r > 0) g_ram_audio_total_bytes += r;
                if (r < RAM_CHUNK_SIZE) break;
            }
        }
        display_print("[DIAG] Preloaded "); display_print_dec(g_ram_audio_total_bytes); display_print(" bytes into RAM\n");
        // Completely isolate VFS
        vfs_close(g_audio_session.fd);
        g_audio_session.fd = -1;
        // ========================================

        audio_forensic_reset();
        
        display_print("[016] Starting prefill loop\n");
        while (audio_stream_capacity(g_audio_session.stream_id) - audio_stream_available(g_audio_session.stream_id) >= 16384 + audio_pcm_bytes_per_frame(&g_audio_session.format) &&
               g_audio_session.bytes_played < g_audio_session.data_size) {
            audio_player_update();
        }
        
        display_print("[020] Prefill completed\n");
        display_print("[021] Calling audio_hal_start_stream(48000, 2, 16)\n");
        audio_hal_start_stream(48000, 2, 16);
        audio_realtime_worker_start();
        display_print("[022] Returned from audio_hal_start_stream()\n");
        g_audio_session.state = PLAYER_STATE_PLAYING;
    }
}

void audio_player_pause(void) {
    if (g_audio_session.state == PLAYER_STATE_PLAYING) {
        g_audio_session.state = PLAYER_STATE_PAUSED;
        audio_realtime_worker_stop();
        audio_hal_stop_stream();
    }
}

void audio_player_resume(void) {
    if (g_audio_session.state == PLAYER_STATE_PAUSED) {
        audio_hal_start_stream(48000, 2, 16);
        audio_realtime_worker_start();
        g_audio_session.state = PLAYER_STATE_PLAYING;
    }
}

void audio_player_stop(void) {
    if (g_audio_session.state == PLAYER_STATE_PLAYING || g_audio_session.state == PLAYER_STATE_PAUSED) {
        audio_player_close();
    }
}

void audio_player_close(void) {
    if (g_audio_session.state != PLAYER_STATE_STOPPED) {
        // Set state first to prevent background task from trying to read/pump
        g_audio_session.state = PLAYER_STATE_STOPPED;
        audio_realtime_worker_stop();
        audio_hal_stop_stream();
        audio_hal_shutdown();
        
        if (g_audio_session.fd >= 0) {
            vfs_close(g_audio_session.fd);
            g_audio_session.fd = -1;
        }
    }
}

#define PRODUCER_CHUNK_SIZE 16384
#define PRODUCER_HIGH_WATERMARK_PCT 80
#define PRODUCER_LOW_WATERMARK_PCT  40
#define PRODUCER_CRITICAL_WATERMARK_PCT 20
#define PRODUCER_REFILL_TARGET_PCT  90

void audio_player_update(void) {
    if (g_audio_session.state == PLAYER_STATE_STOPPED || g_audio_session.state == PLAYER_STATE_PAUSED) {
        return;
    }

        
    if (g_audio_session.bytes_played >= g_audio_session.data_size) {
        struct AudioStream* s = audio_core_get_stream(g_audio_session.stream_id);
        
        vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
        g_audio_session.bytes_played = 0;
    }
    extern uint64_t step14_cycles_to_us(uint64_t);
    uint64_t now_ticks = timer_get_ticks();
    if (g_last_producer_ticks != 0) {
        uint64_t gap_ms = step14_cycles_to_us(now_ticks - g_last_producer_ticks) / 1000;
        if (gap_ms > g_ProducerMaxServiceGapMs) {
            g_ProducerMaxServiceGapMs = gap_ms;
        }
    }
    g_last_producer_ticks = now_ticks;

    size_t available_bytes = audio_stream_available(g_audio_session.stream_id);
    if (available_bytes < g_AudioRingMinAvailable) g_AudioRingMinAvailable = available_bytes;

    size_t capacity_bytes = audio_stream_capacity(g_audio_session.stream_id);
    if (capacity_bytes == 0) return;

    size_t free_bytes = capacity_bytes - available_bytes;
    uint32_t occupancy_pct = (uint32_t)((available_bytes * 100) / capacity_bytes);

    // Staged Proactive Watermark Refill:
    // If occupancy falls below HIGH WATERMARK (80%) OR we are in initial prefill,
    // immediately start refilling staged 16KB non-blocking chunks without waiting for buffer to empty.
    if ((occupancy_pct < PRODUCER_HIGH_WATERMARK_PCT || g_audio_session.state == PLAYER_STATE_OPENED) && free_bytes >= PRODUCER_CHUNK_SIZE + audio_pcm_bytes_per_frame(&g_audio_session.format)) {
        uint32_t max_chunks = 2;
        if (g_audio_session.state == PLAYER_STATE_OPENED) {
            max_chunks = 16; // Fill up to 256KB during initial prefill
        } else if (occupancy_pct < PRODUCER_CRITICAL_WATERMARK_PCT) {
            max_chunks = 8;  // Emergency priority staged refill
        } else if (occupancy_pct < PRODUCER_LOW_WATERMARK_PCT) {
            max_chunks = 4;  // Aggressive staged refill
        }

        for (uint32_t c = 0; c < max_chunks; c++) {
                
    if (g_audio_session.bytes_played >= g_audio_session.data_size) {
                vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
                g_audio_session.bytes_played = 0;
            }

            available_bytes = audio_stream_available(g_audio_session.stream_id);
            free_bytes = capacity_bytes - available_bytes;
            if (free_bytes < PRODUCER_CHUNK_SIZE + audio_pcm_bytes_per_frame(&g_audio_session.format)) break;

            if (g_audio_session.state != PLAYER_STATE_OPENED) {
                occupancy_pct = (uint32_t)((available_bytes * 100) / capacity_bytes);
                if (occupancy_pct >= PRODUCER_REFILL_TARGET_PCT) break;
            }

            uint32_t to_read = PRODUCER_CHUNK_SIZE;
            if (g_audio_session.bytes_played + to_read > g_audio_session.data_size) {
                to_read = g_audio_session.data_size - g_audio_session.bytes_played;
            }
            if (audio_pcm_bytes_per_frame(&g_audio_session.format) > 0) {
                to_read = to_read - (to_read % audio_pcm_bytes_per_frame(&g_audio_session.format));
            }
            if (to_read == 0) continue;
                        uint64_t rt1 = timer_get_ticks();
            g_ProducerRefillCalls++;
            int read_bytes = 0;
            
            // === DIAGNOSTIC RAM-ONLY TEST PRODUCER ===
            if (g_RAM_Only_Test_Active) {
                g_RAMAudioRefillCalls++;
                
                uint32_t to_copy = to_read;
                if (g_ram_audio_cursor + to_copy > g_ram_audio_total_bytes) {
                    to_copy = g_ram_audio_total_bytes - g_ram_audio_cursor;
                }
                
                uint32_t chunk_idx = g_ram_audio_cursor / RAM_CHUNK_SIZE;
                uint32_t chunk_offset = g_ram_audio_cursor % RAM_CHUNK_SIZE;
                
                uint32_t copied = 0;
                while (copied < to_copy) {
                    uint32_t avail_in_chunk = RAM_CHUNK_SIZE - chunk_offset;
                    uint32_t copy_now = (to_copy - copied < avail_in_chunk) ? (to_copy - copied) : avail_in_chunk;
                    
                    extern void* memcpy(void*, const void*, size_t);
                    memcpy(g_read_buffer + copied, g_ram_audio_chunks[chunk_idx] + chunk_offset, copy_now);
                    
                    copied += copy_now;
                    chunk_idx++;
                    chunk_offset = 0;
                }
                
                g_ram_audio_cursor += to_copy;
                if (g_ram_audio_cursor >= g_ram_audio_total_bytes) {
                    g_ram_audio_cursor = 0; // Loop seamlessly
                }
                
                read_bytes = to_copy;
                g_RAMAudioBytesProduced += to_copy;
            } else {
                read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);
            }
            // =========================================

            if (read_bytes > 0) g_ProducerBytesRead += read_bytes;
            if (read_bytes > 0 && audio_pcm_bytes_per_frame(&g_audio_session.format) > 0 && (read_bytes % audio_pcm_bytes_per_frame(&g_audio_session.format)) != 0) {
                
                }

            uint64_t rt2 = timer_get_ticks();
            g_last_read_time_ticks = rt2 - rt1;
            
            if (read_bytes <= 0) {
                if (g_RAM_Only_Test_Active) {
                    g_ram_audio_cursor = 0;
                    g_audio_session.bytes_played = 0;
                } else {
                    vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
                    g_audio_session.bytes_played = 0;
                }
                break;
            }

            AudioPcmPacket packet;
            packet.format = g_audio_session.format;
            packet.frame_count = read_bytes / audio_pcm_bytes_per_frame(&g_audio_session.format);
            packet.timestamp = 0;
            packet.flags = 0;
            packet.pcm_data = g_read_buffer;
            packet.size_bytes = read_bytes;

                        size_t written = audio_stream_write(g_audio_session.stream_id, &packet);
            g_audio_session.bytes_played += read_bytes;
            if (written < (size_t)read_bytes) {
                break;
            }
        }
    }
}

bool audio_player_is_playing(void) {
    return g_audio_session.state == PLAYER_STATE_PLAYING;
}

void audio_player_get_diag_info(uint32_t* stream_id, uint32_t* bytes_played, uint32_t* data_size, uint64_t* read_time_ticks) {
    if (stream_id) *stream_id = g_audio_session.stream_id;
    if (bytes_played) *bytes_played = g_audio_session.bytes_played;
    if (data_size) *data_size = g_audio_session.data_size;
    if (read_time_ticks) *read_time_ticks = g_last_read_time_ticks;
}
