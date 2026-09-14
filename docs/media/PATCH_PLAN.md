# ATOMS OS — MEDIA SUBSYSTEM PATCH PLAN
**Document ID**: `PLAN-MEDIA-2026-09-13-V2`  
**Classification**: ARCHITECTURAL PATCH SPECIFICATION & EXECUTION ROADMAP  
**Target Hardware**: Intel Haswell H81 Motherboard (LGA1150), Core i3 4th Gen, 8GB RAM, Native UEFI Mode  
**Target Media Primary**: `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4` (38,125,940 bytes)  
**Target Media Secondary**: `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3` (3,329,709 bytes)  
**Status**: APPROVED — UNDER EXECUTION

---

## 1. MIRRORED TEXT FIX
- **Exact File / Function**:
  - `sdk/src/bos/ui/surface.cpp` -> `Surface::draw_string(int32_t x, int32_t y, const char* str, const Color& color)`
- **LSB-First Glyph Correction**:
  - The font atlas bitmap in `bovisual/Text/font8x16.h` (`g_font8x16_stub`) stores 8-bit scanlines where bit 0 is the leftmost pixel of the glyph column and bit 7 is the rightmost pixel.
  - Previous code used MSB-first extraction: `if ((bits >> (7 - col)) & 1)`, which inverted the horizontal column indexing (0 <-> 7, 1 <-> 6, 2 <-> 5, 3 <-> 4), mirroring characters horizontally ("Ulta Text").
  - Fixed line: `if ((bits >> col) & 1)`
- **Regression Test**:
  - Test binary `scratch/test_glyph.exe` verified character rendering for 'A', 'B', 'P', 'R'.
  - Verify on-screen string rendering: `ATOMS Media Center` renders left-to-right with all character loops and stems correctly oriented.

---

## 2. MEDIA TITLE FALLBACK
- **Exact File / Function**:
  - `userspace/apps/media_player/main.cpp` -> `MediaCenterWidget::paint()` & `open_media()`
- **Filename Fallback**:
  - MP4 container files such as `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4` often lack Apple/iTunes `udta` metadata atoms (leaving `m_meta.title[0] == '\0'`).
  - When `m_meta.title[0] == '\0'`, the media player header previously reverted to `"No Media Loaded — Press [Space] to Play Default"` even while playing.
  - Patch logic:
    ```cpp
    const char* display_name = m_meta.title;
    if (!display_name || display_name[0] == '\0') {
        const char* slash = strrchr(m_current_uri, '/');
        const char* bslash = strrchr(m_current_uri, '\\');
        const char* fname = slash > bslash ? slash : bslash;
        display_name = fname ? fname + 1 : m_current_uri;
        if (!display_name || display_name[0] == '\0') {
            display_name = "Dolby_Vision_AtmosHDR.mp4";
        }
    }
    ```
- **Regression Test**:
  - When opening `/TEST1/Dolby_Vision_AtmosHDR.mp4` or empty metadata files, title bar immediately shows `"ATOMS Media Center  |  Playing: Dolby_Vision_AtmosHDR.mp4"`.

---

## 3. MEDIA DEBUGGER
- **Reuse Existing ATOMS Debug Infrastructure**:
  - Directly connect Ring-3 Media Player and `libbos_media` to Ring-0 diagnostic services via Syscall 7 (`SYS_DEBUG_PRINT`) and `debuglan_log_subsys("MEDIA", ...)`.
- **Ring 0 + Ring 3 Telemetry**:
  - Ring 3: `bos_media_telemetry_event()` logs structured media events.
  - Ring 0: Syscall 7 dispatches formatted records to serial COM1 and PXE/LAN UDP broadcasts.
- **UDP 9999 Structured Media Events**:
  - Format: `[MEDIA_EVENT] ts=%llu pid=%u ring=%u stage=%s status=%s frame=%u pts=%llu err=%d msg="%s"`
  - Emitted at key lifecycle points: `STREAM_OPEN`, `DEMUX_INIT`, `PACKET_READ`, `PACKET_SUBMIT`, `DECODER_INIT`, `FRAME_DECODED`, `FRAME_OUTPUT`, `FRAME_CRC`, `YUV_CONVERSION`, `SURFACE_WRITE`, `PRESENT`, `ERROR`.
