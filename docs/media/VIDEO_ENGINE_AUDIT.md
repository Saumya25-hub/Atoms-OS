# ATOMS OS — VIDEO ENGINE & DECODER FORENSIC AUDIT
**Document ID**: `docs/media/VIDEO_ENGINE_AUDIT.md`  
**Subsystem**: BOSPECTRA Video Subsystem, Demuxers, Decoders, Color Pipeline, Render Backends  
**Date**: September 10, 2026  
**Status**: AUDIT COMPLETE  

---

## 1. Executive Summary

A comprehensive code audit of the video subsystem located in `kernel/media/bospectra/` and `kernel/shell/apps/bos_media_player/` reveals that while a functional native **AVI / MJPEG** pipeline was implemented and validated, **modern video formats (MP4, MKV, H.264, VP9) are either stubs or synthetic mocks**.

### Primary Findings:
1. **Container Demuxers**:
   - **AVI**: Fully functional. Accurately traverses RIFF chunks, parses stream headers, locates the `movi` list, identifies video chunks (`00dc`, `01dc`), and reads raw JPEG payloads from storage.
   - **MP4**: Partially implemented. Parses top-level ISO boxes (`ftyp`, `moov`, `mvhd`, `trak`, `tkhd`, `hdlr`, `stsz`), but `mp4_read_packet()` synthesizes dummy 4096-byte buffers without reading sample chunks from `mdat`.
   - **MKV**: Complete stub. Probes EBML magic number, but `mkv_read_packet()` returns `BOSPECTRA_ERR_BUFFER_UNDERFLOW` immediately.
2. **Video Decoders**:
   - **MJPEG**: Fully functional baseline JPEG decoder (737 lines) with custom Huffman table decoding, inverse discrete cosine transform (IDCT), and MCU assembly to planar YUV420P.
   - **H.264**: **100% Mock/Placeholder**. Fills frames with arbitrary byte loops (`src[i % size] + shift`) or an animated gradient. Contains zero H.264 NAL parsing, slice headers, CAVLC/CABAC, or macroblock reconstruction.
   - **MPEG-2**: **100% Stub**. Allocates a blank 1920x1080 buffer and returns without inspecting the bitstream.
   - **VP8, VP9, AV1, H.265**: Not implemented.
3. **Color Conversion & Presentation**:
   - Software color pipeline (`yuv420_converter.c` and `software_backend.c`) features a high-performance integer lookup-table BT.601 YUV420P→ARGB32 converter.
   - Output presentation (`sw_present`) blits directly to the kernel BWE window manager via `BOImage_DrawEx()`. There is **no existing bridge** to deliver decoded frames into userspace C++ `bos::Surface` / `bos::Image` widgets.

---

## 2. Container & Codec Forensic Support Matrix

### 2.1 Container / Demuxer Support Matrix
| Container | Extension | Probe Implemented? | Header Parsing? | Packet Demuxing? | Seeking? | Forensic Status |
|---|---|---|---|---|---|---|
| **RIFF / AVI** | `.avi` | **YES** (100%) | **YES** (`avih`, `strh`, `strf`) | **YES** (Reads `00dc`/`01dc` from disk) | **YES** (Frame-index seek) | **REAL / FUNCTIONAL** |
| **MP4 / ISO BMFF** | `.mp4`, `.mov` | **YES** (100%) | **YES** (`ftyp`, `moov`, `trak`, `stsz`) | **NO** (Synthesizes fake 4KB packets) | **STUB** (Sample index only) | **PARTIAL / MOCK DEMUX** |
| **Matroska / WebM**| `.mkv`, `.webm`| **YES** (EBML ID) | **STUB** (Hardcoded VP9 metadata) | **NO** (Returns underflow) | **NO** (No-op) | **STUB** |

### 2.2 Video Codec Decoder Support Matrix
| Codec | Implementation File | Decoder Exists? | Bitstream Parsing? | Pixel Format Output | Forensic Reality |
|---|---|---|---|---|---|
| **MJPEG (JPEG)** | `mjpeg_decoder.c` | **YES** | **YES** (DHT, DQT, SOF0, SOS, IDCT) | `YUV420P` | **REAL DECODER** (Functional baseline JPEG) |
| **H.264 (AVC)** | `h264_decoder.c` | **NO** | **NO** (No NAL, SPS/PPS, or slices) | `YUV420P` | **MOCK** (Generates gradient or modulo patterns) |
| **MPEG-2** | `mpeg2_decoder.c` | **NO** | **NO** (Zero bitstream inspection) | `YUV420P` | **STUB** (Returns blank 1080p frame) |
| **VP8 / VP9** | None | **NO** | **NO** | None | **MISSING** |
| **AV1** | None | **NO** | **NO** | None | **MISSING** |
| **H.265 (HEVC)** | None | **NO** | **NO** | None | **MISSING** |

---

## 3. In-Depth Code Inspections

