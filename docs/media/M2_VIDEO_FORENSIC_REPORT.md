# ATOMS OS — Phase M2: Real MP4 Video Playback Forensic Report
**Protocol Stage**: Task 1 (Forensic Team)  
**Date**: September 10, 2026  
**Auditor**: ATOMS OS Media & Core Engineering  
**Status**: INVESTIGATION COMPLETE — NO CODE MODIFIED

---

## 1. Executive Summary

A forensic audit of the ATOMS OS video pipeline was conducted to determine why real `.mp4` video files cannot currently be decoded or played on BOSurface v2.5. 

The investigation revealed that while the high-level multimedia architecture (`BOSPECTRA`, `playback_session`, `software_backend`, `BOImage_DrawEx`) is structurally intact and capable of rendering YUV420P frames to screen, the underlying MP4 container demuxer and H.264 video decoder are **placeholders**:
1. `kernel/media/bospectra/container/mp4/mp4_parser.c` generates uninitialized `4096-byte` dummy buffers and never reads real video sample bytes from `mdat`.
2. `kernel/media/bospectra/decoder/h264/h264_decoder.c` generates a synthetic animated mathematical color gradient rather than performing NAL parsing and macroblock decoding.
3. No Sequence Parameter Set (SPS) or Picture Parameter Set (PPS) is extracted from the MP4 `avcC` box.
4. The media player application hardcodes opening `DOLBY.AVI`, and `tools/image_builder.c` does not provision a test `.mp4` file into the FAT32 filesystem image.

To achieve genuine MP4 video playback, the entire chain:
$$\text{BOFS File} \to \text{MP4 Demuxer} \to \text{Sample Tables (stsc/stco/stsz/stts)} \to \text{mdat extraction} \to \text{H.264 Decoder} \to \text{YUV420P Frame} \to \text{BOSurface v2.5}$$
must be replaced with real, deterministic parsing and decoding.

---

## 2. Forensic Evidence & Root Cause Analysis

### 2.1 MP4 Container Demuxer Mock Behavior
- **File**: `kernel/media/bospectra/container/mp4/mp4_parser.c`
- **Lines**: 218–232
```c
size_t sample_size = 4096; // Simulated/demuxed sample payload size
BOSPacket* pkt = bospectra_packet_alloc(sample_size);
if (!pkt) return BOSPECTRA_ERR_OUT_OF_MEMORY;

pkt->stream_id = trk->track_id;
pkt->pts = (trk->current_sample_idx * 33333ULL); // ~30 FPS microsecond calculation
pkt->dts = pkt->pts;
pkt->duration_us = 33333ULL;
pkt->flags = (trk->current_sample_idx % 30 == 0) ? BOSPECTRA_PACKET_FLAG_KEYFRAME : 0;

trk->current_sample_idx++;
ctx->active_track_idx = (ctx->active_track_idx + 1) % ctx->track_count;

*out_pkt = pkt;
return BOSPECTRA_SUCCESS;
```
- **Finding**:
  1. No file I/O is performed in `mp4_read_packet`. `bospectra_file_read` or `bospectra_file_seek` is never invoked.
  2. The sample payload allocated is always 4096 bytes and contains whatever uninitialized memory or zeroes `bospectra_packet_alloc` returns.
  3. No sample index translation takes place.

### 2.2 Missing MP4 Sample Tables
- **File**: `kernel/media/bospectra/container/mp4/mp4_parser.c`
- **Finding**:
  1. `stsz` (Sample Size table): Only reads header and records `sample_count`, ignoring individual sample sizes.
  2. `stco` / `co64` (Chunk Offset table): Completely unparsed.
  3. `stsc` (Sample-to-Chunk table): Completely unparsed.
  4. `stts` (Time-to-Sample table): Completely unparsed.
  5. `stsd` / `avcC`: The decoder configuration record (`avcC`) containing SPS and PPS NAL units is completely skipped.
  Without these tables, it is mathematically impossible to calculate where sample $N$ resides within `mdat`.

### 2.3 H.264 Decoder Fake Gradient Generation
- **File**: `kernel/media/bospectra/decoder/h264/h264_decoder.c`
- **Lines**: 57–78
```c
if (y_plane) {
    if (packet->data && packet->size >= 16) {
        /* Decode stream payload into Y plane */
        const uint8_t* src = (const uint8_t*)packet->data;
        for (uint32_t i = 0; i < y_size; i++) {
            y_plane[i] = (uint8_t)(src[i % packet->size] + (i & 0x7F) + shift);
        }
    } else {
        /* Animated YUV test pattern */
        for (uint32_t i = 0; i < y_size; i++) {
            y_plane[i] = (uint8_t)(((i % w) * 255 / w) + shift);
        }
    }
}
if (u_plane) {
    for (uint32_t i = 0; i < uv_size; i++) u_plane[i] = (uint8_t)(128 + (shift / 2));
}
if (v_plane) {
    for (uint32_t i = 0; i < uv_size; i++) v_plane[i] = (uint8_t)(128 - (shift / 2));
}
```
- **Finding**:
  1. The decoder does not parse NAL units, does not read slice headers, and does not decode macroblocks.
  2. It performs a modulo addition on arbitrary bytes to paint color bars/gradients on screen.
  3. This completely violates the mandate: "No fake/demo/generated frames."