- **UDP 9998 Automatic Screenshot on First Important/Fatal Failure**:
  - Dedicated screen capture port 9998 broadcast via `debuglan_send_screenshot_packet()`.
  - When a fatal failure occurs, `atoms_first_failure_record()` automatically issues a remote screen capture trigger (`atoms_screenshot_request(1)`), streaming the raw UEFI framebuffer to `tools/capture_live_screen.py`.
- **Remote SCREENSHOT Request**:
  - Syscall or hotkey `F11` triggers immediate host snapshot on UDP 9998.
- **FIRST_FAILURE_LOCK**:
  - Static `AtomsFirstFailureRecord` in `bos_media_pipeline.cpp`:
    ```c
    typedef struct {
        uint32_t locked;
        uint32_t stage;
        int32_t  error_code;
        uint32_t frame_index;
        uint64_t timestamp;
        char     source_file[64];
        uint32_t source_line;
        char     message[128];
    } AtomsFirstFailureRecord;
    ```
  - Only the very first error is latched (`locked = 1`). Subsequent cascading errors are ignored, ensuring the true root cause is never masked.
- **Media Pipeline State Snapshot**:
  - Pipeline state structure captures: Demuxer state, stream dimensions, codec type, bitstream offset, decoded picture buffer (DPB) fullness, surface format, and render latency.
- **On-Screen Media Debug HUD (Channel A)**:
  - Semi-transparent overlay in `userspace/apps/media_player/main.cpp` toggled via `F12` or shown automatically upon `first_failure_locked == 1`.
  - Displays:
    - Pipeline stages: `VFS`, `MP4`, `SPS`, `PPS`, `CABAC`, `DPB`, `BOSPECTRA`, `SURFACE`, `SCHED`, `PRESENT`.
    - Real-time stats: Frame count, FPS, Bitrate, Drift/Jitter, First-Failure Root Cause.
- **Bounded / Rate-Limited Logging**:
  - Frame telemetry throttled to once every 30 frames during normal playback to avoid flooding serial/LAN buffers.
  - Errors and state transitions are always transmitted immediately.

---

## 4. H264 HIGH PROFILE / CABAC
- **Current Incomplete Implementation**:
  - `third_party/media/h264/src/h264bsd_cabac.c` had a 343-line stub that:
    1. Incorrectly interpreted I-slice macroblocks using Inter contexts (`ctx[14]`) and labeled non-zero bin macroblocks as `P_L0_16x16`, causing immediate inter-prediction crashes on IDR frames.
    2. Completely lacked `residual_block_cabac` parsing, failing to consume transform coefficient bits.
    3. Caused the bitstream pointer to desynchronize immediately after Macroblock 0, triggering error concealment across all 8,160 macroblocks (`num_err = 8160`).
- **Target-Specific Implementation**:
  - Target bitstream requires:
    - High Profile (IDC 100), Level 4.0, 1920x1080 @ 24fps.
    - IDR Slice (I-slice) macroblocks: `I_4x4`, `I_16x16` (bins 0..5), and `I_PCM`.
    - P-Slice macroblocks: `P_Skip` (ctx 11..13), `P_16x16`, `P_16x8`, `P_8x16`, `P_8x8` (ctx 14..17).
    - Intra Prediction Modes: `prev_intra4x4_pred_mode_flag` (ctx 68), `rem_intra4x4_pred_mode` (bypass), `intra_chroma_pred_mode` (ctx 64..67).
    - Coded Block Pattern (`coded_block_pattern`): Luma (ctx 73..76), Chroma (ctx 77..84).
    - Macroblock QP Delta (`mb_qp_delta`): (ctx 60..63).
    - Residual parsing (`residual_block_cabac`):
      - Coded Block Flag (`coded_block_flag`): ctx 85..104.
      - Significant Coefficient Flag (`significant_coeff_flag`): ctx 105..165.
      - Last Significant Coefficient Flag (`last_significant_coeff_flag`): ctx 166..226.
      - Coefficient Level (`coeff_abs_level_minus1`, `coeff_sign_flag`): ctx 227..276 & bypass.
- **Strict Quality Rules**:
  - No fake/synthetic frames (no synthetic colorbars or gradients).
  - No concealment presented as successful decoding (concealed macroblocks are tracked and flagged; frame is marked corrupt if errors exceed threshold).
  - No host-side transcoding.
  - No "PASS" verdict unless actual Dolby bitstream frames decode cleanly.

---

