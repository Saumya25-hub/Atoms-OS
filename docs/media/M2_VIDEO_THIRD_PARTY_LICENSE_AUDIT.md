# ATOMS OS — Phase M2: Third-Party Media Component License & Dependency Audit
**Protocol Stage**: Task 1 (Forensic Team)  
**Date**: September 10, 2026  
**Auditor**: ATOMS OS Architecture & Compliance  
**Status**: AUDIT COMPLETE — LICENSE VERIFIED PERMISSIVE

---

## 1. Compliance Principles & Requirements

Per ATOMS OS Engineering Protocol V1:
1. **Zero GPL / LGPL**: No GPL-2.0, GPL-3.0, LGPL-2.1, or LGPL-3.0 licensed code may be incorporated into the ATOMS kernel.
2. **Component Isolation**: All external third-party implementations must reside exclusively under `third_party/media/`.
3. **Freestanding Compatibility**: All external code must compile cleanly with `-ffreestanding -msoft-float -mno-sse -mno-sse2` without linking to standard C libraries (`libc`) or floating-point registers.
4. **Permissive Licenses Only**: Only BSD (2-Clause / 3-Clause), MIT, Apache-2.0, or CC0 / Public Domain are acceptable.

---

## 2. Evaluated Candidates

### 2.1 H.264 / AVC Video Decoder Candidates

| Candidate | License | Floating Point / SSE | Freestanding Portability | Verdict |
|---|---|---|---|---|
| **`h264bsd`** (Google / Android AOSP / PacketVideo) | **Apache-2.0** | **Integer only** (zero float, zero SSE/AVX) | **Exceptional**. Self-contained ANSI C. Custom allocator hooks (`H264SwDecMalloc`/`Free`). | ✅ **SELECTED** |
| **`libavcodec` / `FFmpeg`** | LGPL-2.1+ / GPL-2.0+ | Heavy FP, SIMD, POSIX threads | Non-trivial. Violates license rule (LGPL/GPL). | ❌ **REJECTED** |
| **`openh264`** (Cisco) | BSD-2-Clause | Requires C++, threading, complex SIMD assembly | High build complexity; large footprint; requires float emulation. | ❌ **REJECTED** |
| **`wels` / `tinyh264`** | BSD / Apache | Mixed | High maintenance burden. | ❌ **REJECTED** |

#### In-Depth Audit: `h264bsd`
- **Origin**: Developed by PacketVideo, maintained as part of Android Open Source Project (AOSP) Stagefright multimedia framework (`platform/frameworks/av/media/libstagefright/codecs/on2/h264dec`).
- **License**: Apache License, Version 2.0.
  - Allows commercial and proprietary use, modification, and distribution.
  - Compatible with ATOMS OS kernel licensing.
  - Explicit patent grant included under Section 3 of Apache-2.0.
- **Dependencies**:
  - Zero external library dependencies.
  - Memory allocation is isolated behind `h264bsd_util.c` (`H264SwDecMalloc`, `H264SwDecFree`), which can be directly mapped to `bospectra_mem_alloc` and `bospectra_mem_free`.
  - Math is 100% 32-bit fixed-point and integer arithmetic.

---

### 2.2 MP4 Demuxer Candidates

| Candidate | License | Dependencies | Freestanding Portability | Verdict |
|---|---|---|---|---|
| **`minimp4`** (lieff) | **CC0-1.0 / Public Domain** | Standalone header/source | **Excellent**. Pure C99, stream callback interface, parses `stsd`, `stts`, `stsc`, `stsz`, `stco`, `co64`, `avcC`. | ✅ **SELECTED** |
| **`libmp4v2`** | MPL-1.1 | C++ classes, file system assumptions | Hard to compile in freestanding kernel. | ❌ **REJECTED** |
| **`GPAC / MP4Box`** | LGPL-2.1 | Large codebase, POSIX OS dependencies | Violates license rule. | ❌ **REJECTED** |
| **Native ATOMS Demuxer** | BOS-owned | Integrates with `bospectra_file` | Direct integration, zero external dependencies. | ✅ **SELECTED (Enhanced)** |

#### In-Depth Audit: `minimp4`
- **Origin**: Created by lieff, widely used in game engines, embedded tools, and lightweight players.
- **License**: CC0 1.0 Universal (Public Domain Dedication) / Unlicense.
  - Zero restrictions on use, copying, modification, or distribution.
- **Capabilities**:
  - Parses ISO Base Media File Format (`moov`, `trak`, `mdia`, `minf`, `stbl`).
  - Reads `stsd` (`avc1`, `avcC`, SPS, PPS).
  - Computes exact sample byte offsets and sizes using `stsc`, `stco`, `stsz`.
  - Stream callbacks allow seamless integration with `bospectra_file_read` and `bospectra_file_seek`.

---

## 3. Directory Layout & Isolation Plan

All third-party code will be cleanly isolated inside:
```
third_party/media/
├── LICENSE.APACHE-2.0
├── LICENSE.CC0-1.0
├── h264/
│   ├── include/
│   │   ├── h264bsd_decoder.h
│   │   ├── h264bsd_container.h
│   │   ├── h264bsd_util.h
│   │   └── ...
│   └── src/
│       ├── h264bsd_decoder.c
│       ├── h264bsd_nal_unit.c
│       ├── h264bsd_seq_param_set.c
│       ├── h264bsd_pic_param_set.c
│       ├── h264bsd_slice_header.c
│       ├── h264bsd_slice_data.c
│       ├── h264bsd_macroblock_layer.c
│       ├── h264bsd_intra_prediction.c
│       ├── h264bsd_inter_prediction.c
│       ├── h264bsd_transform.c
│       ├── h264bsd_vlc.c
│       ├── h264bsd_deblocking.c
│       └── h264bsd_storage.c
└── mp4/
    ├── include/
    │   └── minimp4.h
    └── src/
        └── minimp4_demux.c
```

The native BOS integration remains strictly inside:
- `kernel/media/bospectra/container/mp4/mp4_parser.c`
- `kernel/media/bospectra/decoder/h264/h264_decoder.c`
- `kernel/media/bospectra/render/backends/software/software_backend.c`
- `kernel/shell/apps/bos_media_player/`

---

## 4. Final Verdict

- **GPL / LGPL Code**: **0%** (Strictly Zero).
- **Approved Licenses**: **Apache-2.0** (`h264bsd`) & **CC0-1.0** (`minimp4`).
- **Architectural Approval**: **APPROVED** for Phase M2 implementation.
