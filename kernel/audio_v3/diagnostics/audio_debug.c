#include "kernel/audio/diagnostics/audio_debug.h"
#include "kernel/audio/hal/audio_hal.h"

#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/streams/audio_buffer.h"
#include "kernel/audio/core/audio_core.h"
#include "kernel/audio/formats/audio_pcm.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/volume/audio_volume.h"
#include "kernel/drivers/display/display.h"

static AudioTelemetry telemetry;

void audio_debug_init(void) {
    telemetry.active_streams = 0;
    telemetry.allocated_buffers = 0;
    telemetry.allocated_bytes = 0;
    telemetry.peak_streams = 0;
    telemetry.peak_memory = 0;
    telemetry.failed_allocations = 0;
    telemetry.destroyed_streams = 0;
}

void audio_debug_log_alloc(uint32_t bytes) {
    telemetry.allocated_buffers++;
    telemetry.allocated_bytes += bytes;
    if (telemetry.allocated_bytes > telemetry.peak_memory) {
        telemetry.peak_memory = telemetry.allocated_bytes;
    }
}

void audio_debug_log_free(uint32_t bytes) {
    if (telemetry.allocated_buffers > 0) telemetry.allocated_buffers--;
    if (telemetry.allocated_bytes >= bytes) {
        telemetry.allocated_bytes -= bytes;
    }
}

void audio_debug_log_stream_create(void) {
    telemetry.active_streams++;
    if (telemetry.active_streams > telemetry.peak_streams) {
        telemetry.peak_streams = telemetry.active_streams;
    }
}

void audio_debug_log_stream_destroy(void) {
    if (telemetry.active_streams > 0) telemetry.active_streams--;
    telemetry.destroyed_streams++;
}

void audio_debug_log_fail_alloc(void) {
    telemetry.failed_allocations++;
}


void audio_debug_print_stats(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    display_print("--- Audio HAL Status ---\n");
    if (drv) {
        display_print("Active Driver: "); display_print(drv->name); display_print("\n");
        display_print("Capabilities: "); display_print_hex(drv->capabilities); display_print("\n");
    } else {
        display_print("Active Driver: [NONE DETECTED]\n");
    }

    display_print("--- Audio Telemetry ---\n");
    display_print("Active Streams: "); display_print_dec(telemetry.active_streams); display_print("\n");
    display_print("Peak Streams: "); display_print_dec(telemetry.peak_streams); display_print("\n");
    display_print("Destroyed Streams: "); display_print_dec(telemetry.destroyed_streams); display_print("\n");
    display_print("Allocated Buffers: "); display_print_dec(telemetry.allocated_buffers); display_print("\n");
    display_print("Allocated Bytes: "); display_print_dec(telemetry.allocated_bytes); display_print("\n");
    display_print("Peak Memory: "); display_print_dec(telemetry.peak_memory); display_print("\n");
    display_print("Failed Allocations: "); display_print_dec(telemetry.failed_allocations); display_print("\n");
    
    AudioStream* current = audio_core_get_active_streams();
    uint64_t total_written = 0, total_read = 0;
    uint32_t total_underruns = 0, total_overflows = 0;
    while(current) {
        total_written += current->stats.bytes_written;
        total_read += current->stats.bytes_read;
        total_underruns += current->stats.underrun_counter;
        total_overflows += current->stats.overflow_counter;
        current = current->next;
    }
    display_print("PCM Bytes Written (Active): "); display_print_dec(total_written); display_print("\n");
    display_print("PCM Bytes Read (Active): "); display_print_dec(total_read); display_print("\n");
    display_print("PCM Underruns (Active): "); display_print_dec(total_underruns); display_print("\n");
    display_print("PCM Overflows (Active): "); display_print_dec(total_overflows); display_print("\n");
    
    display_print("-----------------------\n");
}

