# ATOMS OS — MEDIA COMPONENT AUTHENTICITY LEDGER
**Document ID**: `AUTH-MEDIA-2026-09-13-V1`  
**Classification**: AUTHORITATIVE SOURCE-VERIFIED COMPONENT AUDIT  
**Target Hardware**: Intel Haswell H81 Motherboard (LGA1150), Core i3 4th Gen, 8GB RAM, Native UEFI Mode  
**Test Media Primary**: `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4` (38,125,940 bytes)  
**Test Media Secondary**: `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3` (3,329,709 bytes)  
**Status**: INVESTIGATION PHASE COMPLETE (READ-ONLY) — ZERO SOURCE MODIFICATIONS PERFORMED

---

## 1. EXECUTIVE AUTHENTICITY SUMMARY

Every media-related component in the ATOMS OS codebase was audited against ground-truth source code, build scripts, linker maps, and runtime execution. Components are classified into one of seven formal forensic categories:
- **REAL**: Fully functional, authentic implementation that processes actual bitstreams and interfaces with real hardware/subsystems.
- **PARTIAL**: Authentic foundational code present, but missing key features (e.g. baseline profile only, or missing specific profiles/syntax).
- **STUB**: Minimal function shells or dummy structures that return fixed success/error codes without real processing.
- **MOCK**: Simulated or synthetic hardware interfaces that print log messages or query capabilities but perform no real acceleration or ring submission.
- **IMPOSTER**: Code claiming to decode or process data that actually synthesizes fake output (e.g. artificial math patterns/gradients) while discarding input bitstreams.
- **DEAD**: Unused or unreferenced code compiled into the tree or bypassed at runtime.
- **TEST-ONLY**: Harnesses, mock demuxers, or scripts intended solely for unit testing or test automation.

---

## 2. MASTER COMPONENT AUTHENTICITY MATRIX

| Component | Subsystem Location | Verdict | Evidence & Line Proof | Operational Reality |
|:---|:---|:---:|:---|:---|
| **MP4 Demuxer** | `userspace/libbos_media/src/mp4_demuxer.c` | **REAL** | 450 lines; `mp4_demuxer_open()` parses `ftyp`, `moov`, `mvhd`, `trak`, `stbl`, `stsd`, `stsz`, `stco`, `stsc`. | Authentically parses ISO Base Media File Format and indexes all 1,440 samples of `Dolby_Vision_AtmosHDR.mp4`. |
| **H.264 Baseline / CAVLC** | `third_party/media/h264/` (`h264bsd`) | **REAL** | C99 Hantro G1 port. `h264bsd_vlc.c`, `h264bsd_intra_prediction.c`, `h264bsd_inter_prediction.c`. | Authentically decodes Baseline Profile CAVLC streams into planar YUV420P pictures. |
| **H.264 High Profile / CABAC** | `third_party/media/h264/h264bsd_cabac.c` | **PARTIAL / STUB** | 343 lines. Lines 240–310: Dummy `mb_type` and dummy `cbp`; zero transform coefficients, zero motion vectors, zero intra modes. | Fails on real High Profile CABAC streams (e.g. `Dolby_Vision_AtmosHDR.mp4`), triggering concealment (`num_err = 8160`) and solid grey output. |
| **HEVC / H.265 Decoder** | `kernel/media/bospectra/decoder/hevc/hevc_decoder.c` | **IMPOSTER** | 153 lines. Lines 115–118: `row_luma = (((r * 255) / h) ^ (ctx->frame_count * 4)); memset(...)`. | Discards slice payload; synthesizes fake animated XOR luma gradient. |
| **VP8 Decoder** | `kernel/media/bospectra/decoder/vp8/vp8_decoder.c` | **IMPOSTER** | 124 lines. Lines 89–92: `row_luma = (((r * 255) / h) ^ (ctx->frame_count * 3)); memset(...)`. | Discards slice payload; synthesizes fake animated XOR luma gradient. |
| **VP9 Decoder** | `kernel/media/bospectra/decoder/vp9/vp9_decoder.c` | **IMPOSTER** | 132 lines. Lines 95–98: `row_luma = (((r * 255) / h) ^ (ctx->frame_count * 5)); memset(...)`. | Discards slice payload; synthesizes fake animated XOR luma gradient. |
| **Video Accel HAL** | `kernel/media/bospectra/decoder/common/video_accel.c` | **MOCK / STUB** | 123 lines. Lines 22, 65, 116: Unconditionally sets `BOSPECTRA_ACCEL_BACKEND_SOFTWARE`; `Hardware Accel: NO`. | Queries PCI display ID, prints vendor name, but dispatches zero GPU commands. Pure software fallback. |
| **MP3 Audio Decoder** | `third_party/media/minimp3/minimp3.h` | **REAL** | Lieff's minimp3 header library. Full Huffman, dequantization, IMDCT, and subband synthesis. | Authentically decodes MPEG-1 Layer 3 audio frames to 16-bit stereo PCM. |
| **MP4 AAC Audio Decoder** | `userspace/libbos_media/src/bos_media_pipeline.cpp` | **DEAD / STUB** | No AAC decoder is linked or instantiated; MP4 audio tracks are ignored in pipeline setup. | MP4 files play without audio in the native player. |
| **Audio Stream HAL** | `userspace/libbos_media/src/bos_audio_stream.cpp` | **REAL** | 128 lines; invokes `SYS_AUDIO_CALL` (syscall 43) with `AUDIO_SUBMIT_BUFFER`. | Streams PCM across syscall boundary to kernel Intel HDA circular DMA buffer. |
| **Intel HDA Driver** | `kernel/drivers/audio/hda.c` | **REAL** | Programs PCI HDA Base Address Registers (BAR0), CORB/RIRB DMA engines, and Azalia stream descriptors. | Generates real analog audio output on Haswell H81 motherboard hardware. |
| **BOSpectra Color Engine** | `userspace/libbos_media/src/bos_media_simd.cpp` | **REAL** | AVX2 / SSE2 vectorized and C99 scalar YUV420P-to-ARGB color matrix converter. | Performs mathematically authentic BT.601 / BT.709 color conversion. |
| **Frame Scheduler & Clock** | `userspace/libbos_media/src/bos_media_pipeline.cpp` | **REAL** | Monotonic HPET/TSC time delta evaluation against frame PTS; implements early/late/drop pacing. | Accurately paces presentation to target media frame rate (e.g. 24.00 FPS). |
| **BOSurface Window Client** | `sdk/src/bos/ui/surface.cpp`, `window.cpp` | **REAL (BUGGED)** | Interfaces with BOSurface v2.5 shared memory via Syscall 20 (`SYS_GUI_MAP_SURFACE`). Bug: mirrored font shift. | Allocates and renders into Ring-3 shared memory; has horizontal text mirroring bug in `draw_string()`. |
| **BWE / BCM Compositor** | `kernel/wm/bwe/`, `kernel/wm/bcm/` | **REAL** | Window server with double-buffering, clipping rects, z-ordering, and linear framebuffer blits. | Reliably composes desktop windows and surfaces to physical UEFI framebuffer. |
| **VFS / BOFS Media I/O** | `kernel/fs/vfs.c`, `kernel/fs/bofs/` | **REAL** | Mounts AHCI SATA, USB MSC, or RAMFS; handles `open`, `read`, `seek` with cluster caching. | Authentically streams media files from physical disk/flash drives. |
| **Embedded Player ELF** | `kernel/embedded_media_player_elf.S` | **REAL (FALLBACK)** | Embeds `media_player.elf` binary directly in kernel `.rodata` segment. | Allows kernel to launch Media Player in Ring 3 even if VFS root disk is not yet mounted. |

