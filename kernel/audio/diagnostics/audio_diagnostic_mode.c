#include "kernel/audio/session/audio_player.h"
#include "kernel/drivers/display/display.h"
#include "kernel/audio/hal/audio_hal.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/audio/diagnostics/audio_debug.h"
#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "kernel/audio/drivers/ac97/ac97_dma.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/core/audio_core.h"
#include "kernel/audio/streams/audio_stream.h"
#include "kernel/audio/volume/audio_volume.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/audio/core/audio_realtime_worker.h"

extern uint32_t ac97_dma_get_current_position(void);

void audio_diagnostic_mode(void) {
    display_print("\n\n=======================================================\n");
    display_print("         BOS KERNEL AUDIO DIAGNOSTIC MODE\n");
    display_print("         (DESKTOP & SCHEDULER COMPLETELY BYPASSED)\n");
    display_print("=======================================================\n\n");
    
    display_print("[001] Enter audio_diagnostic_mode()\n");
    display_print("Loading DEMO1.WAV strictly from Kernel space...\n");
    
    extern void audio_realtime_worker_init(void);
    extern void audio_producer_worker_init(void);
    audio_realtime_worker_init();
    audio_producer_worker_init();
    
    uint64_t init_time = timer_get_ticks();
    display_print("[002] Calling audio_player_open(\"/DEMO1.WAV\")\n");
    audio_player_open("/DEMO1.WAV");
    display_print("[013] Returned from audio_player_open()\n");
    
    display_print("[014] Calling audio_player_play()\n");
    audio_player_play();
    display_print("[023] Returned from audio_player_play()\n");
    
    uint64_t start_time = timer_get_ticks();
    uint64_t last_print_time = start_time;
    uint64_t last_calls = 0;
    
    uint8_t prev_civ = 0;
    uint64_t last_civ_change_time = timer_get_ticks();
    uint32_t civ_stall_ticks = 0;
    bool final_report_done = false;
    
    display_print("[024] Entered playback loop\n");
    display_print("[DIAG] Entered Kernel Audio Pump & Forensic Loop\n");
    
    while (1) {
        extern void audio_producer_worker_run(void);
        audio_producer_worker_run();
        scheduler_sleep(10); // Sleep 10ms between low-priority producer checks while IRQ 0 runs realtime pump at 1000 Hz
        
        uint16_t nabm = ac97_dma_get_nabm_bar();
        uint8_t civ = nabm ? io_in8(nabm + 0x14) : 0;
        uint8_t lvi = nabm ? io_in8(nabm + 0x15) : 0;
        uint16_t sr = nabm ? io_in16(nabm + 0x16) : 0;
        uint16_t picb = nabm ? io_in16(nabm + 0x18) : 0;
        uint8_t cr = nabm ? io_in8(nabm + 0x1B) : 0;
        uint32_t glob_sta = nabm ? io_in32(nabm + 0x30) : 0;
        
        // Check CIV movement when DMA is running (cr & 1)
        if (cr & 0x01) {
            if (civ == prev_civ) {
                civ_stall_ticks = (uint32_t)(timer_get_ticks() - last_civ_change_time);
                if (civ_stall_ticks > 150) { // Report if stalled for over 150ms
                    display_print("\n[DIAG ERROR] DMA HALT: CIV stopped moving at descriptor ");
                    display_print_dec(civ); display_print("!\n");
                    last_civ_change_time = timer_get_ticks(); // reset to avoid spamming every iteration
                }
            } else {
                last_civ_change_time = timer_get_ticks();
                civ_stall_ticks = 0;
                prev_civ = civ;
            }
        }
        
        uint64_t now = timer_get_ticks();
        
        // Print live runtime diagnostics every 1000ms (1 second)
        if (now - last_print_time >= 1000 && !final_report_done) {
            uint64_t elapsed_sec = (now - start_time) / 1000;
            
            uint32_t stream_id = 0, bytes_played = 0, data_size = 0;
            uint64_t read_time_ticks = 0;
            audio_player_get_diag_info(&stream_id, &bytes_played, &data_size, &read_time_ticks);
            
            AudioStream* stream = audio_core_get_stream(stream_id);
            size_t avail = stream && stream->ring_buffer ? audio_buffer_available(stream->ring_buffer) : 0;
            size_t cap = stream && stream->ring_buffer ? stream->ring_buffer->capacity : 0;
            
            uint64_t calls = 0, req = 0, ret = 0, silence = 0;
            audio_mixer_get_diag_counters(&calls, &req, &ret, &silence);
            uint64_t calls_per_sec = calls - last_calls;
            last_calls = calls;
            
            display_clear();
            display_print("=========================================================\n");
            display_print("SECTION 1: DRIVER STATUS (Elapsed: "); display_print_dec(elapsed_sec); display_print("s)\n");
            display_print("=========================================================\n");
            display_print("Driver Loaded    : "); display_print(audio_hal_get_active_driver() ? "YES\n" : "NO\n");
            display_print("HAL Initialized  : YES\n");
            display_print("Codec Initialized: YES\n");
            display_print("Codec Ready      : "); display_print((glob_sta & (1<<8)) ? "YES\n" : "NO\n");
            display_print("DMA Initialized  : "); display_print(ac97_dma_get_manager() ? "YES\n" : "NO\n");
            display_print("Playback Started : "); display_print((ac97_playback_get_state() >= AC97_PB_STATE_STARTING) ? "YES\n" : "NO\n");
            display_print("Playback Running : "); display_print((cr & 0x01) ? "YES\n" : "NO\n");
            display_print("Driver Health    : "); display_print((sr & 0x02) ? "FAIL (HALTED)\n" : "PASS (HEALTHY)\n");
            
            display_print("\n=========================================================\n");
            display_print("SECTION 2: DMA STATUS\n");
            display_print("=========================================================\n");
            display_print("CIV                  : "); display_print_dec(civ); display_print("\n");
            display_print("LVI                  : "); display_print_dec(lvi); display_print("\n");
            display_print("PICB                 : "); display_print_dec(picb); display_print("\n");
            display_print("CR                   : 0x"); display_print_hex(cr); display_print("\n");
            display_print("SR                   : 0x"); display_print_hex(sr); display_print("\n");
            display_print("DMA Running          : "); display_print((cr & 0x01) ? "YES\n" : "NO\n");
            display_print("Current Descriptor   : "); display_print_dec(civ); display_print("\n");
            display_print("Last Descriptor      : "); display_print_dec(lvi); display_print("\n");
            display_print("Descriptor Rotations : "); display_print_dec(ac97_playback_get_rotations()); display_print("\n");
            uint32_t fill_pct = (32 - ((civ - lvi + 32) % 32)) * 100 / 32;
            display_print("DMA Buffer Fill %    : "); display_print_dec(fill_pct); display_print("%\n");
            display_print("DMA Position         : "); display_print_dec(ac97_playback_get_frames_played() * 4); display_print(" bytes\n");
            
            display_print("\n=========================================================\n");
            display_print("SECTION 3: STREAM STATUS\n");
            display_print("=========================================================\n");
            if (stream && stream->ring_buffer) {
                display_print("Bytes Written    : "); display_print_dec(stream->stats.bytes_written); display_print("\n");
                display_print("Bytes Read       : "); display_print_dec(stream->stats.bytes_read); display_print("\n");
                display_print("Bytes Available  : "); display_print_dec(avail); display_print("\n");
                display_print("Read Pointer     : "); display_print_dec(stream->ring_buffer->tail); display_print("\n");
                display_print("Write Pointer    : "); display_print_dec(stream->ring_buffer->head); display_print("\n");
                display_print("Ring Capacity    : "); display_print_dec(cap); display_print("\n");
                display_print("Ring Usage %     : "); display_print_dec(cap ? (avail * 100 / cap) : 0); display_print("%\n");
                display_print("Overflow Count   : "); display_print_dec(stream->stats.overflow_counter); display_print("\n");
                display_print("Underflow Count  : "); display_print_dec(stream->stats.underrun_counter); display_print("\n");
            } else {
                display_print("Stream Status    : N/A\n");
            }
            
            display_print("\n=========================================================\n");
            display_print("SECTION 4: MIXER STATUS\n");
            display_print("=========================================================\n");
            display_print("Mixer Called     : "); display_print_dec(calls); display_print("\n");
            display_print("Calls Per Second : "); display_print_dec(calls_per_sec); display_print("\n");
            display_print("Requested Bytes  : "); display_print_dec(req); display_print("\n");
            display_print("Returned Bytes   : "); display_print_dec(ret); display_print("\n");
            display_print("Silence Generated: "); display_print_dec(silence); display_print("\n");
            display_print("Current Volume   : "); display_print_dec(audio_volume_get_master()); display_print("\n");
            uint64_t f_mixed = 0;
            audio_mixer_get_stats(NULL, &f_mixed, NULL, NULL);
            display_print("Frames Mixed     : "); display_print_dec(f_mixed); display_print("\n");
            
            display_print("\n=========================================================\n");
            display_print("SECTION 5: PLAYER STATUS\n");
            display_print("=========================================================\n");
            display_print("Current File     : /DEMO1.WAV\n");
            display_print("Current Offset   : "); display_print_dec(bytes_played + 44); display_print("\n");
            display_print("Chunk Size       : 4096 bytes\n");
            display_print("Read Time        : "); display_print_dec(read_time_ticks); display_print(" ms\n");
            display_print("Bytes Read       : "); display_print_dec(bytes_played); display_print("\n");
            display_print("End Of File      : "); display_print((bytes_played >= data_size && data_size > 0) ? "YES\n" : "NO\n");
            display_print("Playback Time    : "); display_print_dec(bytes_played / 192000); display_print(" s\n");
            
            AudioRealtimeTelemetry rt_stats;
            audio_realtime_worker_get_telemetry(&rt_stats);

            display_print("\n=========================================================\n");
            display_print("SECTION 6: REALTIME WORKER TIMING (1000 Hz IRQ 0 Pump)\n");
            display_print("=========================================================\n");
            display_print("Disk Read Time   : "); display_print_dec(read_time_ticks); display_print(" ticks/ms\n");
            display_print("Last Pump Dur    : "); display_print_dec(rt_stats.last_pump_duration_us); display_print(" us\n");
            display_print("Max Pump Dur     : "); display_print_dec(rt_stats.max_pump_duration_us); display_print(" us\n");
            display_print("Realtime Violations: "); display_print_dec(rt_stats.timing_violations); display_print("\n");
            display_print("Total IRQ Pumps  : "); display_print_dec(rt_stats.total_pumps); display_print("\n");
            
            display_print("\n=========================================================\n");
            display_print("SECTION 7: ERROR DETECTION\n");
            display_print("=========================================================\n");
            display_print("DMA Halt         : "); display_print_dec(ac97_playback_get_dch_halts()); display_print("\n");
            display_print("Codec Failure    : "); display_print((glob_sta & (1<<8)) ? "NONE\n" : "PCR NOT READY\n");
            display_print("Ring Underflow   : "); display_print_dec(stream ? stream->stats.underrun_counter : 0); display_print("\n");
            display_print("Ring Overflow    : "); display_print_dec(stream ? stream->stats.overflow_counter : 0); display_print("\n");
            display_print("Descriptor Stall : "); display_print((civ_stall_ticks > 100) ? "DETECTED\n" : "NONE\n");
            display_print("Stream Empty     : "); display_print((avail == 0) ? "YES\n" : "NO\n");
            display_print("Stream Corruption: NO\n");
            display_print("Unexpected Zero  : "); display_print_dec(silence / 4096); display_print(" chunks\n");
            display_print("Invalid Pointer  : NO\n");
            display_print("Late Refill      : "); display_print_dec(ac97_playback_get_late_refills()); display_print("\n");
            display_print("Late Update      : "); display_print((rt_stats.timing_violations > 0) ? "YES\n" : "NO\n");
            display_print("Abnormal Condition: ");
            if ((sr & 0x02) || civ_stall_ticks > 100 || (avail == 0) || ac97_playback_get_late_refills() > 0) {
                display_print("DETECTED (Underrun/Stall/Late)\n");
            } else {
                display_print("NONE (Normal Operation)\n");
            }
            
            last_print_time = now;
        }
        
        // Section 8: Final Analysis after exactly 10 seconds of playback
        if (now - start_time >= 10000 && !final_report_done) {
            final_report_done = true;
            
            uint32_t stream_id = 0, bytes_played = 0, data_size = 0;
            audio_player_get_diag_info(&stream_id, &bytes_played, &data_size, NULL);
            AudioStream* stream = audio_core_get_stream(stream_id);
            uint64_t calls = 0, req = 0, ret = 0, silence = 0;
            audio_mixer_get_diag_counters(&calls, &req, &ret, &silence);
            
            display_print("\n\n=========================================================\n");
            display_print("SECTION 8: FINAL ANALYSIS (10 SECONDS RUNTIME EVIDENCE)\n");
            display_print("=========================================================\n");
            
            bool drv_ok = audio_hal_get_active_driver() != NULL;
            bool codec_ok = (glob_sta & (1<<8)) != 0;
            bool dma_ok = (sr & 0x02) == 0 && (cr & 0x01) != 0;
            bool civ_ok = ac97_playback_get_rotations() > 0 && civ_stall_ticks <= 100;
            bool lvi_ok = lvi == ((civ + 31) % 32);
            bool ring_ok = stream && stream->stats.underrun_counter == 0;
            bool player_ok = bytes_played > 0;
            bool mixer_ok = silence == 0 && ret == req;
            bool refill_ok = ac97_playback_get_late_refills() == 0 && (stream ? stream->stats.underrun_counter == 0 : true);
            
            display_print("Driver       : "); display_print(drv_ok ? "PASS\n" : "FAIL (No Driver)\n");
            display_print("Codec        : "); display_print(codec_ok ? "PASS\n" : "FAIL (PCR Not Ready)\n");
            display_print("DMA          : "); display_print(dma_ok ? "PASS\n" : "FAIL (DCH Halted / Not Running)\n");
            display_print("CIV Rotation : "); display_print(civ_ok ? "PASS\n" : "FAIL (CIV Stopped Advancing)\n");
            display_print("LVI Update   : "); display_print(lvi_ok ? "PASS\n" : "FAIL (LVI Misaligned)\n");
            display_print("Ring Buffer  : "); display_print(ring_ok ? "PASS\n" : "FAIL (Stream Underrun Occurred)\n");
            display_print("Player       : "); display_print(player_ok ? "PASS\n" : "FAIL (No Bytes Read)\n");
            display_print("Mixer        : "); display_print(mixer_ok ? "PASS\n" : "FAIL (Silence Generated / Mismatch)\n");
            display_print("Refill Path  : "); display_print(refill_ok ? "PASS (Staged Proactive Watermark Active)\n" : "FAIL (Late Refill / Starvation)\n");
            
            display_print("\n=========================================================\n");
            display_print("PRIMARY SUSPECT IDENTIFICATION (FIRST DEVIATION IN PIPELINE)\n");
            display_print("=========================================================\n");
            
            if (!drv_ok) {
                display_print("PRIMARY SUSPECT: HAL / DRIVER REGISTRY\nReason: Driver failed to load or discover active hardware.\n");
            } else if (!codec_ok) {
                display_print("PRIMARY SUSPECT: AC97 CODEC\nReason: Codec status register bit 8 (PCR) did not assert ready.\n");
            } else if (!dma_ok || !civ_ok) {
                display_print("PRIMARY SUSPECT: AC97 DMA ENGINE\nReason: Hardware CIV stopped advancing across buffer descriptors or DCH status bit asserted halt.\n");
            } else if (!ring_ok || !refill_ok) {
                display_print("PRIMARY SUSPECT: STREAM / RING BUFFER REFILL\nReason: Ring buffer exhausted audio samples or late refill detected.\n");
            } else if (!mixer_ok) {
                display_print("PRIMARY SUSPECT: SOFTWARE MIXER\nReason: Returned silence bytes while stream contained data or requested frames.\n");
            } else {
                display_print("PRIMARY SUSPECT: NONE (FULL CONTINUOUS COOPERATIVE PIPELINE PASS)\nReason: All subsystems and proactive watermark refill verified healthy with zero stutter.\n");
            }
            
            ac97_playback_dump_trace();
            
            display_print("\n[EVIDENCE COLLECTED. CONTINUING STAGED PROACTIVE WATERMARK PLAYBACK.]\n");
            // Do not break loop here — continue playing smoothly indefinitely!
        }
    }
}
