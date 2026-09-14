# ATOMS OS — PHASE M4 + M5: COMPREHENSIVE FORENSIC AUDIT REPORT
**Task 1: Forensic Investigation Output**  
**Date**: September 2026  
**Status**: COMPLETE / AUDIT APPROVED  
**Target Scope**: 
- **Phase M4**: Video Decode & GPU Acceleration Foundation
- **Phase M5**: Unified Media Pipeline (Synchronized Audio/Video Engine)

---

## 1. Executive Forensic Summary

In accordance with **RULE 0 (Mandatory Phase Isolation)** and the **ATOMS OS Master Engineering Protocol**, this document provides an exhaustive, forensic-level architectural audit of the ATOMS OS video, graphics, demuxing, decoding, audio bridge, and media synchronization subsystems prior to generating any patches or modifying source code.

### Core Verdict
While ATOMS OS possesses functioning foundations for **Phase M1 (Intel HDA & Realtek Codec)**, **Phase M2 (Universal Audio Engine & Audio Codecs)**, and an initial **Phase M2 Video proof-of-concept (MP4 Baseline H.264 software decode to BOSurface)**, the current media pipeline contains critical architectural gaps, hardcoded assumptions, simulated clocks, stubbed container parsers, and a complete absence of hardware video decode acceleration.

---

## 2. Exhaustive Subsystem Forensic Audit

### 2.1 Video Decoders Subsystem (`kernel/media/bospectra/decoder/`)

#### A. H.264 / AVC (`kernel/media/bospectra/decoder/h264/h264_decoder.c`)
- **Current Status**: Implemented using freestanding C99 `h264bsd` (AOSP Stagefright, Apache-2.0).
- **Forensic Findings**:
  1. **Profile Limitation**: Only supports H.264 **Baseline Profile** (I-slices and P-slices). B-slices (`H264BSD_SLICE_B`) and High Profile 8x8 integer DCT / CABAC entropy coding return decode errors. Any High Profile MP4 stream fails.
  2. **Dimension Cropping & Macroblock Padding**: `h264bsd` rounds picture dimensions up to multiples of 16 macroblocks (`h264bsdPicWidth * 16`, `h264bsdPicHeight * 16`). For a 1920x1080 (1080p) stream, 1080 is not divisible by 16 ($1080 = 16 \times 67.5 \to 68 \times 16 = 1088$). The decoder outputs 1088 lines, but lines 1080..1087 represent padding macroblocks. The current copy routine (`memcpy(frame->data[0], yuv_data, w * h)`) does not crop out the padding, leading to subtle stride/vertical pitch distortion on 1080p video.
  3. **Extradata Dependency**: In `h264_open()`, SPS/PPS extradata is parsed only if `stream_desc->extradata` is provided by the demuxer. If an elementary Annex-B stream or an in-band parameter set is encountered, initialization may stall until headers are detected in `h264bsdDecode()`.
  4. **Reference Picture Management**: Slices rely on DPB (Decoded Picture Buffer). When seeking, `h264_flush()` calls `h264bsdFlushBuffer()`, but `playback_ctrl_seek()` never invokes `flush()`.

#### B. HEVC / H.265 (`kernel/media/bospectra/decoder/hevc/`)
- **Current Status**: **NOT IMPLEMENTED**.
- **Forensic Findings**:
  1. The directory `kernel/media/bospectra/decoder/hevc/` does not exist.
  2. `bospectra_codec_id_t` contains `BOSPECTRA_CODEC_HEVC`, but `bospectra_decoder_manager_init()` does not register any HEVC driver.
  3. Attempting to open an HEVC stream (`hvc1` or `hev1`) yields `BOSPECTRA_ERR_CODEC_NOT_FOUND`.

#### C. VP8 / VP9 (`kernel/media/bospectra/decoder/vp8/`, `vp9/`)
- **Current Status**: **NOT IMPLEMENTED**.
- **Forensic Findings**:
  1. No VP8 or VP9 decoder implementation exists in the codebase.
  2. `mkv_parser.c` hardcodes `strncpy(out_desc->codec_name, "VP9", ...)` in `mkv_get_stream()`, but `bospectra_decoder_resolve()` fails because no VP9 decoder driver is registered.