### 2.4 Rendering Subsystem Capability
- **File**: `kernel/media/bospectra/render/backends/software/software_backend.c`
- **Finding**:
  Unlike the decoder, the software renderer is **fully functional**. It implements:
  - Valid YUV420P planar layout verification (`frame->data[0]`, `frame->data[1]`, `frame->data[2]`).
  - Pre-computed BT.601 YUV-to-RGB color space lookup tables with clamping (`s_lut_cr_r`, `s_lut_cb_g`, etc.).
  - Proper row stride handling (`frame->linesize[0..2]`).
  - Presentation via `BOImage_DrawEx` to BWE / BOSurface compositor.
  - Scaling support (`draw_w`, `draw_h`).
  Therefore, the rendering backend requires **zero replacement**, only aspect-ratio and viewport enhancement.

---

## 3. Files Involved

| File | Current Role | Forensic Finding |
|---|---|---|
| `kernel/media/bospectra/container/mp4/mp4_parser.c` | MP4 container driver | Dummy 4096-byte packet generator; lacks `stco`, `stsc`, `stsz`, `avcC` parsing. |
| `kernel/media/bospectra/container/mp4/mp4_parser.h` | MP4 container definitions | Context missing sample tables and `avcC` storage. |
| `kernel/media/bospectra/decoder/h264/h264_decoder.c` | H.264 video decoder | Generates synthetic gradient pattern; no real decoding. |
| `kernel/media/bospectra/decoder/h264/h264_decoder.h` | H.264 decoder header | Missing real decoder context and SPS/PPS state. |
| `kernel/shell/apps/bos_media_player/bos_media_player.c` | Flagship player UI | Hardcoded to open `DOLBY.AVI`. Needs path to `test.mp4`. |
| `tools/image_builder.c` | Disk image packaging | Does not include `test.mp4` in FAT32 filesystem table. |
| `build.ps1` | Kernel build script | Needs build rules for isolated `third_party/media/` modules. |

---

## 4. Risk Analysis

| Risk | Impact | Mitigation |
|---|---|---|
| Freestanding kernel compilation (`-ffreestanding -msoft-float -mno-sse`) | Build breaks if imported code uses libc headers, floating point, or SSE instructions. | Choose integer-only C99 decoders (`h264bsd`) that require zero float/SSE and wrap memory through `bospectra_mem_alloc`. |
| GPL / LGPL contamination | Legal & architectural non-compliance. | Mandatory license audit before code ingestion. Use only Apache-2.0, BSD-3-Clause, MIT, or CC0/Public Domain. |
| Kernel stack exhaustion | System crash if recursive box parsing or deep call chains occur. | Demuxer must use iterative parsing and table indexing; decoder scratch buffers must be heap-allocated via `bospectra_mem_alloc`. |
| Frame allocation bottleneck | Memory starvation or stuttering. | Pre-allocate `BOSFrame` from existing `bospectra_frame_pool`. |

---

## 5. Suspected Fix Strategy (No Code)

1. **Isolation under `third_party/media/`**:
   - Demuxer: Permissive CC0/Public Domain MP4 demuxer (`minimp4`) isolated in `third_party/media/mp4/`.
   - Decoder: Permissive Apache-2.0 H.264 Baseline decoder (`h264bsd`) isolated in `third_party/media/h264/`.
2. **Native Integration**:
   - Adapt `mp4_parser.c` to parse full sample tables, extract SPS/PPS from `avcC`, calculate exact sample byte offsets in `mdat`, and read true compressed NAL packets via `bospectra_file_read`.
   - Adapt `h264_decoder.c` to feed SPS/PPS and NAL slice packets into the freestanding decoder, producing true YUV420P planar frames in `BOSFrame`.
3. **Asset Packaging & Telemetry**:
   - Provide a compliant, progressive H.264 test video (`test.mp4`) baked into the FAT32 image via `tools/image_builder.c`.
   - Implement the exact forensic telemetry chain requested:
     `[MP4] file opened`
     `[MP4] moov parsed`
     `[MP4] video track found`
     `[MP4] codec = avc1`
     `[MP4] resolution = ...`
     `[MP4] samples = ...`
     `[MP4] mdat sample extraction = OK`
     `[H264] SPS = OK`
     `[H264] PPS = OK`
     `[H264] decoder initialized`
     `[VIDEO] frame 0 decoded`
     `...`
     `[BOSURFACE] frame presented`
     `[VIDEO] playback started`
