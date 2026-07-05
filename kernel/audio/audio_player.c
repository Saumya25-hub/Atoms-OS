#include "audio_player.h"
#include "../vfs/vfs_legacy/include/vfs.h"
#include "audio_api.h"
#include "audio_mixer.h"
#include "../drivers/audio/ac97/ac97_playback.h"
#include "../drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

#define display_print(x)
#define display_clear()
#define display_set_cursor(x, y)
#define display_print_dec(x)

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
} AudioSession;

static AudioSession g_audio_session = {
    .state = PLAYER_STATE_STOPPED,
    .fd = -1,
    .stream_id = 0,
    .data_size = 0,
    .bytes_played = 0
};

static uint8_t g_read_buffer[32768];

void audio_player_open(const char* path) {
    if (g_audio_session.state != PLAYER_STATE_STOPPED) {
        audio_player_close();
    }
    
    int fd = vfs_open(path);
    if (fd < 0) return;
    
    RiffHeader riff;
    if (vfs_read(fd, &riff, sizeof(RiffHeader)) != sizeof(RiffHeader)) {
        vfs_close(fd);
        return;
    }
    
    if (riff.riff_id[0] != 'R' || riff.riff_id[1] != 'I' || riff.riff_id[2] != 'F' || riff.riff_id[3] != 'F' ||
        riff.wave_id[0] != 'W' || riff.wave_id[1] != 'A' || riff.wave_id[2] != 'V' || riff.wave_id[3] != 'E') {
        vfs_close(fd);
        return;
    }
    
    FmtChunk fmt = {0};
    bool found_fmt = false;
    uint32_t data_size = 0;
    
    while (1) {
        ChunkHeader chunk;
        if (vfs_read(fd, &chunk, sizeof(ChunkHeader)) != sizeof(ChunkHeader)) {
            break;
        }
        
        if (chunk.id[0] == 'f' && chunk.id[1] == 'm' && chunk.id[2] == 't' && chunk.id[3] == ' ') {
            if (vfs_read(fd, &fmt, sizeof(FmtChunk)) != sizeof(FmtChunk)) break;
            found_fmt = true;
            if (chunk.size > sizeof(FmtChunk)) {
                uint8_t dummy;
                for (uint32_t i = 0; i < chunk.size - sizeof(FmtChunk); i++) {
                    vfs_read(fd, &dummy, 1);
                }
            }
        } else if (chunk.id[0] == 'd' && chunk.id[1] == 'a' && chunk.id[2] == 't' && chunk.id[3] == 'a') {
            data_size = chunk.size;
            break; 
        } else {
            uint8_t dummy;
            for (uint32_t i = 0; i < chunk.size; i++) {
                vfs_read(fd, &dummy, 1);
            }
        }
    }
    
    if (!found_fmt || data_size == 0) {
        vfs_close(fd);
        return;
    }
    
    if (fmt.audio_format != 1 || fmt.num_channels != 2 || fmt.sample_rate != 48000 || fmt.bits_per_sample != 16) {
        vfs_close(fd);
        return;
    }
    
    if (!ac97_playback_init()) {
        vfs_close(fd);
        return;
    }
    ac97_playback_prepare();
    
    uint32_t stream_id = audio_stream_create(0);
    audio_set_volume(stream_id, 255);
    audio_mixer_add_stream(stream_id);
    
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
    g_audio_session.state = PLAYER_STATE_OPENED;
}

void audio_player_play(void) {
    if (g_audio_session.state == PLAYER_STATE_OPENED) {
        // Remove pre-fill buffer to prevent ATA driver reentrancy 
        // since AudioSvc runs audio_player_update in background thread
        ac97_playback_start();
        g_audio_session.state = PLAYER_STATE_PLAYING;
    }
}

void audio_player_pause(void) {
    if (g_audio_session.state == PLAYER_STATE_PLAYING) {
        g_audio_session.state = PLAYER_STATE_PAUSED;
        ac97_playback_stop();
    }
}

void audio_player_resume(void) {
    if (g_audio_session.state == PLAYER_STATE_PAUSED) {
        ac97_playback_start();
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
        
        ac97_playback_stop();
        ac97_playback_shutdown();
        
        if (g_audio_session.fd >= 0) {
            vfs_close(g_audio_session.fd);
            g_audio_session.fd = -1;
        }
    }
}

void audio_player_update(void) {
    if (g_audio_session.state == PLAYER_STATE_STOPPED || g_audio_session.state == PLAYER_STATE_PAUSED) {
        return;
    }

    if (g_audio_session.state == PLAYER_STATE_PLAYING) {
        // Pump DMA buffer first to minimize latency
        ac97_playback_update();
    }

    if (g_audio_session.bytes_played >= g_audio_session.data_size) {
        // Check if stream is empty before fully closing
        if (audio_stream_available(g_audio_session.stream_id) == 0) {
            audio_player_close();
        }
        return;
    }

    // Attempt to refill the stream from VFS in chunks of up to 32KB
    size_t available_bytes = audio_stream_available(g_audio_session.stream_id);
    size_t capacity_bytes = audio_stream_capacity(g_audio_session.stream_id);
    size_t free_bytes = capacity_bytes - available_bytes;

    // Leave a 4-byte margin to prevent truncation issues
    if (free_bytes >= sizeof(g_read_buffer) + 4) {
        uint32_t to_read = sizeof(g_read_buffer);
        if (g_audio_session.bytes_played + to_read > g_audio_session.data_size) {
            to_read = g_audio_session.data_size - g_audio_session.bytes_played;
        }

        int read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);
        if (read_bytes <= 0) {
            g_audio_session.data_size = g_audio_session.bytes_played; // Force EOF
            return;
        }

        AudioPcmPacket packet;
        packet.format = g_audio_session.format;
        packet.frame_count = read_bytes / 4;
        packet.timestamp = 0;
        packet.flags = 0;
        packet.pcm_data = g_read_buffer;
        packet.size_bytes = read_bytes;

        audio_stream_write(g_audio_session.stream_id, &packet);
        g_audio_session.bytes_played += read_bytes;

        if (g_audio_session.state == PLAYER_STATE_PLAYING) {
            ac97_playback_update(); // Pump again in case it starved during slow read
        }
    }
}

bool audio_player_is_playing(void) {
    return g_audio_session.state == PLAYER_STATE_PLAYING;
}
