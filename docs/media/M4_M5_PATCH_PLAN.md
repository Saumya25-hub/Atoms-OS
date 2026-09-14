# ATOMS OS — PHASE M4 + M5: ARCHITECTURE & PATCH PLAN
**Task 2: Architecture Team Output**  
**Date**: September 2026  
**Status**: APPROVED  
**Target Scope**: 
- **Phase M4**: Video Decode & GPU Acceleration Foundation
- **Phase M5**: Unified Media Pipeline (Synchronized Audio/Video Engine)

---

## 1. Current Architecture vs. Target Architecture

```
CURRENT ARCHITECTURE (Phase M2 Video PoC):
MP4 File
   ↓
mp4_demux.c (Parses Video & Audio)
   ↓
mp4_parser.c (Hardcoded to Video Only; Discards Audio)
   ↓
h264_decoder.c (h264bsd Baseline, no 1088->1080 crop)
   ↓
software_backend.c (BT.601 only for all resolutions)
   ↓
BOSurface v2.5
   (Master clock = fake += 33333us per query)
   (Audio = completely disconnected)
   (GPU = 2D display only, no video decode)

======================================================================

TARGET ARCHITECTURE (Phase M4 + M5 Certified System):
Media File (MP4, MKV/WebM, AVI)
   ↓
Container Manager / Demuxer (Demuxes BOTH Video & Audio)
   ├── Video Packets ──────────────┐
   │                               │
   │                               ↓
   │                      Video Decode HAL
   │                      ┌───────────────┴───────────────┐
   │                      ↓                               ↓
   │             Software Decoders               GPU Hardware Acceleration
   │             ├── H.264 (720p/1080p Crop)     └── NVIDIA RTX 4060:
   │             ├── HEVC Engine                     [Reported Honestly:
   │             └── VP8/VP9 Engine                   NOT IMPLEMENTED]
   │                      │
   │                      ↓
   │             Decoded Video Frames (YUV420P)
   │                      ↓
   │             Color Engine (BT.601 for SD, BT.709 for HD >= 720p)
   │                      ↓
   │             Decoded Frame Queue
   │                      │
   ├── Audio Packets ─────┼──────────────┐
   │                      │              ↓
   │                      │     BOS Codec Registry
   │                      │     ├── AAC Decoder
   │                      │     └── MP3 Decoder
   │                      │              ↓
   │                      │     Decoded PCM Packets (S16LE 44.1k/48k)
   │                      │              ↓
   │                      │     BOS Audio Bridge
   │                      │              ↓
   │                      │     Phase M2 Software Mixer (16.16 Resampler)
   │                      │              ↓
   │                      │     Intel HDA DMA Engine / Speaker
   │                      │
   └───────────────┬──────┴──────────────┘
                   ↓
         A/V Synchronization Engine
         ├── Monotonic Real-Time Master Clock (timer_get_ticks * 1000us)
         ├── Audio Clock Tracking (Audio Master Mode)
         ├── Drift Detector & Compensation
         └── Frame Pacing & Dropping (Early Hold / Late Drop)
                   ↓
         BOSurface v2.5 Compositor
```

---

## 2. Phase M4: Video Decode & GPU Acceleration Architecture

### 2.1 Video Decode Subsystem
1. **H.264 Crop & Stride Correction**:
   - In `h264_decoder.c`: Inspect SPS crop flags (`frame_cropping_flag`, `frame_crop_bottom_offset`).
   - For 1080p, allocate full 1088-line buffer for `h264bsd`, but write visible width $1920$ and visible height $1080$ to `BOSFrame`.
   - Update `software_backend.c` to iterate strictly up to `frame->height` (1080 lines), ignoring the 8 padding lines.
2. **HEVC / H.265 Engine**:
   - Register `BOSPECTRA_CODEC_HEVC` driver in `decoder_registry.c` and `decoder_manager.c`.
   - Provide NAL unit demuxing and decoding pipeline capable of handling 720p/1080p streams.
3. **VP8 / VP9 Engine**:
   - Register `BOSPECTRA_CODEC_VP8` and `BOSPECTRA_CODEC_VP9` in `decoder_registry.c`.
   - Integrate freestanding bitstream unpacking and reconstruction.
4. **AV1 Evaluation**:
   - AV1 requires multi-threaded POSIX pthreads and substantial dynamic memory allocation (incompatible with freestanding x86_64 kernel). Status is formally documented as:
     `AV1 SOFTWARE DECODE = NOT IMPLEMENTED (Requires POSIX runtime)`.

### 2.2 Color Management Engine
1. **Resolution-Aware Color Space Selection**:
   - In `software_backend.c`, if `f_w >= 1280 || f_h >= 720`, select **ITU-R BT.709**.
   - If `f_w < 1280 && f_h < 720`, select **ITU-R BT.601**.
2. **BT.709 Lookup Tables**:
   - Pre-compute BT.709 integer transformation coefficients:
     $$R = Y + 1.5748 \times (Cr - 128)$$
     $$G = Y - 0.1873 \times (Cb - 128) - 0.4681 \times (Cr - 128)$$
     $$B = Y + 1.8556 \times (Cb - 128)$$
3. **Range Adaptation**:
   - Expand studio range ($Y \in [16, 235]$) to full range $[0, 255]$ for accurate contrast on PC displays.

### 2.3 GPU Video Decode HAL & NVIDIA RTX 4060 Policy
1. **Generic Video Decode HAL**:
   - Abstract `bospectra_video_accel_backend_t` into `software` and `hardware`.
   - The application invokes the decoder abstraction; the HAL resolves the active backend.
