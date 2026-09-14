# ATOMS OS — Phase 2 Production Media Engine Build Report
**Subsystem:** Userspace Media Engine ➔ Real Mature Media Integration  
**Milestone:** Phase 2 Build Certification  
**Target Architecture:** x86_64 Freestanding Ring-3 Userspace  
**Date:** September 12, 2026  
**Status:** **BUILD CLEAN — 0 COMPILATION ERRORS, 0 LINK ERRORS**  

---

## 1. Toolchain & Freestanding Compilation Flags

All Phase 2 media engine components are built in strict compliance with the **ATOMS OS Freestanding Userspace Specification**:
- Target: `x86_64-unknown-none` / `x86_64-elf`
- Code Model: `-mcmodel=small`
- Memory Safety: `-mno-red-zone`, `-fno-pic`, `-fno-pie`
- C++ Runtime Decoupling: `-fno-rtti`, `-fno-exceptions`, `-fno-threadsafe-statics`
- Optimization: `-O2`

### Exact Compiler Flags Used:
```bash
clang -target x86_64-unknown-none -ffreestanding -mno-red-zone \
      -fno-pie -fno-pic -fno-rtti -fno-exceptions \
      -mcmodel=small -O2 -Iuserspace/libbos_media/include ...
```

---

## 2. Static Archive Inventory

### 2.1 `build/libu_h264.a` (29 Object Modules)
Contains the upstream Hantro G1 H.264 video decoding engine augmented with FFmpeg `libavcodec` CABAC tables:

| Module Name | Source Origin | License | Role / Functionality |
|:---|:---|:---|:---|
| `u_h264bsd_byte_stream.o` | Hantro G1 Core | Apache 2.0 | Annex B byte stream NAL extraction & emulation prevention byte removal |
| `u_h264bsd_cabac.o` | Hantro G1 Core | Apache 2.0 | Context-adaptive binary arithmetic decoding syntax parsing |
| `u_h264bsd_cavlc.o` | Hantro G1 Core | Apache 2.0 | Context-adaptive variable-length coding residual decoding |
| `u_h264bsd_conceal.o` | Hantro G1 Core | Apache 2.0 | Transmission loss / corrupted slice error concealment |
| `u_h264bsd_deblocking.o`| Hantro G1 Core | Apache 2.0 | In-loop adaptive deblocking filter (H.264 standard compliant) |
| `u_h264bsd_decoder.o` | Hantro G1 Core | Apache 2.0 | Top-level H.264 slice decoding state machine |
| `u_h264bsd_dpb.o` | Hantro G1 Core | Apache 2.0 | Decoded Picture Buffer (DPB) reference frame lifecycle management |
| `u_h264bsd_image.o` | Hantro G1 Core | Apache 2.0 | YUV420 planar buffer allocation and stride calculation |
| `u_h264bsd_inter_prediction.o` | Hantro G1 Core | Apache 2.0 | Quarter-pel motion compensation & P/B macroblock interpolation |
| `u_h264bsd_intra_prediction.o` | Hantro G1 Core | Apache 2.0 | 4x4, 8x8, 16x16 luma and chroma spatial intra prediction modes |
| `u_h264bsd_macroblock_layer.o` | Hantro G1 Core | Apache 2.0 | Macroblock header decoding and transform coefficient reconstruction |
| `u_h264bsd_nal_unit.o` | Hantro G1 Core | Apache 2.0 | NAL unit type decoding (SPS, PPS, IDR, non-IDR, SEI) |
| `u_h264bsd_neighbour.o`| Hantro G1 Core | Apache 2.0 | Spatial motion vector and intra prediction neighbor lookup |
| `u_h264bsd_pic_order_cnt.o` | Hantro G1 Core | Apache 2.0 | Picture Order Count (POC) computation (Types 0, 1, 2) |
| `u_h264bsd_pic_param_set.o` | Hantro G1 Core | Apache 2.0 | Picture Parameter Set (PPS) parser and activation |
| `u_h264bsd_reconstruct.o` | Hantro G1 Core | Apache 2.0 | Residual add and saturation clipping to 8-bit luma/chroma samples |
| `u_h264bsd_sei.o` | Hantro G1 Core | Apache 2.0 | Supplemental Enhancement Information parsing |
| `u_h264bsd_seq_param_set.o` | Hantro G1 Core | Apache 2.0 | Sequence Parameter Set (SPS) parser (resolution, profile, level) |
| `u_h264bsd_slice_data.o` | Hantro G1 Core | Apache 2.0 | Slice data macroblock traversal loop |
| `u_h264bsd_slice_group_map.o` | Hantro G1 Core | Apache 2.0 | Flexible Macroblock Ordering (FMO) slice group mapping |
| `u_h264bsd_slice_header.o` | Hantro G1 Core | Apache 2.0 | Slice header syntax decoding and reference list reordering |
| `u_h264bsd_storage.o` | Hantro G1 Core | Apache 2.0 | Parameter storage and active configuration state |
| `u_h264bsd_stream.o` | Hantro G1 Core | Apache 2.0 | Bitstream reader with bit-exact Exp-Golomb decoding |
| `u_h264bsd_transform.o`| Hantro G1 Core | Apache 2.0 | Inverse 4x4 integer discrete cosine transform (IDCT) |
| `u_h264bsd_util.o` | Hantro G1 Core | Apache 2.0 | Freestanding memory utilities and bitwise helpers |
| `u_h264bsd_vlc.o` | Hantro G1 Core | Apache 2.0 | Variable length code tables |
| `u_h264bsd_vui.o` | Hantro G1 Core | Apache 2.0 | Video Usability Information parser (aspect ratio, timing) |
| `u_h264_cabac.o` | FFmpeg libavcodec | LGPL v2.1+ | Upstream CABAC state transitions (`ff_h264_lps_range`, `ff_h264_mps_state`, `ff_h264_lps_state`) |
| `u_h264_decoder.o` | ATOMS Native | MIT | High-level freestanding wrapper bridging NAL ingestion to YUV output |