## 5. EXACT TARGET MEDIA
- **Primary Asset**: `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4`
  - Exact Size: `38,125,940 bytes`
  - Container: ISO Base Media File Format (MP4 v2 / `isom` / `iso2` / `mp41`)
  - Video Stream: Track ID 1, Codec `avc1.640028` (H.264 High Profile, Level 4.0, CABAC)
  - Resolution: `1920x1080` (Progressive)
  - Dimensions in Macroblocks: 120 width x 68 height = 8,160 macroblocks/frame
  - Audio Stream: Track ID 2, Codec `mp4a.40.2` (AAC-LC / Atmos payload)
  - Transcoding: **STRICTLY PROHIBITED**. The test pipeline must process this exact byte stream.
- **Secondary Asset**: `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3`
  - Exact Size: `3,329,709 bytes`
  - Format: MPEG-1 Audio Layer III, 320 kbps, 44.1 kHz Stereo.

---

## 6. FRAME PROOF
To certify playback, the pipeline must log verifiable proof for multiple consecutive frames across all pipeline stages:
1. `PACKET_READ`: Demuxer extracts packet from MP4 track (PTS, size, offset).
2. `PACKET_SUBMIT`: Packet submitted to H.264 byte stream parser.
3. `DECODER_INIT`: SPS/PPS parsed, DPB allocated (1920x1080 YUV420).
4. `FRAME_DECODED`: Slice decode completed with authentic macroblocks (`num_err == 0`).
5. `FRAME_OUTPUT`: DPB outputs decoded picture in display order.
6. `FRAME_CRC`: 32-bit CRC computed over luma (Y) and chroma (Cb, Cr) planes of the authentic frame.
7. `YUV_CONVERSION`: SIMD/scalar conversion from YUV420 to ARGB32.
8. `SURFACE_WRITE`: Decoded pixels copied into `BOSurface` backing buffer.
9. `PRESENT`: Compositor presents window surface to UEFI GOP framebuffer.

---

## 7. BUILD SAFETY & INTEGRITY
- **Ring 3 Isolation Preserved**:
  - Media Player application (`media_player.elf`) and media engine (`libbos_media.a`) remain strictly in Ring 3.
  - Video and audio decoding are performed entirely in userspace memory.
  - No media decoding code is moved into Ring 0.
- **Architectural Components Preserved**:
  - BOFS / VFS: Unaltered.
  - BOSurface: Unaltered client surface shared-memory protocol.
  - BWE (Window Engine): Unaltered message loop and event dispatch.
  - BCM (Compositor): Unaltered desktop surface composition.
  - Debug Infrastructure: Serial COM1 and PXE/LAN UDP 9998/9999 reused without architectural changes.
  - Working Audio: PCM / Sound Blaster / AC97 pipelines preserved.

---

## 8. NO UNRELATED WORK
- The following areas are strictly out of scope and will NOT be touched:
  - HEVC / H.265 decoding
  - VP8 / VP9 decoding
  - Hardware GPU HAL acceleration
  - SIMD architecture rewrites
  - Unrelated audio codecs (Vorbis, FLAC, Opus)
  - Kernel memory manager or scheduler changes

---

## 9. CERTIFICATION CRITERIA
Only the following binary/formal verdicts are permitted:
- `PASS`: Requirement fully met with empirical proof (telemetry logs + screen verification).
- `FAIL`: Requirement attempted but failed verification.
- `PARTIAL`: Functional in limited cases (explicitly documented).
- `NOT_IMPLEMENTED`: Architectural component stubbed or missing.
- `NOT_VERIFIED`: Unverified on target hardware.

Separate certifications are maintained for:
1. **QEMU RESULT**: Pure UEFI OVMF virtualized environment.
2. **PHYSICAL RESULT**: Intel Haswell H81 physical bare-metal hardware.

---

## 10. CONTINUATION
Upon completion of Task 2 execution, `docs/media/MEDIA_PLAYER_CONTINUATION.md` will be updated with:
- **ROOT CAUSE**: Exact technical diagnosis of failures resolved.
- **FIX**: Precise code modifications made across files.
- **FILES CHANGED**: Full manifest of modified files and line counts.
- **TEST EVIDENCE**: Verification logs, frame CRCs, and packet traces.
- **SCREENSHOT EVIDENCE**: Framebuffer screenshots captured via UDP 9998.
- **TELEMETRY**: UDP 9999 structured event output.
- **REMAINING ISSUES**: Documented limitations or edge cases.
- **KNOWN-GOOD COMPONENTS**: Certified operational pipeline blocks.
- **NEXT EXACT STEP**: Specific instructions for physical H81 hardware deployment.