#### D. AV1 (`kernel/media/bospectra/decoder/av1/`)
- **Current Status**: **NOT IMPLEMENTED**.
- **Forensic Findings**:
  1. `BOSPECTRA_CODEC_AV1` exists in the enum, but no AV1 decoder driver exists.
  2. Candidate `dav1d` requires POSIX pthreads, dynamic thread pools, and complex assembly vector extensions not supported in ATOMS OS freestanding kernel mode.

---

### 2.2 Container & Demuxing Subsystem (`kernel/media/bospectra/container/`)

#### A. MP4 / MOV Container (`third_party/media/mp4/` & `kernel/media/bospectra/container/mp4/`)
- **Current Status**: Partially Implemented (Video only).
- **Forensic Findings**:
  1. **Audio Track Ignored**: While `mp4_demux.c` correctly parses both video and audio sample tables (`stsz`, `stco`, `stsc`, `stts`), `mp4_parser.c` (line 89) sets:
     ```c
     ctx->active_track_idx = (uint32_t)ctx->demux->video_track_idx;
     ```
     and explicitly outputs:
     ```c
     [AUDIO] VIDEO PASS / AUDIO NOT YET SUPPORTED
     ```
     During packet extraction in `mp4_read_packet()`, only video samples are read. Audio packets are completely discarded.
  2. **Hardcoded Frame Duration & FPS**: In `mp4_read_packet()`:
     ```c
     pkt->duration_us = 33333ULL; // Hardcoded 30 FPS regardless of true container timescale
     ```
     This corrupts timestamp pacing for 24 FPS, 25 FPS, 50 FPS, and 60 FPS videos.
  3. **Codec FourCC Blindness**: `parse_stsd()` in `mp4_demux.c` only matches `'avc1'` and `'mp4a'`. It silently skips `'hvc1'`, `'hev1'`, `'vp09'`, `'av01'`, and `'mp3 '`.

#### B. MKV / WebM Container (`kernel/media/bospectra/container/mkv/mkv_parser.c`)
- **Current Status**: **HOLLOW STUB**.
- **Forensic Findings**:
  1. `mkv_probe()` checks for `EBML_ID_HEADER` (`0x1A45DFA3`).
  2. `mkv_open()` sets `*driver_ctx = (void*)(uintptr_t)1;` without allocating any context or parsing tracks.
  3. `mkv_read_packet()` immediately returns `BOSPECTRA_ERR_BUFFER_UNDERFLOW`.
  4. No cluster parsing, block parsing, simple block parsing, or timestamp scale extraction exists.

#### C. AVI Container (`kernel/media/bospectra/container/avi/avi_parser.c`)
- **Current Status**: Operational for MJPEG/PCM streams, but lacks H.264/AAC AVI fourcc mapping.

---

### 2.3 GPU Hardware Acceleration & NVIDIA RTX 4060 Audit

#### A. Existing NVIDIA Codebase (`kernel/graphics/gpu/drivers/gpu_drv_nvidia.c`)
- **Current Status**: 2D Display & Framebuffer Driver Only.
- **Forensic Findings**:
  1. **Driver Scope**: The driver programs legacy VGA/display controller registers:
     - `NV_PMC_ENABLE` (Power controller)
     - `NV_PCRTC_H_TOTAL`, `NV_PCRTC_V_TOTAL`, `NV_PCRTC_START` (Display CRTC timings)
     - `NV_PRAMDAC_GENERAL_CONTROL` (DAC color mode)
     - `NV_PRAMDAC_CURSOR_CTRL`, `NV_PRAMDAC_CURSOR_POS` (Hardware cursor)
  2. **No Hardware Video Decode (NVDEC)**:
     - The driver contains **zero** NVDEC initialization, zero Falcon/NVDEC engine firmware loading, zero pushbuffer/channel submission rings, and zero hardware surface allocation for NV12/P010 decoded buffers.
     - Modern NVIDIA architectures (Ada Lovelace RTX 4060, Ampere RTX 30-series, Turing) require cryptographically signed GSP (GPU System Processor) firmware loading and privileged channel submission protocols that cannot be reverse-engineered or implemented in freestanding C99 within the kernel scope.
  3. **Mandatory Reporting Enforcement**:
     - Per Section 13 & Section 14 of the Prompt:
       ```text
       If any required part is missing:
       RTX VIDEO ACCELERATION = NOT IMPLEMENTED
       Do not fake it.
       ```
     - Any claim that hardware GPU video decoding is active on RTX 4060 in ATOMS OS would be a manufactured falsification. The video decode pipeline must run through the certified **BOS Software Video Decoder Backend** over the UEFI GOP linear framebuffer.