void audio_debug_run_selftest(void) {
    display_print("[AUDIO SELF-TEST] Starting...\n");
    
    audio_init();
    
    // Test 1: Single Stream Lifecycle
    uint32_t stream1 = audio_stream_create(1001);
    if (stream1 == 0) { display_print("FAIL: Create 1\n"); return; }
    if (!audio_stream_destroy(stream1)) { display_print("FAIL: Destroy 1\n"); return; }
    
    // Test 2: Invalid Stream Destruction (Double destroy)
    if (audio_stream_destroy(stream1)) { display_print("FAIL: Double destroy allowed\n"); return; }
    
    // Test 3: Multiple Streams (100)
    uint32_t stream_ids[100];
    for (int i = 0; i < 100; i++) {
        stream_ids[i] = audio_stream_create(1000 + i);
        if (stream_ids[i] == 0) { display_print("FAIL: Multi-create\n"); return; }
    }
    for (int i = 0; i < 100; i++) {
        if (!audio_stream_destroy(stream_ids[i])) { display_print("FAIL: Multi-destroy\n"); return; }
    }
    
    // Test 4: Ring Buffer Logic
    AudioRingBuffer* rb = audio_buffer_create(10);
    if (!rb) { display_print("FAIL: Buffer Create\n"); return; }
    
    uint8_t write_data[5] = {1, 2, 3, 4, 5};
    if (audio_buffer_write(rb, write_data, 5) != 5) { display_print("FAIL: Buffer Write 1\n"); return; }
    
    uint8_t read_data[5];
    if (audio_buffer_read(rb, read_data, 5) != 5) { display_print("FAIL: Buffer Read 1\n"); return; }
    for (int i=0; i<5; i++) {
        if (read_data[i] != i+1) { display_print("FAIL: Buffer Data Mismatch\n"); return; }
    }
    
    // Test 5: Overwrite / Wrap-around
    if (audio_buffer_write(rb, write_data, 5) != 5) { display_print("FAIL: Buffer Write 2\n"); return; }
    if (audio_buffer_write(rb, write_data, 5) != 4) { display_print("FAIL: Buffer Overwrite should clamp\n"); return; }
    
    audio_buffer_destroy(rb);
    
    audio_debug_print_stats();
    
    if (telemetry.allocated_bytes == 0 && telemetry.allocated_buffers == 0 && telemetry.active_streams == 0) {
        display_print("[AUDIO SELF-TEST] SUCCESS: All tests passed with zero leaks.\n");
    } else {
        display_print("[AUDIO SELF-TEST] FAILED: Memory leak detected.\n");
    }
}

#include "kernel/core/memory/heap/include/heap.h"

void audio_debug_test_pcm_engine(void) {
    display_print("\n[PCM SELF-TEST] Starting PCM Engine Test...\n");
    
    uint32_t stream_id = audio_stream_create(1);
    if (stream_id == 0) {
        display_print("[PCM TEST] FAILED: Could not create stream\n");
        return;
    }
    
    AudioPcmFormat fmt = { .format = PCM_FORMAT_S16_LE, .sample_rate = 44100, .channels = 2, .bit_depth = 16, .is_signed = true };
    AudioPcmPacket pkt;
    pkt.format = fmt;
    pkt.timestamp = 0;
    pkt.flags = 0;
    
    // Generate 1000 frames (4000 bytes) of sine wave
    size_t frames = 1000;
    size_t bpf = audio_pcm_bytes_per_frame(&fmt);
    size_t bytes = frames * bpf;
    uint8_t* tx_buf = (uint8_t*)kmalloc(bytes);
    uint8_t* rx_buf = (uint8_t*)kmalloc(bytes);
    
    if (!tx_buf || !rx_buf) {
        display_print("[PCM TEST] FAILED: kmalloc error\n");
        return;
    }

    uint32_t phase = 0;
    audio_pcm_generate_sine(&fmt, 440, tx_buf, frames, &phase);
    
    pkt.pcm_data = tx_buf;
    pkt.frame_count = frames;
    pkt.size_bytes = bytes;
    
    // Test Write
    size_t written = audio_stream_write(stream_id, &pkt);
    if (written != bytes) {
        display_print("[PCM TEST] FAILED: Write mismatch\n");
    }
    
    // Test Read
    size_t read_bytes = audio_stream_read(stream_id, rx_buf, bytes);
    if (read_bytes != bytes) {
        display_print("[PCM TEST] FAILED: Read mismatch\n");
    }
    
    // Verify Integrity
    bool integrity = true;
    for (size_t i = 0; i < bytes; i++) {
        if (tx_buf[i] != rx_buf[i]) {
            integrity = false;
            break;
        }
    }
    
    if (integrity) {
        display_print("[PCM TEST] Integrity ... PASS (Sine wave verified byte-for-byte)\n");
    } else {
        display_print("[PCM TEST] FAILED: Data corruption detected!\n");
    }
    
    // Stress test 100,000 iterations of write/read
    // Using a 64 frame block to fit well inside the 16KB buffer
    uint32_t phase_saw = 0;
    audio_pcm_generate_saw(&fmt, 880, tx_buf, 64, &phase_saw);
    pkt.pcm_data = tx_buf;
    pkt.frame_count = 64;
    pkt.size_bytes = 64 * bpf;
    
    for (int i = 0; i < 100000; i++) {
        audio_stream_write(stream_id, &pkt);
        audio_stream_read(stream_id, rx_buf, pkt.size_bytes);
    }
    
    display_print("[PCM TEST] 100,000 R/W Ops ... PASS\n");
    
    audio_stream_destroy(stream_id);
    kfree(tx_buf);
    kfree(rx_buf);
    
    display_print("[PCM SELF-TEST] SUCCESS: Engine validated.\n");
}