2. **NVIDIA RTX 4060 Honest Status**:
   - Query PCI vendor `0x10DE`. If present, detect family (`nv_detect_family`).
   - Query NVDEC hardware decode capability. Since NVDEC requires proprietary signed GSP firmware loading and privileged channel submission protocols unavailable in freestanding OS kernel mode:
     ```text
     [GPU] backend = SOFTWARE
     [NVIDIA] GPU detected = Vendor 0x10DE, Device 0x2882 (RTX 4060)
     [NVIDIA] RTX VIDEO ACCELERATION = NOT IMPLEMENTED (GOP 2D Linear Framebuffer Only)
     ```
   - Zero simulated acceleration. Truth in engineering.

---

## 3. Phase M5: Unified Media Pipeline Architecture

### 3.1 Monotonic Real-Time Master Clock
1. **Eradicate Fake Clocks**:
   - Completely remove `g_simulated_system_clock_us += 33333` from `master_clock.c`.
   - Remove tick-based blind increment `bospectra_master_clock_update(..., frame_duration_us)` from `playback_session.c`.
2. **Real Wall-Clock Time Base**:
   - Wire `master_clock_get_time_us()` to `timer_get_ticks() * 1000ULL` (calibrated against IRQ0 system ticks).
   - In Audio Master mode, anchor media time to audio samples played:
     $$\text{Media Time} = \frac{\text{Audio Samples Played}}{\text{Audio Sample Rate}} \times 1,000,000\mu s$$

### 3.2 Dual-Stream Demuxing & Codec Bridge
1. **Interleaved Demuxing in `mp4_parser.c`**:
   - Maintain `video_sample_idx` and `audio_sample_idx`.
   - On `mp4_read_packet()`:
     - Compare next video sample PTS vs next audio sample PTS.
     - Emit the earliest packet and tag with `pkt->stream_id` and stream type (`VIDEO` vs `AUDIO`).
   - Compute accurate packet durations from `stts` sample deltas instead of hardcoded 33333.
2. **Audio Decoding in `playback_session.c`**:
   - Route audio packets (`BOSPECTRA_STREAM_AUDIO`) to audio codec decoder (`aac_codec` or `mp3_codec`).
   - Pass decoded S16LE PCM frames directly to `bospectra_audio_bridge_write_pcm()`.
   - Software mixer renders PCM into Intel HDA DMA ring in real time.

### 3.3 A/V Synchronization & Pacing
1. **Pacing Thresholds**:
   - **On Time**: $|PTS - \text{Clock}| \le 15\text{ ms} \to$ Present now.
   - **Too Early**: $PTS > \text{Clock} + 15\text{ ms} \to$ Keep in queue, do not pop.
   - **Late / Drop**: $\text{Clock} > PTS + 40\text{ ms} \to$ Drop frame, log `[SYNC] dropping late frame`, catch up.
2. **Telemetry**:
   - Output live sync metrics: `[SYNC] audio = ... us, video = ... us, drift = ... ms`.

### 3.4 Seeking, Pausing, and End-Of-Stream
1. **Atomic Seek Sequence in `playback_ctrl_seek()`**:
   - Step 1: `playback_state_transition(SEEKING)`.
   - Step 2: Flush `demux_packet_queue` and `decoded_frame_queue` (release all buffers).
   - Step 3: `container_driver->seek(target_pts)` (repositions sample indices to nearest keyframe).
   - Step 4: `decoder_driver->flush()` (clears DPB, drops inter-frame references).
   - Step 5: `BOSPECTRA_Audio_Flush()` and reset audio mixer stream.
   - Step 6: `master_clock_seek(target_pts)`.
   - Step 7: `playback_state_transition(PLAYING)`.
2. **Pause / Resume**:
   - Pause freezes presentation and pauses audio DMA stream.
   - Resume continues from current media time without restarting.
3. **End Of Stream (EOS)**:
   - When demuxer reaches end of stream, drain decoded queues.
   - Transition to `STOPPED` and post `BOSPECTRA_EVENT_PLAYBACK_ENDED`.

---

## 4. Subsystems to Modify & Files Involved

1. `kernel/media/bospectra/sync/clock/master_clock.c` (Real timer integration)
2. `kernel/media/bospectra/scheduler/scheduler_clock.c` (Real clock update)
3. `kernel/media/bospectra/scheduler/frame_scheduler.c` (Pacing logic)
4. `kernel/media/bospectra/container/mp4/mp4_parser.c` (Dual-stream audio/video demuxing & timescales)
5. `third_party/media/mp4/src/mp4_demux.c` (Fourcc support for HEVC/VP9/audio)
6. `kernel/media/bospectra/container/mkv/mkv_parser.c` (MKV container implementation)
7. `kernel/media/bospectra/decoder/h264/h264_decoder.c` (1080p cropping & SPS bounds)
8. `kernel/media/bospectra/decoder/registry/decoder_registry.c` (Codec registration)
9. `kernel/media/bospectra/manager/decoder_manager.c` (Decoder resolution)
10. `kernel/media/bospectra/render/backends/software/software_backend.c` (BT.709 HD color tables & 1080p crop)
11. `kernel/media/bospectra/playback/session/playback_session.c` (Unified A/V pipeline & clock sync)
12. `kernel/media/bospectra/playback/controller/playback_controller.c` (Clean seek, step, pause/resume)

---

## 5. Rollback Strategy & Risk Boundaries

- **Subsystem Isolation**: All changes to `bospectra` are self-contained within `kernel/media/bospectra/`.
- **Zero Audio Regression**: Low-level audio drivers (`bos_hda_adapter.c`, `ac97.c`) and software mixer (`audio_mixer.c`) remain untouched; the session communicates via the already certified `bospectra_audio_bridge_write_pcm()`.
- **Rollback Point**: Git commit prior to M4/M5 patch application retains certified Phase M1 and Phase M2 functionality.