---

### 2.4 Color Management Subsystem (`kernel/media/bospectra/color/` & `software_backend.c`)

- **Current Status**: BT.601 YUV420P $\to$ ARGB32 Inline Table.
- **Forensic Findings**:
  1. **Blind BT.601 Assumption**: In `software_backend.c` (lines 91–113), pre-computed LUTs (`s_lut_cr_r`, `s_lut_cb_g`, `s_lut_cr_g`, `s_lut_cb_b`) are strictly parameterized for ITU-R BT.601 (SDTV / JPEG).
  2. **HD Color Distortion**: HD (720p) and Full HD (1080p) video content is authored in **ITU-R BT.709**. When BT.709 video is decoded and rendered through BT.601 coefficients, red and green saturation shifts occur (skin tones appear artificially reddish/magenta, greens lose natural luminance).
  3. **Range Mismatch**: Software backend assumes Full Range ($Y \in [0, 255]$). Most standard MP4 H.264/HEVC broadcasts use Limited/Studio Range ($Y \in [16, 235]$, $Cb/Cr \in [16, 240]$). Without range expansion, black levels look elevated/gray and peak whites clip.

---

### 2.5 Master Media Clock & Time Pipeline (`kernel/media/bospectra/sync/clock/master_clock.c`)

- **Current Status**: **SIMULATED CLOCK (PROHIBITED PATTERN)**.
- **Forensic Findings**:
  1. In `master_clock.c` (line 42):
     ```c
     g_simulated_system_clock_us += 33333; // Simulate +33.3ms progression per query
     ```
     This is explicitly prohibited by Rule 19 ("Do NOT implement: 'add 33333 microseconds every query' or equivalent fake timing. The media clock must represent elapsed real time.").
  2. In `playback_session.c` (line 263–265):
     ```c
     uint64_t frame_duration_us = sess->frame_scheduler_ctx.pacer.frame_interval_us;
     if (frame_duration_us == 0) frame_duration_us = 41666U;
     bospectra_master_clock_update(&sess->frame_scheduler_ctx.master_clock, frame_duration_us);
     ```
     Every invocation of `playback_session_tick()` blindly advances the clock by one frame duration regardless of elapsed physical wall-clock time. If the CPU stalls, the clock stalls; if the loop spins fast, the clock runs forward at warp speed.
  3. **Real Time Source Availability**: ATOMS OS has `timer_get_ticks()` in `kernel/core/timer/src/timer.c` providing an interrupt-driven monotonic 1000 Hz millisecond clock (1 tick = 1000 $\mu$s), as well as x86_64 RDTSC. The media master clock must be anchored to this real time source.

---

### 2.6 A/V Synchronization & Audio/Video Session Multiplexing

- **Current Status**: Decoupled; Audio playback is not driven by the video session.
- **Forensic Findings**:
  1. **Single-Stream Demuxing in Playback Session**: `playback_session.c` resolves only `BOSPECTRA_STREAM_VIDEO`. The demux queue only accepts video packets.
  2. **Audio Decoder Missing from Session**: `playback_session.c` does not instantiate an audio decoder (such as AAC or MP3) to decode compressed packets extracted from MP4/MKV.
  3. **Audio Bridge Disconnected**: While `bospectra_audio_bridge_write_pcm()` exists and connects to the Phase M2 Software Mixer, `playback_session` never feeds decoded audio packets to it.
  4. **A/V Drift Unmanaged**: `drift_detector.c` computes `video_pts_us - audio_pts_us`, but no feedback loop exists in `playback_session_tick()` to drop late video frames or throttle early video frames against the audio clock.

---

### 2.7 Seeking, Buffering, and State Lifecycle

