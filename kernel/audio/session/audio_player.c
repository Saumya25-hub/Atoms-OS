#include "kernel/audio/session/audio_player.h"
#include "kernel/audio/core/audio_core.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/hal/audio_hal.h"
#include "kernel/audio/codecs/codec_registry.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

extern uint64_t timer_get_ticks(void);
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
    bos_audio_codec_handle_t* codec_handle;
    uint8_t* file_buffer;
    size_t file_buffer_size;
} AudioSession;

static AudioSession g_audio_session = {
    .state = PLAYER_STATE_STOPPED,
    .fd = -1,
    .stream_id = 0,
    .data_size = 0,
    .bytes_played = 0,
    .data_offset = 44,
    .codec_handle = NULL,
    .file_buffer = NULL,
    .file_buffer_size = 0
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
    display_print("RingMinAvailable  : "); display_print_dec(g_AudioRingMinAvailable); display_print("\n");
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
    uint32_t remaining = size;
    uint8_t dummy[512];
    while (remaining > 0) {
        uint32_t to_read = (remaining < sizeof(dummy)) ? remaining : sizeof(dummy);
        if (!audio_player_read_exact(fd, dummy, to_read, cursor)) {
            return false;
        }
        remaining -= to_read;
    }
    return true;
}

void audio_player_open(const char* path) {
    display_print("[003] Entered audio_player_open()\n");
    if (g_audio_session.state != PLAYER_STATE_STOPPED) {
        audio_player_close();
    }
    
    if (!path) return;

    display_print("[004] Calling vfs_open: ");
    display_print(path);
    display_print("\n");

    int fd = vfs_open(path);
    if (fd < 0) {
        display_print("[AUDIO_PLAYER] vfs_open failed! Result: fd < 0\n");
        return;
    }
    display_print("[005] vfs_open() returned successfully\n");

    int file_size = vfs_seek(fd, 0, 2 /* SEEK_END */);
    vfs_seek(fd, 0, 0 /* SEEK_SET */);

    if (file_size <= 0) {
        display_print("[AUDIO_PLAYER] File empty or unseekable\n");
        vfs_close(fd);
        return;
    }

    uint8_t* file_buf = (uint8_t*)kmalloc((size_t)file_size);
    if (!file_buf) {
        display_print("[AUDIO_PLAYER] Failed to allocate memory for file\n");
        vfs_close(fd);
        return;
    }

    int bytes_read = vfs_read(fd, file_buf, (uint32_t)file_size);
    vfs_close(fd);

    if (bytes_read <= 0) {
        display_print("[AUDIO_PLAYER] Failed to read audio file bytes\n");
        kfree(file_buf);
        return;
    }

    /* Try Codec Registry */
    bos_audio_codec_handle_t* codec_handle = codec_registry_open(file_buf, (size_t)bytes_read, path);
    if (codec_handle) {
        display_print("[AUDIO_PLAYER] Codec detected: ");
        display_print(codec_handle->info.codec_name);
        display_print(" (");
        display_print_dec(codec_handle->info.sample_rate);
        display_print(" Hz, ");
        display_print_dec(codec_handle->info.channels);
        display_print(" ch)\n");

        uint32_t stream_id = audio_stream_create(0);
        audio_set_volume(stream_id, 255);
        audio_mixer_add_stream(stream_id);

        AudioPcmFormat format = {
            .format = PCM_FORMAT_S16_LE,
            .sample_rate = codec_handle->info.sample_rate,
            .channels = codec_handle->info.channels,
            .bit_depth = 16,
            .is_signed = true
        };
        audio_stream_set_format(stream_id, &format);

        g_audio_session.codec_handle = codec_handle;
        g_audio_session.file_buffer = file_buf;
        g_audio_session.file_buffer_size = (size_t)bytes_read;
        g_audio_session.fd = -1;
        g_audio_session.stream_id = stream_id;
        g_audio_session.format = format;
        g_audio_session.data_size = (uint32_t)(codec_handle->info.total_samples * format.channels * 2);
        g_audio_session.bytes_played = 0;
        g_audio_session.state = PLAYER_STATE_OPENED;
        return;
    }

    /* Fallback to legacy WAV chunk parser */
    kfree(file_buf);

    fd = vfs_open(path);
    if (fd < 0) return;

    RiffHeader riff;
    uint64_t file_cursor = 0;
    if (!audio_player_read_exact(fd, &riff, sizeof(RiffHeader), &file_cursor)) {
        vfs_close(fd);
        return;
    }
    
    if (riff.riff_id[0] != 'R' || riff.riff_id[1] != 'I' || riff.riff_id[2] != 'F' || riff.riff_id[3] != 'F' ||
        riff.wave_id[0] != 'W' || riff.wave_id[1] != 'A' || riff.wave_id[2] != 'V' || riff.wave_id[3] != 'E') {
        display_print("[AUDIO_PLAYER] Unsupported format!\n");
        vfs_close(fd);
        return;
    }
    
    FmtChunk fmt = {0};
    bool found_fmt = false;
    uint32_t data_size = 0;
    uint64_t data_offset = 0;
    
    while (1) {
        ChunkHeader chunk;
        if (!audio_player_read_exact(fd, &chunk, sizeof(ChunkHeader), &file_cursor)) break;
        
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
        vfs_close(fd);
        return;
    }

    uint32_t stream_id = audio_stream_create(0);
    audio_set_volume(stream_id, 255);
    audio_mixer_add_stream(stream_id);
    
    AudioPcmFormat format = {
        .format = PCM_FORMAT_S16_LE,
        .sample_rate = fmt.sample_rate,
        .channels = (uint8_t)fmt.num_channels,
        .bit_depth = (uint8_t)fmt.bits_per_sample,
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
        audio_forensic_reset();
        
        display_print("[016] Starting prefill loop\n");
        while (audio_stream_capacity(g_audio_session.stream_id) - audio_stream_available(g_audio_session.stream_id) >= 16384 + audio_pcm_bytes_per_frame(&g_audio_session.format) &&
               (g_audio_session.codec_handle || g_audio_session.bytes_played < g_audio_session.data_size)) {
            audio_player_update();
            if (audio_stream_available(g_audio_session.stream_id) >= 32768) break;
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
        g_audio_session.state = PLAYER_STATE_STOPPED;
        audio_realtime_worker_stop();
        audio_hal_stop_stream();
        audio_hal_shutdown();
        
        if (g_audio_session.codec_handle) {
            g_audio_session.codec_handle->driver->close(g_audio_session.codec_handle);
            g_audio_session.codec_handle = NULL;
        }
        if (g_audio_session.file_buffer) {
            kfree(g_audio_session.file_buffer);
            g_audio_session.file_buffer = NULL;
            g_audio_session.file_buffer_size = 0;
        }
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

    /* Universal Codec Path */
    if (g_audio_session.codec_handle) {
        size_t available_bytes = audio_stream_available(g_audio_session.stream_id);
        size_t capacity_bytes = audio_stream_capacity(g_audio_session.stream_id);
        if (capacity_bytes == 0) return;
        size_t free_bytes = capacity_bytes - available_bytes;

        while (free_bytes >= 4096) {
            static int16_t decode_buf[2048];
            size_t max_s = sizeof(decode_buf) / sizeof(int16_t);
            size_t decoded = g_audio_session.codec_handle->driver->decode(
                g_audio_session.codec_handle, decode_buf, max_s);

            if (decoded == 0) {
                /* Loop back to beginning */
                g_audio_session.codec_handle->driver->seek(g_audio_session.codec_handle, 0);
                g_audio_session.bytes_played = 0;
                break;
            }

            AudioPcmPacket packet;
            packet.format = g_audio_session.format;
            packet.frame_count = decoded / g_audio_session.format.channels;
            packet.timestamp = 0;
            packet.flags = 0;
            packet.pcm_data = (uint8_t*)decode_buf;
            packet.size_bytes = decoded * sizeof(int16_t);

            size_t written = audio_stream_write(g_audio_session.stream_id, &packet);
            g_audio_session.bytes_played += packet.size_bytes;

            available_bytes = audio_stream_available(g_audio_session.stream_id);
            free_bytes = capacity_bytes - available_bytes;
            if (written < packet.size_bytes) break;
        }
        return;
    }

    /* Legacy WAV Stream Path */
    if (g_audio_session.bytes_played >= g_audio_session.data_size) {
        vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
        g_audio_session.bytes_played = 0;
    }

    size_t available_bytes = audio_stream_available(g_audio_session.stream_id);
    if (available_bytes < g_AudioRingMinAvailable) g_AudioRingMinAvailable = available_bytes;

    size_t capacity_bytes = audio_stream_capacity(g_audio_session.stream_id);
    if (capacity_bytes == 0) return;

    size_t free_bytes = capacity_bytes - available_bytes;
    uint32_t occupancy_pct = (uint32_t)((available_bytes * 100) / capacity_bytes);

    if ((occupancy_pct < PRODUCER_HIGH_WATERMARK_PCT || g_audio_session.state == PLAYER_STATE_OPENED) && free_bytes >= PRODUCER_CHUNK_SIZE + audio_pcm_bytes_per_frame(&g_audio_session.format)) {
        uint32_t max_chunks = (g_audio_session.state == PLAYER_STATE_OPENED) ? 16 : 4;

        for (uint32_t c = 0; c < max_chunks; c++) {
            if (g_audio_session.bytes_played >= g_audio_session.data_size) {
                vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
                g_audio_session.bytes_played = 0;
            }

            available_bytes = audio_stream_available(g_audio_session.stream_id);
            free_bytes = capacity_bytes - available_bytes;
            if (free_bytes < PRODUCER_CHUNK_SIZE + audio_pcm_bytes_per_frame(&g_audio_session.format)) break;

            uint32_t to_read = PRODUCER_CHUNK_SIZE;
            if (g_audio_session.bytes_played + to_read > g_audio_session.data_size) {
                to_read = g_audio_session.data_size - g_audio_session.bytes_played;
            }
            if (audio_pcm_bytes_per_frame(&g_audio_session.format) > 0) {
                to_read = to_read - (to_read % audio_pcm_bytes_per_frame(&g_audio_session.format));
            }
            if (to_read == 0) continue;

            int read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);
            if (read_bytes <= 0) {
                vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
                g_audio_session.bytes_played = 0;
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
            if (written < (size_t)read_bytes) break;
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
