#include "audio_player.h"
#include "../vfs/vfs_legacy/include/vfs.h"
#include "audio_api.h"
#include "audio_mixer.h"
#include "../drivers/audio/ac97/ac97_playback.h"
#include "../drivers/display/display.h"

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

void audio_player_play(const char* path) {
    display_print("[PLAYER] Opening WAV...\n");
    int fd = vfs_open(path);
    if (fd < 0) {
        display_print("[PLAYER] Failed to open file\n");
        return;
    }
    
    RiffHeader riff;
    if (vfs_read(fd, &riff, sizeof(RiffHeader)) != sizeof(RiffHeader)) {
        display_print("[PLAYER]\nUnsupported WAV Format\n");
        vfs_close(fd);
        return;
    }
    
    if (riff.riff_id[0] != 'R' || riff.riff_id[1] != 'I' || riff.riff_id[2] != 'F' || riff.riff_id[3] != 'F' ||
        riff.wave_id[0] != 'W' || riff.wave_id[1] != 'A' || riff.wave_id[2] != 'V' || riff.wave_id[3] != 'E') {
        display_print("[PLAYER]\nUnsupported WAV Format\n");
        vfs_close(fd);
        return;
    }
    
    display_print("[PLAYER] Header OK\n");
    
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
        display_print("[PLAYER]\nUnsupported WAV Format\n");
        vfs_close(fd);
        return;
    }
    
    if (fmt.audio_format != 1 || fmt.num_channels != 2 || fmt.sample_rate != 48000 || fmt.bits_per_sample != 16) {
        display_print("[PLAYER]\nUnsupported WAV Format\n");
        vfs_close(fd);
        return;
    }
    
    display_print("[PLAYER] PCM Format OK\n");
    
    if (!ac97_playback_init()) {
        display_print("[PLAYER] HW Init Failed\n");
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
    
    display_print("[PLAYER] Streaming...\n");
    display_print("[PLAYER] Playback Started\n");
    
    uint32_t total_seconds = data_size / 192000;
    uint32_t bytes_played = 0;
    uint32_t last_ui_seconds = 0xFFFFFFFF;
    
    // Pre-fill the entire ring buffer before starting DMA
    static uint8_t read_buffer[65536];
    while (1) {
        size_t free_bytes = audio_stream_capacity(stream_id) - audio_stream_available(stream_id);
        // Leave a 4-byte margin to account for ring buffer's 1-byte empty indicator
        // preventing a 1-byte dropped write which causes permanent audio misalignment (white noise)
        if (free_bytes < 65536 + 4) break;
        
        uint32_t to_read = 65536;
        if (bytes_played + to_read > data_size) {
            to_read = data_size - bytes_played;
            if (to_read == 0) break;
        }
        
        int read_bytes = vfs_read(fd, read_buffer, to_read);
        if (read_bytes <= 0) break;
        
        AudioPcmPacket packet;
        packet.format = format;
        packet.frame_count = read_bytes / 4;
        packet.timestamp = 0;
        packet.flags = 0;
        packet.pcm_data = read_buffer;
        packet.size_bytes = read_bytes;
        audio_stream_write(stream_id, &packet);
        bytes_played += read_bytes;
    }
    
    display_print("[PLAYER] Buffer pre-filled\n");
    
    ac97_playback_start();
    
    display_clear();
    
    // Main streaming loop: pump DMA aggressively, feed from VFS when space available
    while (bytes_played < data_size) {
        // ALWAYS pump DMA first — this is the highest priority operation
        ac97_playback_update();
        
        size_t available_bytes = audio_stream_available(stream_id);
        size_t capacity_bytes = audio_stream_capacity(stream_id);
        size_t free_bytes = capacity_bytes - available_bytes;
        
        // Leave a 4-byte margin to prevent 1-byte write truncation
        if (free_bytes >= 65536 + 4) {
            uint32_t to_read = 65536;
            if (bytes_played + to_read > data_size) {
                to_read = data_size - bytes_played;
            }
            
            // Pump DMA before slow VFS read
            ac97_playback_update();
            
            int read_bytes = vfs_read(fd, read_buffer, to_read);
            
            if (read_bytes <= 0) break;
            
            AudioPcmPacket packet;
            packet.format = format;
            packet.frame_count = read_bytes / 4;
            packet.timestamp = 0;
            packet.flags = 0;
            packet.pcm_data = read_buffer;
            packet.size_bytes = read_bytes;
            
            audio_stream_write(stream_id, &packet);
            bytes_played += read_bytes;
            
            // Pump DMA AFTER writing to stream, so if it halted during VFS read,
            // it restarts using the newly read data instead of silence!
            ac97_playback_update();
        }
        
        // Lightweight UI update — only when seconds change
        uint32_t current_seconds = bytes_played / 192000;
        if (current_seconds != last_ui_seconds) {
            last_ui_seconds = current_seconds;
            uint32_t progress = (bytes_played * 10) / data_size;
            if (progress > 10) progress = 10;
            
            display_set_cursor(0, 0);
            display_print("+----------------------------------+\n");
            display_print("|        ATOMS Music Player        |\n");
            display_print("+----------------------------------+\n\n");
            display_print("Now Playing:\nDEMO1.wav\n\n");
            display_print("Status:\nPlaying...\n\n");
            
            display_print("Progress:\n");
            for (uint32_t p = 0; p < 10; p++) {
                if (p < progress) display_print("\xDB"); 
                else display_print("\xB0");
            }
            display_print("\n\nTime:\n");
            
            uint32_t cur_min = current_seconds / 60;
            uint32_t cur_sec = current_seconds % 60;
            uint32_t tot_min = total_seconds / 60;
            uint32_t tot_sec = total_seconds % 60;
            
            if (cur_min < 10) display_print("0"); display_print_dec(cur_min); display_print(":");
            if (cur_sec < 10) display_print("0"); display_print_dec(cur_sec); display_print(" / ");
            if (tot_min < 10) display_print("0"); display_print_dec(tot_min); display_print(":");
            if (tot_sec < 10) display_print("0"); display_print_dec(tot_sec); display_print("    \n");
        }
    }
    
    // Drain remaining data in stream
    for (int drain = 0; drain < 5000; drain++) {
        ac97_playback_update();
        if (audio_stream_available(stream_id) == 0) break;
    }
    
    ac97_playback_stop();
    
    // Print telemetry BEFORE shutdown so counters are still valid
    display_print("\n[PLAYER]\nPlayback Complete\n");
    ac97_playback_status();
    
    // Stream telemetry
    display_print("\n======== STREAM TELEMETRY ========\n");
    display_print("Ring Buf Capacity : "); display_print_dec(audio_stream_capacity(stream_id)); display_print("\n");
    display_print("Ring Buf Remain   : "); display_print_dec(audio_stream_available(stream_id)); display_print("\n");
    display_print("Bytes Fed to Strm : "); display_print_dec(bytes_played); display_print("\n");
    display_print("Data Size (WAV)   : "); display_print_dec(data_size); display_print("\n");
    display_print("==================================\n");
    
    ac97_playback_shutdown();
    
    vfs_close(fd);
}