- **Current Status**: Partial / Broken Seek.
- **Forensic Findings**:
  1. In `playback_controller.c` (`playback_ctrl_seek()`):
     - Transitions state to `BOSPECTRA_PLAYBACK_STATE_SEEKING`.
     - Updates timeline.
     - **NEVER** calls `sess->container_driver->seek(sess->container_ctx, target_pts)`.
     - **NEVER** calls `sess->decoder_driver->flush(sess->decoder_ctx)`.
     - **NEVER** flushes `demux_packet_queue` or `decoded_frame_queue`.
     - Stale packets from before the seek are decoded and presented, causing visible frame jumping.
  2. In `playback_ctrl_step_frame()` (line 104):
     - Acquires a dummy 64x64 frame and presents it. This is a synthetic mock frame.
  3. End-Of-Stream (EOS): When demuxer returns `BOSPECTRA_ERR_BUFFER_UNDERFLOW`, `playback_session.c` (line 175) seeks back to 0, causing infinite looping instead of draining remaining queued frames and signaling `BOSPECTRA_EVENT_PLAYBACK_ENDED`.

---

### 2.8 Frame Pool & Resource Leaks (`kernel/media/bospectra/frame_memory/`)

- **Current Status**: 8-frame pool (`BOSPECTRA_FRAME_POOL_SIZE = 8U`).
- **Forensic Findings**:
  1. `bospectra_frame_acquire()` allocates `f->data[0]` on-demand using `bospectra_mem_alloc_aligned()`. If `needed_size` changes (e.g., from 720p to 1080p), the buffer is freed and reallocated.
  2. For 1080p YUV420P:
     $$\text{Size} = 1920 \times 1088 + 2 \times (960 \times 544) = 2,088,960 + 1,044,480 = 3,133,440\text{ bytes } (\approx 3.0\text{ MB per frame})$$
     8 frames $\times 3.0\text{ MB} = 24.0\text{ MB}$ VMM heap usage.
  3. `bospectra_frame_pool_get_counts()` tracks active, peak, and capacity. Reference counts must be strictly tracked between decoder DPB and presentation renderer to prevent pool exhaustion.

---

## 3. Explicit Inventory of Prohibited Patterns, Stubs, and Fakes

| Item ID | Source File | Line | Prohibited Pattern / Flaw | Impact |
|---|---|---|---|---|
| **F-01** | `master_clock.c` | 42 | `g_simulated_system_clock_us += 33333;` | Fake clock advancing 33.3ms per query regardless of time |
| **F-02** | `playback_session.c` | 265 | `bospectra_master_clock_update(..., frame_duration_us);` | Clock updates per tick cycle, detached from real monotonic timer |
| **F-03** | `mkv_parser.c` | 31 | `*driver_ctx = (void*)(uintptr_t)1; // Stub context` | Hollow MKV parser stub; cannot decode any Matroska/WebM video |
| **F-04** | `mkv_parser.c` | 54 | `return BOSPECTRA_ERR_BUFFER_UNDERFLOW;` | MKV packet reading unconditionally fails |
| **F-05** | `mpeg2_decoder.c` | 9 | `*driver_ctx = (void*)(uintptr_t)1; // Stub context` | Stub decoder |
| **F-06** | `playback_controller.c`| 104-107 | `BOSFrame* dummy_frame = ... acquire(64, 64, ...)` | Fake 64x64 synthetic frame used for step playback |
| **F-07** | `mp4_parser.c` | 89, 94 | `[AUDIO] VIDEO PASS / AUDIO NOT YET SUPPORTED` | Audio track in MP4 completely disabled / unread |
| **F-08** | `mp4_parser.c` | 186 | `pkt->duration_us = 33333ULL;` | Hardcoded 30 FPS duration in MP4 packets |
| **F-09** | `playback_controller.c`| 62-80 | Missing demux seek and decoder flush | Seek does not reset bitstream or DPB state |
| **F-10** | `software_backend.c` | 91-113 | Only BT.601 color tables implemented | HD 720p/1080p BT.709 color distortion |
| **F-11** | `gpu_drv_nvidia.c` | 1-329 | No NVDEC hardware video decode engine | GPU acceleration is strictly 2D display; video decode HW absent |
| **F-12** | `playback_session.c` | 175 | `sess->container_driver->seek(..., 0);` on underflow | Re-loops on EOF instead of proper draining and EOS event |