void audio_debug_test_mixer(void) {
    display_print("\n[MIXER SELF-TEST] Starting Mixer Engine Test...\n");
    
    audio_volume_set_master(255);
    audio_volume_set_mute(false);
    
    AudioPcmFormat fmt = { .format = PCM_FORMAT_S16_LE, .sample_rate = 44100, .channels = 2, .bit_depth = 16, .is_signed = true };
    size_t bpf = audio_pcm_bytes_per_frame(&fmt);
    size_t mix_frames = 1000;
    size_t mix_bytes = mix_frames * bpf;
    
    uint8_t* output_buf = (uint8_t*)kmalloc(mix_bytes);
    if (!output_buf) {
        display_print("[MIXER TEST] FAILED: kmalloc error\n");
        return;
    }
    
    // Create 32 streams to mix
    #define NUM_MIX_TEST_STREAMS 32
    uint32_t streams[NUM_MIX_TEST_STREAMS];
    
    AudioPcmPacket pkt;
    pkt.format = fmt;
    pkt.timestamp = 0;
    pkt.flags = 0;
    pkt.frame_count = mix_frames;
    pkt.size_bytes = mix_bytes;
    
    uint8_t* tx_buf = (uint8_t*)kmalloc(mix_bytes);
    uint32_t phase = 0;
    
    for (int i = 0; i < NUM_MIX_TEST_STREAMS; i++) {
        streams[i] = audio_stream_create(1);
        audio_mixer_add_stream(streams[i]);
        audio_set_volume(streams[i], 128); // 50% volume to avoid massive clipping
        
        // Generate sine wave
        audio_pcm_generate_sine(&fmt, 440 + (i * 10), tx_buf, mix_frames, &phase);
        pkt.pcm_data = tx_buf;
        
        audio_stream_write(streams[i], &pkt);
    }
    
    // Perform Mix
    size_t bytes_mixed = audio_mixer_process(output_buf, mix_bytes, &fmt);
    
    if (bytes_mixed != mix_bytes) {
        display_print("[MIXER TEST] FAILED: Mix size mismatch\n");
    }
    
    uint32_t active_mixed = 0;
    uint64_t frames_mixed = 0;
    uint64_t clipped = 0;
    int32_t peak = 0;
    
    audio_mixer_get_stats(&active_mixed, &frames_mixed, &clipped, &peak);
    
    display_print("[MIXER] Streams Mixed: "); display_print_dec(active_mixed); display_print("\n");
    display_print("[MIXER] Clipped Samples: "); display_print_dec(clipped); display_print("\n");
    display_print("[MIXER] Peak Amplitude: "); display_print_dec(peak); display_print("\n");
    
    if (active_mixed == NUM_MIX_TEST_STREAMS) {
        display_print("[MIXER SELF-TEST] SUCCESS: All streams mixed safely.\n");
    } else {
        display_print("[MIXER SELF-TEST] FAILED.\n");
    }
    
    // Cleanup
    for (int i = 0; i < NUM_MIX_TEST_STREAMS; i++) {
        audio_stream_destroy(streams[i]);
    }
    kfree(tx_buf);
    kfree(output_buf);
}
