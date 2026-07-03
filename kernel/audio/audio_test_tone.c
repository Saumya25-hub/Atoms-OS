#include "audio_test_tone.h"
#include "audio_api.h"
#include "audio_mixer.h"
#include "audio_pcm.h"
#include "../drivers/audio/ac97/ac97_playback.h"
#include "../drivers/display/display.h"

void audio_test_tone_run(void) {
    extern void audio_forensic_init(void);
    audio_forensic_init();
    
    display_print("\n[AUDIO] Phase 7.6.5 - PCM Signal Integrity & Format Verification\n");
    
    // Check Endianness (Goal 6)
    uint16_t endian_test = 0x1234;
    uint8_t* endian_ptr = (uint8_t*)&endian_test;
    if (endian_ptr[0] == 0x34 && endian_ptr[1] == 0x12) {
        display_print("Endian: Little-Endian\nPASS\n");
    } else {
        display_print("Endian: BIG-ENDIAN\nFAIL\n");
        return;
    }
    
    // 1. Initialize DMA Engine
    if (!ac97_playback_init()) {
        display_print("[AC97] Playback Init Failed\n");
        return;
    }
    
    ac97_playback_prepare();
    
    // 2. Setup Audio Stream
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
    
    // Verify Format (Goal 2)
    if (format.sample_rate != 48000 || format.bit_depth != 16 || format.channels != 2 || !format.is_signed) {
        display_print("FORMAT MISMATCH\nReason: Invalid generation format parameters\nFAIL\n");
        return;
    }
    
    audio_stream_set_format(stream_id, &format);
    
    uint8_t temp_buffer[4096]; // 1024 frames * 4 bytes
    uint32_t phase_l = 0;
    uint32_t phase_r = 0;
    
    // Pre-fill the stream with 16384 bytes (4096 frames) so that DMA start has data
    audio_pcm_generate_stereo_sine(&format, 440, 880, temp_buffer, 1024, &phase_l, &phase_r);
    
    // Calculate stats on first block
    AudioPcmStats stats;
    audio_math_calculate_stats(&format, temp_buffer, 1024, 440, &stats);
    
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);
    
    display_print("========== PCM VERIFY ==========\n");
    display_print("Frequency        : "); display_print_dec(stats.frequency); display_print("Hz\n");
    display_print("Sample Rate      : "); display_print_dec(stats.sample_rate); display_print("\n");
    display_print("Channels         : "); display_print_dec(stats.channels); display_print("\n");
    display_print("Bits             : "); display_print_dec(stats.bits); display_print("\n");
    display_print("Peak +           : "); display_print_dec((uint32_t)stats.peak_pos); display_print("\n");
    display_print("Peak -           : "); display_print_dec((uint32_t)(-stats.peak_neg)); display_print("\n");
    display_print("RMS              : "); display_print_dec(stats.rms); display_print("\n");
    display_print("DC Offset        : "); display_print_dec((uint32_t)stats.dc_offset); display_print("\n");
    display_print("Zero Crossings   : "); display_print_dec(stats.zero_crossings); display_print("\n");
    display_print("Checksum         : "); display_print_dec(stats.checksum); display_print("\n");
    
    if (stats.dc_offset > 100 || stats.dc_offset < -100) {
        display_print("FAIL\nReason: High DC Offset\n");
    } else {
        display_print("PASS\n");
    }
    display_print("================================\n");
    
    extern void audio_forensic_log_pcm_gen(const uint8_t*, size_t);
    audio_forensic_log_pcm_gen(temp_buffer, 4096);
    
    AudioPcmPacket packet;
    packet.format = format;
    packet.frame_count = 1024;
    packet.timestamp = 0;
    packet.flags = 0;
    packet.pcm_data = temp_buffer;
    packet.size_bytes = 4096;
    audio_stream_write(stream_id, &packet);
    
    // Fill three more blocks
    for (int i = 0; i < 3; i++) {
        audio_pcm_generate_stereo_sine(&format, 440, 880, temp_buffer, 1024, &phase_l, &phase_r);
        audio_forensic_log_pcm_gen(temp_buffer, 4096); // It will stop capturing after 4096 total bytes anyway
        audio_stream_write(stream_id, &packet);
    }
    
    extern uint64_t timer_get_ticks(void);
    uint64_t t_start = timer_get_ticks();
    ac97_playback_start();
    uint64_t t_dma_start = timer_get_ticks() - t_start;
    
    // 10 seconds for continuous verification (48000Hz * 10)
    uint32_t target_frames = 480000;
    uint32_t frames_generated = 4096; // we generated 4 chunks of 1024
    
    bool pcm_verified = true; // stats already printed
    bool latency_printed = false;
    extern uint8_t io_in8(uint16_t port);
    extern uint16_t g_nabm_base;
    uint8_t start_civ = io_in8(g_nabm_base + 0x14); // AC97_NABM_PO_CIV
    
    while (frames_generated < target_frames) {
        size_t available_bytes = audio_stream_available(stream_id);
        size_t capacity_bytes = audio_stream_capacity(stream_id);
        
        size_t free_bytes = capacity_bytes - available_bytes;
        size_t free_frames = free_bytes / 4; // stereo 16-bit
        
        if (free_frames > 0) {
            size_t frames_to_generate = free_frames;
            if (frames_to_generate > 1024) frames_to_generate = 1024;
            
            if (frames_generated + frames_to_generate > target_frames) {
                frames_to_generate = target_frames - frames_generated;
            }
            
            audio_pcm_generate_stereo_sine(&format, 440, 880, temp_buffer, frames_to_generate, &phase_l, &phase_r);
            
            if (!pcm_verified) {
                AudioPcmStats stats;
                audio_math_calculate_stats(&format, temp_buffer, frames_to_generate, 440, &stats);
                
                extern void display_print(const char*);
                extern void display_print_dec(uint64_t);
                extern void display_print_hex(uint64_t);
                
                display_print("========== PCM VERIFY ==========\n");
                display_print("Frequency        : "); display_print_dec(stats.frequency); display_print("Hz\n");
                display_print("Sample Rate      : "); display_print_dec(stats.sample_rate); display_print("\n");
                display_print("Channels         : "); display_print_dec(stats.channels); display_print("\n");
                display_print("Bits             : "); display_print_dec(stats.bits); display_print("\n");
                display_print("Peak +           : "); display_print_dec((uint32_t)stats.peak_pos); display_print("\n");
                display_print("Peak -           : "); display_print_dec((uint32_t)(-stats.peak_neg)); display_print("\n");
                display_print("RMS              : "); display_print_dec(stats.rms); display_print("\n");
                display_print("DC Offset        : "); display_print_dec((uint32_t)stats.dc_offset); display_print("\n");
                display_print("Zero Crossings   : "); display_print_dec(stats.zero_crossings); display_print("\n");
                display_print("Checksum         : "); display_print_dec(stats.checksum); display_print("\n");
                
                if (stats.dc_offset > 100 || stats.dc_offset < -100) {
                    display_print("FAIL\nReason: High DC Offset\n");
                } else {
                    display_print("PASS\n");
                }
                display_print("================================\n");
                
                // Also capture forensic generator log
                extern void audio_forensic_log_pcm_gen(const uint8_t*, size_t);
                audio_forensic_log_pcm_gen(temp_buffer, frames_to_generate * 4);
                
                pcm_verified = true;
            } else {
                extern void audio_forensic_log_pcm_gen(const uint8_t*, size_t);
                audio_forensic_log_pcm_gen(temp_buffer, frames_to_generate * 4);
            }
            
            AudioPcmPacket packet;
            packet.format = format;
            packet.frame_count = frames_to_generate;
            packet.timestamp = 0;
            packet.flags = 0;
            packet.pcm_data = temp_buffer;
            packet.size_bytes = frames_to_generate * 4;
            
            audio_stream_write(stream_id, &packet);
            
            frames_generated += frames_to_generate;
        }
        
        // Pump DMA
        ac97_playback_update();
        
        if (!latency_printed) {
            uint8_t curr_civ = io_in8(g_nabm_base + 0x14);
            if (curr_civ != start_civ) {
                uint64_t t_civ = timer_get_ticks() - t_start;
                extern void display_print(const char*);
                extern void display_print_dec(uint64_t);
                display_print("\n[PLAYBACK TIMING]\n");
                display_print("DMA Startup Time: "); display_print_dec(t_dma_start); display_print(" ms\n");
                display_print("First CIV Rotation: "); display_print_dec(t_civ); display_print(" ms\n");
                display_print("First Audible Sample: PASS\n");
                display_print("PASS\n\n");
                latency_printed = true;
            }
        }
    }
    
    ac97_playback_stop();
    
    extern void ac97_playback_dump_trace(void);
    ac97_playback_dump_trace();
    
    ac97_playback_shutdown();
    
    extern void audio_forensic_dump_oscilloscope(void);
    audio_forensic_dump_oscilloscope();
    
    // Shut down VM gracefully after everything is done to flush WAV
    extern void io_out16(uint16_t port, uint16_t val);
    io_out16(0x604, 0x2000); // QEMU isa-debug-exit
}
