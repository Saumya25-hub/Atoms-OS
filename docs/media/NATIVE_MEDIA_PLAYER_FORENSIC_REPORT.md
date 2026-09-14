# ATOMS OS — First Native C++ Media Player: Forensic Investigation Report
**Document ID**: `NATIVE_MEDIA_PLAYER_FORENSIC_REPORT.md`  
**Protocol Phase**: TASK 1 — FORENSIC TEAM  
**Date**: September 10, 2026  
**Status**: INVESTIGATION COMPLETE — PENDING ARCHITECTURE APPROVAL  

---

## 1. Executive Summary & Forensic Objective

The objective of this forensic investigation is to establish the exact architecture, bitstream parameters, container layout, and subsystem integration points required to implement the **First Native C++ Media Player** for ATOMS OS.

Per user directive, this application must:
1. Conduct an authentic, end-to-end media playback test with zero mock/synthetic data.
2. Utilize two real test files from `D:\Signatures_OS\TEST1[TEMP]\`:
   - `Dolby_Vision_AtmosHDR.mp4`
   - `NCSJanjiHeroesTonight.mp3`
3. Execute through the **CURRENT BOS GUI architecture**: `BOSurface v2.5` (`bos::Application`, `bos::Window`, `bos::Widget`, `bos::Surface`).
4. Execute video playback through the existing **BOSpectra Media Pipeline** (`mp4_parser.c`, `h264_decoder.c`, `video_accel.c`, `master_clock.c`, `display_scheduler.c`).
5. Execute audio playback through the existing **BOS Audio Pipeline** (`minimp3.h`, `mp3_codec.c`, `audio_mixer.c`, `intel_hda.c`, `SYS_AUDIO_CALL`).
6. Feature a clean, intentionally minimal UI (Window opens $\to$ Video surface takes focus and dominates content area $\to$ Auto-play video $\to$ Play audio concurrently) with NO extraneous controls (no playlists, seek bars, volume sliders, settings, or visualizers).
7. Transparently query the **Universal Video Acceleration HAL** without hardcoding vendor logic (e.g. RTX 4060).
8. Enforce crash isolation: malformed packets, unsupported features, or decode errors must never crash or hang the kernel, window manager, compositor, or desktop shell.

---

## 2. Bitstream & Container Forensic Analysis

Both test assets located at `D:\Signatures_OS\TEST1[TEMP]\` were forensically probed down to the bitstream headers:

### 2.1 Video Asset: `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4`
- **File Size**: 38,125,940 bytes (36.36 MB).
- **Top-Level Container Structure**:
  - `0x00000000`: `ftyp` (32 bytes) — Compatible brands: `isom`, `iso2`, `avc1`, `mp41`.
  - `0x00000020`: `free` (8 bytes) — Padding.
  - `0x00000028`: `mdat` (38,063,375 bytes) — Media Data.
  - `0x0244CD37`: `moov` (62,525 bytes) — Movie Header located at the tail of the file.
- **Track 1 (Video)**:
  - FourCC: `avc1` (H.264 / MPEG-4 AVC).
  - Profile IDC: `100` (`0x64` = High Profile).
  - Level IDC: `40` (`0x28` = Level 4.0).
  - Coded Macroblock Dimensions: 120 macroblocks wide $\times$ 68 macroblocks high ($1920 \times 1088$ pixels).
  - Cropping Parameters (`h264bsdCroppingParams`): `crop_bottom = 4` (8 raster lines cropped) $\implies$ **Visible Resolution: $1920 \times 1080$ (Full HD 1080p)**.
  - Time Base: Timescale = $12288$, Duration = $1147904$ units ($93.42$ seconds).
  - Frame Timing: Sample delta = $512$ units $\implies \frac{12288}{512} = \mathbf{24.00\,\text{FPS}}$ constant frame rate.
  - Total Video Samples: $2242$ frames.
  - Bitstream Format: AVCC length-prefixed NAL units ($4$-byte size headers).
  - Extradata: SPS ($30$ bytes: `67 64 00 28 ac d1 00 78 02 27 e5 c0 5a 80 80 80 a0 ...`), PPS ($4$ bytes: `68 eb e3 cb`).
- **Track 2 (Audio in container)**:
  - FourCC: `mp4a` (AAC), timescale = $44100$, duration = $93.41$ seconds.
- **Dolby Vision / HDR Reality Check**:
  - The filename contains "Dolby_Vision_AtmosHDR.mp4", but bitstream analysis reveals standard 8-bit SDR High Profile AVC (`avc1`) with standard ITU-R BT.709 colorimetry.
  - **Verdict**: Per Rule 8 ("DO NOT FAKE HDR / DOLBY VISION"), ATOMS OS must honestly report the true detected codec: **H.264 High Profile Level 4.0, 1920x1080, 24 FPS, BT.709 SDR**. It will NOT manufacture fake HDR / Dolby Vision metadata.

### 2.2 Audio Asset: `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3`
- **File Size**: 3,329,709 bytes (3.18 MB).
- **Container / Metadata**:
  - ID3v2.4 Header: 35 bytes at offset 0.
- **MPEG Bitstream**:
  - MPEG Version: MPEG-1 (`0x03`).
  - Layer: Layer III (`0x01` = MP3).
  - Sync Word: `0xFFFB`.
  - Bitrate: 128 kbps (`bitrate_index = 9`).
  - Sample Rate: **48,000 Hz** (`sample_rate_index = 1`).
  - Channels: **2 Channels (Stereo)** (`channel_mode = 0`).
  - Target Audio Output: Direct match for native Intel HDA DMA ($48.0\,\text{kHz}$, 16-bit signed stereo).

---

## 3. Subsystem Audit & Integration Points

### 3.1 GUI Framework: `BOSurface v2.5`
- **Location**: `sdk/include/bos/` and `sdk/src/bos/ui/`
- **Key Abstractions**:
  - `bos::Application`: App lifecycle, event dispatch loop, window registry.
  - `bos::Window`: Native window handle, state machine, rounded chrome frame (`RadiusWindow = 10`), client bounds calculation.
  - `bos::Surface`: Framebuffer abstraction (`m_pixels`, `m_width`, `m_height`, `m_stride_bytes`), 2D blit, dirty rect invalidation via `SYS_GUI_INVALIDATE` (syscall 21).
  - `bos::Widget`: Base class with `paint(Surface&, const Rect&)` and event handlers.
- **Ergonomics**: A dedicated `MediaPlayerView` widget occupying 100% of the window's client bounds will handle continuous video presentation and frame blitting.

### 3.2 Video Pipeline: `BOSpectra`
- **Demuxer (`third_party/media/mp4/src/mp4_demux.c`)**:
  - Successfully scans top-level boxes, handles `mdat` preceding `moov`, parses `trak`, `stsd`, `stsz`, `stco`, `stsc`, `stts`.
  - Extracts SPS/PPS extradata and individual AVCC sample offsets.
- **Decoder (`kernel/media/bospectra/decoder/h264/h264_decoder.c`)**:
  - Decodes NAL units via `h264bsd`.
  - Applies cropping to produce visible $1920 \times 1080$ frame.
  - Outputs planar `YUV420P` buffers.
- **Hardware Acceleration HAL (`kernel/media/bospectra/decoder/common/video_accel.c`)**:
  - Needs update from hardcoded RTX 4060 printouts to a universal vendor detection model inspecting PCI Base Class `0x03` (Display Controller) for Intel (`0x8086`), NVIDIA (`0x10DE`), AMD (`0x1002`), or Virtual/QEMU (`0x1B36`/`0x1234`), returning `BOSPECTRA_ACCEL_BACKEND_SOFTWARE` safely when hardware microcode/rings are unavailable.
- **Color Conversion & Presentation (`kernel/media/bospectra/render/` & `color/`)**:
  - Fast ITU-R BT.709 integer lookup tables convert YUV420P to ARGB32 directly into the target surface framebuffer.

### 3.3 Audio Pipeline: `BOS Audio Engine`
- **MP3 Decoder**: `third_party/audio/mp3/include/minimp3.h` and `kernel/audio/codecs/mp3_codec.c`.
- **Mixer / DMA**: `kernel/audio/session/audio_player.c` and `kernel/audio/drivers/hda/intel_hda.c`.
- **Userspace Syscall**: `SYS_AUDIO_CALL` (43U) allows userspace C++ apps to create audio streams, set format ($48\,\text{kHz}$, 16-bit stereo), start playback, and write PCM chunks.

### 3.4 Storage & Image Provisioning: `tools/gpt_image_builder.c`
- Currently packages `BOOTX64.EFI`, `KERNEL.BIN`, `STARTUP.NSH`, and `TEST.MP4` into `atoms_uefi_test.img`.
- Total disk size is 512 MB (1,048,576 sectors).
- Both test files (`Dolby_Vision_AtmosHDR.mp4` = 38.1 MB, `NCSJanjiHeroesTonight.mp3` = 3.3 MB) total 41.4 MB, easily fitting within the 512 MB FAT32 ESP partition.
- `gpt_image_builder.c` will be updated to package:
  - `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4` $\to$ `/DOLBY.MP4` and `/TEST.MP4`
  - `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3` $\to$ `/HEROES.MP3`

---

## 4. Root Cause & Risk Analysis

| Risk / Failure Mode | Probability | Impact | Mitigation Strategy |
|----------------------|-------------|--------|---------------------|
| **`moov` Box at End of File** | Medium | Video fails to open | `mp4_demux.c` already skips `mdat` based on box size and reaches `moov` at `0x0244CD37`. Verified via python probe. |
| **High Profile 1080p Crop Buffer Mismatch** | Medium | Corrupted bottom scanlines | `h264bsdCroppingParams` extracts $1920 \times 1080$ visible area from $1920 \times 1088$ allocated macroblocks. Verified. |
| **FAT32 8.3 Filename Truncation** | High | File not found | `gpt_image_builder.c` creates FAT entries `DOLBY.MP4` and `HEROES.MP3`. `bospectra_file_open` handles uppercase 8.3 candidates automatically. |
| **Audio/Video Clock Desync** | Medium | Stuttering / Drift | Monotonic wall-clock time base (`master_clock_get_time_us`) drives frame scheduling and drops frames if drift $> 15\,\text{ms}$. |
| **Compositor / Desktop Shell Lockup** | Low | System freeze | The media player window executes in non-blocking event loops using `sys_call_yield()` and non-blocking packet queues. If a decode error occurs, session terminates safely. |

---

## 5. Suspected Fix & Architecture Path (NO CODE)

1. **Test Media Provisioning**:
   - Update `tools/gpt_image_builder.c` to embed `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4` and `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3` into the FAT32 ESP partition.
2. **Universal Video Acceleration HAL**:
   - Update `kernel/media/bospectra/decoder/common/video_accel.c` to perform generic PCI device scanning for display controllers across Intel, NVIDIA, and AMD vendors, reporting capabilities honestly without hardcoding RTX 4060.
3. **Native C++ Media Player Application**:
   - Create `userspace/apps/media_player/main.cpp` using `BOSurface v2.5` (`bos::Application`, `bos::Window`, `bos::Widget`, `bos::Surface`).
   - Construct a minimal, clean player UI: Window $\to$ Video Surface occupies content area $\to$ Auto-play video $\to$ Play audio simultaneously.
   - Stream decoded $1920 \times 1080$ frames to the window surface using ITU-R BT.709 color conversion.
   - Stream 48 kHz stereo PCM to Intel HDA via the BOS audio pipeline.
   - Print required forensic telemetry report at startup and runtime.
4. **Desktop Shell Integration**:
   - Add "Media Player" application launch entry to `userspace/apps/desktop_shell/main.c`.
5. **Build Engine Integration**:
   - Add `media_player.elf` build target in `build.ps1`.