---

## 3. DEEP FORENSIC DISSECTION OF IMPOSTER & MOCK COMPONENTS

### 3.1 HEVC, VP8, and VP9 "Decoders" (`kernel/media/bospectra/decoder/`)
The repository contains files claiming to provide next-generation video decoding:
- `hevc_decoder.c` (153 lines)
- `vp8_decoder.c` (124 lines)
- `vp9_decoder.c` (132 lines)

#### Code Inspection Proof (`hevc_decoder.c`, lines 114–121):
```c
    if (frame->data[0]) {
        for (uint32_t r = 0; r < h; r++) {
            uint8_t row_luma = (uint8_t)(((r * 255) / h) ^ (ctx->frame_count * 4));
            memset(frame->data[0] + (r * w), row_luma, w);
        }
    }
    if (frame->data[1]) memset(frame->data[1], 128, uv_size);
    if (frame->data[2]) memset(frame->data[2], 128, uv_size);
```
#### Forensic Verdict: **100% IMPOSTER**
- These modules do not parse slice data, transform coefficients, motion vectors, prediction units, or entropy structures.
- They generate an animated mathematical gradient XOR'd with `frame_count`.
- **Impact**: Any test claiming "HEVC 4K playback certified" or "VP9 WebM playback certified" was rendering an artificial synthetic pattern.

---

### 3.2 Video Acceleration HAL (`kernel/media/bospectra/decoder/common/video_accel.c`)
Documented as "Intel QuickSync / VA-API and NVIDIA NVDEC Hardware Acceleration Bridge".

#### Code Inspection Proof (`video_accel.c`, lines 21–24, 65–67, 116):
```c
void bospectra_video_accel_init(void) {
    g_active_backend = BOSPECTRA_ACCEL_BACKEND_SOFTWARE;
    g_accel_initialized = true;
    ...
    display_print("[GPU] bridge status: SMART CPU SOFTWARE FALLBACK BRIDGE ENGAGED\n");
    display_print("[GPU] backend selected = SOFTWARE (FFmpeg libavcodec + C99 Color Engine)\n");
}
...
void bospectra_video_accel_dump_diagnostics(void) {
    ...
    display_print(" Hardware Accel   : NO (Pure Software Pipeline)\n");
    display_print(" Decode Mode      : SOFTWARE\n");
}
```
#### Forensic Verdict: **MOCK / STUB**
- The module probes PCI Configuration Space (`dev->base_class == 0x03`), identifies the vendor (Intel `0x8086`, NVIDIA `0x10DE`, AMD `0x1002`), and logs the hardware name.
- It then unconditionally forces `g_active_backend = BOSPECTRA_ACCEL_BACKEND_SOFTWARE`.
- There is zero hardware ring buffer initialization, zero batch buffer submission, and zero GPU MMIO programming.
- All actual decoding in ATOMS OS is performed entirely on the CPU in software.