---

### 2.2 `build/libbos_media.a` (7 Object Modules)
Contains the userspace demuxing, decoding, streaming, and audio playback framework:

| Module Name | Role / Functionality | Upstream Source / Standard | License |
|:---|:---|:---|:---|
| `bos_media_stream.o` | Freestanding VFS Stream I/O adapter using `SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE` | ATOMS OS Native | MIT |
| `mp4_demuxer.o` | Dynamic ISO Base Media File Format parser (`ftyp`, `moov`, `trak`, `avcC`, `stsz`, `stco`, `co64`) | ISO/IEC 14496-12 / 14496-15 | MIT |
| `mp3_decoder.o` | Freestanding MPEG-1/2 Audio Layer III decoder yielding 16-bit stereo PCM | minimp3 | CC0-1.0 |
| `bos_media_pipeline.o` | Playback controller: Demux $\to$ Decode $\to$ BT.709 Color Convert $\to$ BOSurface Blit $\to$ Audio HAL | ATOMS OS Native | MIT |
| `bos_media.o` | Public `bos_media_*` API implementation replacing legacy mock adaptors | ATOMS OS Native | MIT |
| `bos_media_telemetry.o`| Forensic serial telemetry emitter (`[MEDIA-P2]`) | ATOMS OS Native | MIT |
| `audio_user.o` | Ring-3 Audio HAL syscall wrapper (`SYS_AUDIO_CALL`) | ATOMS OS Native | MIT |

---

## 3. Final Binary Layout (`build/media_player.elf`)

Inspection of `build/media_player.elf` via `llvm-objdump -h`:

```text
build\media_player.elf: file format elf64-x86-64

Sections:
Idx Name      Size     VMA              Type
  1 .text     000376c9 0000000040000000 TEXT
  2 .rodata   0000fe38 0000000040038000 DATA
  3 .data     00000710 0000000040048000 DATA
  4 .bss      0003bf94 0000000040049000 BSS
```

### Memory Footprint & Guard Page Clearance:
- **Base Address:** `0x40000000`
- **Code Segment (`.text`):** 227,017 bytes (VMA `0x40000000` - `0x400376C9`)
- **Read-Only Data (`.rodata`):** 65,080 bytes (VMA `0x40038000` - `0x40047E38`)
- **Initialized Data (`.data`):** 1,808 bytes (VMA `0x40048000` - `0x40048710`)
- **BSS Segment (`.bss`):** 245,652 bytes (VMA `0x40049000` - `0x40084F94`)
- **Total Loaded Memory Size:** 544,660 bytes (~532 KB)
- **User Stack Top:** `0x40100000`
- **Stack Base (4 pages):** `0x400FC000`
- **Stack Guard Page:** `0x400FB000`
- **Available Headroom Below Guard Page:** `0x400FB000 - 0x40084F94 = 0x0007606C` (483,436 bytes / ~472 KB)

**Verdict:** Zero stack collision risk. Clean isolation between binary images and thread stack pages.