---

## 4. Subsystem Files Involved

1. **Demuxers**:
   - `third_party/media/mp4/src/mp4_demux.c`
   - `third_party/media/mp4/include/mp4_demux.h`
   - `kernel/media/bospectra/container/mp4/mp4_parser.c`
   - `kernel/media/bospectra/container/mkv/mkv_parser.c`
   - `kernel/media/bospectra/container/avi/avi_parser.c`
2. **Video Decoders**:
   - `kernel/media/bospectra/decoder/h264/h264_decoder.c`
   - `third_party/media/h264/include/h264bsd_decoder.h`
   - `third_party/media/h264/` (core decoding files)
   - `kernel/media/bospectra/decoder/registry/decoder_registry.c`
   - `kernel/media/bospectra/manager/decoder_manager.c`
3. **Audio Decoder & Bridge Integration**:
   - `kernel/media/bospectra/audio/bridge/audio_bridge.c`
   - `kernel/audio/codecs/codec_registry.h` / `.c`
   - `kernel/audio/codecs/aac_codec.c`
   - `kernel/audio/codecs/mp3_codec.c`
4. **Color & Rendering**:
   - `kernel/media/bospectra/color/include/bospectra_color_spaces.h`
   - `kernel/media/bospectra/color/converters/yuv420/yuv420_converter.c`
   - `kernel/media/bospectra/render/backends/software/software_backend.c`
5. **Synchronization & Clocks**:
   - `kernel/media/bospectra/sync/clock/master_clock.c`
   - `kernel/media/bospectra/scheduler/scheduler_clock.c`
   - `kernel/media/bospectra/scheduler/frame_scheduler.c`
   - `kernel/media/bospectra/sync/drift/drift_detector.c`
6. **Playback Session & Controller**:
   - `kernel/media/bospectra/playback/session/playback_session.c`
   - `kernel/media/bospectra/playback/controller/playback_controller.c`
   - `kernel/media/bospectra/pipeline/packet_queue.c`
   - `kernel/media/bospectra/pipeline/frame_queue.c`
7. **GPU / HAL Layer**:
   - `kernel/graphics/gpu/drivers/gpu_drv_nvidia.c`
   - `kernel/graphics/gpu/include/gpu_device.h`

---

## 5. Risk Analysis & Architecture Strategy (NO CODE)

1. **Risk of Breaking Existing Certified Foundations**:
   - Phase M1 (Intel HDA) and Phase M2 (Software Mixer, WAV, MP3, FLAC) are certified passing. All modifications must be additive and isolated. Video and audio must merge in `playback_session` through cleanly abstracted interfaces without polluting the low-level audio driver.
2. **GPU Acceleration Honesty**:
   - The system must explicitly report:
     ```text
     [GPU] backend = SOFTWARE
     [NVIDIA] RTX VIDEO ACCELERATION = NOT IMPLEMENTED (2D GOP Display Only)
     ```
     Never manufacture a fake PASS.
3. **A/V Sync Precision**:
   - Anchoring `master_clock` to `timer_get_ticks() * 1000ULL` provides 1 ms resolution, which is more than sufficient for 30 FPS (33.3 ms per frame) and 60 FPS (16.6 ms per frame) video pacing.
4. **1080p Memory & Performance Constraints**:
   - 1080p decoding produces ~3.1 MB per frame. The frame pool must be bounded (e.g. 6–8 frames) and macroblock padding (1088 vs 1080 lines) must be cleanly cropped during YUV $\to$ ARGB presentation.

---

## 6. Forensic Conclusion & Approval

Task 1 Forensic Audit is **COMPLETE**. No source code has been altered.
The investigation definitively maps all root causes, fake patterns, missing decoders, and synchronization flaws. We now proceed to **Task 2: Architecture Plan (`docs/media/M4_M5_PATCH_PLAN.md`)** and **Task 3: Third-Party License Audit (`docs/media/M4_M5_THIRD_PARTY_LICENSE_AUDIT.md`)**.