### 3.1 AVI Demuxer (`kernel/media/bospectra/container/avi/avi_parser.c`)
- **Header Parsing**: Correctly handles little-endian RIFF chunk sizes, locates `LIST movi`, and reads frame rate / dimensions from `AVIH` chunks.
- **Packet Demuxing** ([avi_parser.c:258-298](file:///d:/Signatures_OS/kernel/media/bospectra/container/avi/avi_parser.c#L258-L298)):
  Scans through `movi` chunks for video fourcc identifiers (`00dc`, `01dc`), verifies the JPEG Start-Of-Image marker (`0xFF 0xD8`), allocates a `BOSPacket`, seeks disk position, and reads the raw JPEG payload into memory.
- **Seeking**: Supports index-based frame calculation (`timestamp_us / us_per_frame`) and resets the `movi_offset` pointer.

### 3.2 MP4 Demuxer (`kernel/media/bospectra/container/mp4/mp4_parser.c`)
- **Header Traversal**: Traverses nested boxes (`moov` -> `trak` -> `mdia` -> `minf` -> `stbl` -> `stsz`). Extracts track width, height, and sample counts.
- **Packet Extraction Mock** ([mp4_parser.c:218-228](file:///d:/Signatures_OS/kernel/media/bospectra/container/mp4/mp4_parser.c#L218-L228)):
  ```c
  size_t sample_size = 4096; // Simulated/demuxed sample payload size
  BOSPacket* pkt = bospectra_packet_alloc(sample_size);
  if (!pkt) return BOSPECTRA_ERR_OUT_OF_MEMORY;

  pkt->stream_id = trk->track_id;
  pkt->pts = (trk->current_sample_idx * 33333ULL); // ~30 FPS
  ```
  The demuxer fails to read chunk offsets (`stco`/`co64`), sample sizes (`stsz`), or sample-to-chunk tables (`stsc`), resulting in empty simulated packets.

### 3.3 MJPEG Decoder (`kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c`)
- Standalone 737-line baseline JPEG decoder.
- Implements marker parsing: `JPEG_SOI` (0xFFD8), `JPEG_SOF0` (0xFFC0), `JPEG_DHT` (0xFFC4), `JPEG_DQT` (0xFFDB), `JPEG_SOS` (0xFFDA).
- Contains complete 64-element zigzag ordering table, 8-bit IDCT matrix transformation (`idct.c`), and MCU expansion for 4:2:0 and 4:2:2 chroma subsampling.
- Correctly produces planar `YUV420P` frames.

### 3.4 H.264 Decoder (`kernel/media/bospectra/decoder/h264/h264_decoder.c`)
Inspection of lines 57–77 exposes the synthetic placeholder:
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
```
There is zero real H.264 bitstream parsing.

---

## 4. Video Rendering & BOSurface v2.5 Integration Gap

### 4.1 Kernel Software Render Backend
In [`kernel/media/bospectra/render/backends/software/software_backend.c:78-148`](file:///d:/Signatures_OS/kernel/media/bospectra/render/backends/software/software_backend.c#L78-L148):
- Implements high-speed integer YUV420P→ARGB32 conversion using precomputed lookup tables for BT.601 color coefficients:
  - $R = Y + 1.402(Cr - 128)$
  - $G = Y - 0.344136(Cb - 128) - 0.714136(Cr - 128)$
  - $B = Y + 1.772(Cb - 128)$
- Computes CRC32 checksums for frame verification.

### 4.2 Presentation Gap
In `sw_present()` ([software_backend.c:215](file:///d:/Signatures_OS/kernel/media/bospectra/render/backends/software/software_backend.c#L215)):
```c
BOImage_DrawEx(&ctx->surface_image, x, y, draw_w, draw_h, BO_FILTER_NEAREST);
```
- This function draws directly to the kernel BWE compositor backbuffer.
- It is incompatible with modern userspace C++ applications using `bos::Window` and `bos::Surface`, where rendering occurs within a mapped userspace memory buffer (`sys_gui_map_surface`).
- **Required Bridge**: A mechanism for the video pipeline to render into or share an ARGB32 buffer with a userspace `bos::Image` or `bos::Widget`.

---

## 5. Summary & Reusability Assessment

1. **Reusable Assets**:
   - `avi_parser.c`: Can be reused directly for RIFF/AVI demuxing.
   - `mjpeg_decoder.c`: Can be reused directly for JPEG/MJPEG video decoding.
   - `software_backend.c` (BT.601 LUT Converter): Can be reused to convert YUV420P frames to ARGB32.
2. **Components Requiring Rework / Replacement**:
   - `mp4_parser.c`: Needs complete sample table parsing (`stco`, `stsz`, `stsc`) to read real payloads from `mdat`.
   - `h264_decoder.c`: Must be replaced with a genuine baseline/constrained baseline H.264 decoder.
   - Video Presentation: Must be decoupled from kernel BWE and routed into userspace `bos::Surface`.
