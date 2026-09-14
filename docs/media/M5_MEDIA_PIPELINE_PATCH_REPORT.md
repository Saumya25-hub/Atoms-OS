# ATOMS OS — Phase M5: Unified Media Pipeline Patch Report
**Protocol Stage**: Task 3 (Patch Team)  
**Date**: September 10, 2026  
**Status**: PATCHES APPLIED & VALIDATED  

---

## 1. Overview & Architectural Scope

Phase M5 establishes the production-grade Unified Audio/Video Media Pipeline for ATOMS OS, unifying the Phase M1/M2 audio subsystem with Phase M2/M4 video decode and rendering:

1. **Monotonic Master Clock**:
   - Replaced artificial time increments (`+= 33333`) with a hardware monotonic wall-clock timer (`timer_get_ticks() * 1000ULL`).
   - Added atomic seek support (`master_clock_seek`).
2. **Dual-Stream Demuxing (MP4 & MKV)**:
   - Updated `third_party/media/mp4/src/mp4_demux.c` to parse both video tracks and audio tracks (`mp4a`, `mp3 `, `.mp3`).
   - Updated `kernel/media/bospectra/container/mp4/mp4_parser.c` to interleave packets based on earliest presentation timestamp (PTS).
   - Freestanding MKV/WebM parser with EBML element reader and cluster parsing.
3. **Unified Audio Bridge**:
   - Routes demuxed audio packets to PCM via `bospectra_audio_bridge_write_pcm()`.
   - Dynamic track parameter configuration (sample rate, channel count) stored in `PlaybackSessionCtx.audio_spec`.
4. **A/V Sync Pacing & Frame Dropping**:
   - Integrated drift calculation: $\Delta = \text{audio\_pts} - \text{video\_pts}$.
   - Pacing logic:
     - Drift between $-40\,\text{ms}$ and $+15\,\text{ms}$: Synchronized on-time playback.
     - Drift $> +15\,\text{ms}$ (video behind audio): Drop non-reference frames to catch up.
     - Drift $< -40\,\text{ms}$ (video ahead of audio): Hold / delay frame.
5. **Atomic Seek Sequence**:
   - Container seek $\to$ Demux queue flush $\to$ Decoded frame queue flush $\to$ Decoder DPB flush $\to$ Audio buffer flush $\to$ Master clock seek.
   - Eradicated 64x64 dummy frame generation in single-step playback; performs genuine bitstream frame decode.

---

## 2. Inventory of Modified and Created Files

### 2.1 Monotonic Master Clock Subsystem
- **[MODIFY]** `kernel/media/bospectra/sync/clock/master_clock.c` & `master_clock.h`:
  - Implemented `master_clock_get_time_us()` using monotonic `timer_get_ticks() * 1000ULL`.
  - Implemented `master_clock_seek(uint64_t seek_time_us)`.

### 2.2 Dual-Stream Demuxer
- **[MODIFY]** `third_party/media/mp4/src/mp4_demux.c` & `include/mp4_demux.h`:
  - Added audio track support with FourCCs: `hvc1`, `hev1`, `vp09`, `vp08`, `mp4a`, `mp3 `, `.mp3`.
  - Extracted audio sample rates, channel counts, and sample counts.
- **[MODIFY]** `kernel/media/bospectra/container/mp4/mp4_parser.c`:
  - Interleaved video and audio packets chronologically by earliest PTS.
- **[NEW]** `kernel/media/bospectra/container/mkv/mkv_parser.c`:
  - Matroska / WebM EBML header and Cluster packet parser.

### 2.3 Unified Playback Session & Audio Bridge
- **[MODIFY]** `kernel/media/bospectra/playback/session/playback_session.h` & `playback_session.c`:
  - Added `audio_spec` to `PlaybackSessionCtx`.
  - Configured audio sessions via `BOSPECTRA_Audio_OpenSession()`.
  - Routed demuxed audio packets to the audio subsystem and updated audio PTS.
  - Implemented real-time A/V sync drift calculation and frame dropping.
  - Anchored playback session tick to monotonic wall-clock time.

### 2.4 Playback Controller & Atomic Seek
- **[MODIFY]** `kernel/media/bospectra/playback/controller/playback_controller.c`:
  - Implemented atomic seek sequence: container seek $\to$ queue flushes $\to$ DPB reset $\to$ audio flush $\to$ master clock sync.
  - Clean single-step frame decoding with genuine bitstream frames.

---

## 3. Verification & Compliance
All code was validated with zero compiler warnings or link errors. Automated host harness and QEMU tests confirmed synchronization, color conversion, dual-track demuxing, and hardware playback.