---

### 3.3 H.264 High Profile / CABAC Entropy Decoder (`h264bsd_cabac.c`)
Documented in Milestone 4 and Milestone 5 reports as "Full H.264 High Profile & CABAC Bitstream Engine".

#### Code Inspection Proof (`third_party/media/h264/h264bsd_cabac.c`):
- Total file length: 343 lines (a complete H.264 CABAC engine typically requires 2,500 to 4,000 lines of complex context models and arithmetic decoding).
- Macroblock decoding stub (lines 270–310):
  ```c
  /* Dummy syntax element parsing */
  mb->mb_type = cabac_read_mb_type(p);
  mb->cbp = dummy_read_cbp(p);
  /* Missing: residual_block_cabac, intra_pred_mode, motion_vector_difference */
  ```
- When tested against `Dolby_Vision_AtmosHDR.mp4`:
  - Bitstream desynchronizes on Macroblock 0 of Slice 0.
  - Returns `H264BSD_ERROR` (`ret = 3`).
  - Concealment routine `h264bsdConceal()` executes, marking 8,160 macroblocks errored and filling the frame buffer with solid grey (`0x808080`).
#### Forensic Verdict: **PARTIAL / INCOMPLETE STUB**
- Baseline Profile CAVLC streams decode authentically and cleanly.
- High Profile CABAC streams fail immediately and produce only solid grey concealed frames.

---

## 4. KNOWN-GOOD (VERIFIED AUTHENTIC) COMPONENTS

The following components have been proven authentic and functional through isolated test harnesses and direct source verification. **THEY MUST NOT BE REWRITTEN OR REPLACED**:

1. **MP4 Container Demuxer** (`userspace/libbos_media/src/mp4_demuxer.c`):
   - Correctly parses ISO box hierarchy, atom lengths, sample-to-chunk indices, and chunk offsets.
   - Accurately resolves 1,440 samples from `Dolby_Vision_AtmosHDR.mp4`.
2. **MP3 Audio Decoder** (`third_party/media/minimp3/minimp3.h`):
   - Flawlessly decodes MPEG-1 Layer 3 bitstreams into 16-bit PCM.
3. **Intel HDA Audio Driver & HAL** (`kernel/drivers/audio/hda.c`, `bos_audio_stream.cpp`):
   - Correctly manages PCI HDA registers and DMA buffer submissions.
4. **BOSpectra Color Space Engine** (`userspace/libbos_media/src/bos_media_simd.cpp`):
   - Authentically converts YUV420P planes to 32-bit ARGB using both AVX2/SSE2 SIMD and integer scalar math.
5. **Frame Pacing & Scheduler** (`userspace/libbos_media/src/bos_media_pipeline.cpp`):
   - Maintains monotonic time synchronization, calculates frame deadlines, and manages presentation queues.
6. **BWE / BCM Window Compositor** (`kernel/wm/bwe/`, `kernel/wm/bcm/`):
   - Handles multi-window composition, clipping, and linear framebuffer updates reliably.
7. **Storage & VFS Subsystem** (`kernel/fs/vfs.c`, `kernel/fs/bofs/`):
   - Reads storage devices and files reliably without corruption.

---

## 5. SUMMARY OF CODE MODIFICATION RESTRICTIONS (RULE 0 COMPLIANCE)

In accordance with ATOMS Engineering Protocol V1:
- ❌ **DO NOT TOUCH**: `kernel/fs/vfs.c`, `kernel/fs/bofs/`, `kernel/wm/bwe/`, `kernel/wm/bcm/`, `kernel/core/exec/process.c`, `kernel/drivers/audio/hda.c`.
- ❌ **DO NOT REWRITE**: The MP4 demuxer, the BOSpectra color engine, or the BOSurface IPC protocol.
- ❌ **DO NOT REPLACE**: The Media Player application with external frameworks.
- 🎯 **TARGETED REPAIRS AUTHORIZED FOR PLAN**:
  1. `sdk/src/bos/ui/surface.cpp`: Fix glyph bit order in `Surface::draw_string()` (`(bits >> col) & 1`).
  2. `userspace/apps/media_player/main.cpp`: Fix title display fallback and integrate with live media debugging.
  3. `third_party/media/h264/`: Address CABAC entropy decoding incompleteness for High Profile streams.
  4. Debugger Integration: Connect live media telemetry to existing `lan_debug` (UDP 9999) and screenshot streaming (UDP 9998).

---
**Component Authenticity Sign-off**:  
*ATOMS Media Forensic Team — Phase 1 Investigation Approved*
