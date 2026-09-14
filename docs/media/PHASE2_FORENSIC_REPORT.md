# ATOMS OS — Phase 2 Forensic Investigation Report
**Subsystem:** Userspace Media Engine ➔ Real Mature Media Integration  
**Milestone:** Phase 2A (Forensic Audit & Source Integrity Inspection)  
**Date:** September 12, 2026  
**Status:** **FORENSIC AUDIT COMPLETE — BASELINE RECORDED (RULE 0 ENFORCED)**  

---

## 1. Executive Summary

In accordance with **Rule 0 (Forensic First, Code Second)** and the ATOMS OS Phase 2 Engineering Protocol, this document establishes the authoritative forensic audit of all existing media-related subsystems, code paths, third-party libraries, and interfaces across the ATOMS OS codebase.

Phase 1 successfully established the architectural boundary:
- Media execution operates strictly in **Ring-3 Userspace** (`CS & 3 == 3`).
- Explorer launches `/media_player.elf` via `sys_service_exec()`.
- The BCM authoritative 60 FPS compositor and BWE window loops are completely decoupled from media decode operations.
- Userspace VFS syscalls (`SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`) operate without page faults under user CR3.
- Userspace BOSurface mapping (`SYS_GUI_MAP_SURFACE`) and Audio HAL streaming (`SYS_AUDIO_CALL`) are operational.

However, the internal media engine in userspace (`userspace/libbos_media/`) is currently an **immature mock/stub layer**. This forensic audit exposes all mock and legacy code, inventories existing genuine upstream code, and defines the roadmap for real upstream media engine integration.

---

## 2. Component Forensic Classification Matrix

| Component | Repository Path | Forensic Classification | Upstream Origin / License | Current Operational Reality | Action Required in Phase 2 |
|:---|:---|:---:|:---|:---|:---|
| `mpv_events.cpp` | `userspace/libbos_media/mpv/` | **MOCK / STUB** | ATOMS Native Stub / Freestanding | Implements dummy `mpv_create()`, `mpv_render_context_render()`, returning fake success without rendering any pixels. | **DELETE / REPLACE** with genuine engine adapter. |
| `mpv_instance.cpp` | `userspace/libbos_media/mpv/` | **MOCK / STUB** | ATOMS Native Stub | Contains hardcoded format categorizations based on file extension; does not demux bitstream. | **REPLACE** with genuine stream demux bridge. |
| `mpv_video_output.cpp` | `userspace/libbos_media/mpv/` | **PARTIAL STUB** | ATOMS Native UI Bridge | Contains genuine letterboxing and CRC32 calculation, but render call routes to stub `mpv_render_context_render()`. | **REFACTOR** into `BOSMediaFrame` $\to$ `BOSurface` bridge. |
| `mpv_audio_output.cpp` | `userspace/libbos_media/mpv/` | **REAL BRIDGE** | ATOMS Audio HAL / `SYS_AUDIO_CALL` | Genuine syscall bridge driving ATOMS Audio HAL via `SYS_AUDIO_CALL` (43). | **REUSE** as backend for genuine audio decode. |
| `mpv/include/` | `third_party/media/mpv/include/` | **HEADERS ONLY** | upstream mpv (LGPLv2.1+) | Only upstream headers (`client.h`, `render.h`, `stream_cb.h`); zero upstream C/C++ source code present. | **AUDIT** suitability or supersede with direct FFmpeg/codec integration. |
| `h264bsd` Core | `third_party/media/h264/src/` | **GENUINE UPSTREAM** | Android AOSP Stagefright / Hantro G1 (Apache 2.0) | 27 source files; fully freestanding C89/C99 H.264 baseline/main/high decoder. | **REUSE** in userspace media engine. |
| `h264bsd_cabac.c` | `third_party/media/h264/src/` | **GENUINE UPSTREAM** | FFmpeg `libavcodec` (Michael Niedermayer, LGPLv2.1+) | Authentic FFmpeg arithmetic coding tables and CABAC state transition engine. | **REUSE** for H.264 High Profile CABAC decoding. |
| `mp4_demux.c` | `third_party/media/mp4/src/` | **KERNEL TIED** | minimp4 (Dmitry Boldyrev, CC0) | Real ISO box parser (`ftyp`, `moov`, `trak`, `avcC`, `stsz`, `stco`), but hard-linked to kernel memory and kernel debug headers. | **REFACTOR** into freestanding userspace `BOSMediaStream` demuxer. |
| `minimp3.h` | `third_party/audio/mp3/include/` | **GENUINE UPSTREAM** | Lieven van der Velden (CC0 / Public Domain) | Production-grade, zero-dependency MP3 (MPEG Layer 1/2/3) decoder. | **REUSE** for real userspace MP3 decoding. |
| `dr_wav.h` | `third_party/audio/wav/include/` | **GENUINE UPSTREAM** | David Reid (Public Domain / MIT-0) | Complete PCM / IEEE Float WAV decoder. | **REUSE** for real userspace WAV decoding. |
| `dr_flac.h` | `third_party/audio/flac/include/` | **GENUINE UPSTREAM** | David Reid (Public Domain / MIT-0) | Complete FLAC lossless decoder. | **REUSE** for real userspace FLAC decoding. |
| In-Kernel BOSpectra | `kernel/media/bospectra/` | **LEGACY (RING-0)** | ATOMS OS Project | Fragmented in-kernel media engine. Gated in Phase 1. | **MAINTAIN ISOLATION**; do not call from userspace. |
| `test_h264_real.c` | `build/` | **LEGACY TEST** | ATOMS OS Scratch | Hardcoded hex offsets (`0x244eb98`, `0x2450eb0`) and static SPS/PPS arrays. | **DEPRECATE**; replace with dynamic test suite. |

---

## 3. Deep Forensic Findings

### 3.1 The "Fake MPV" Layer (`userspace/libbos_media/mpv/`)
- **Finding**: While `mpv_adapter.h` declares the libmpv client API, `mpv_events.cpp` provides a hand-written mock implementation.
- **Evidence**:
  ```cpp
  // mpv_events.cpp:256
  int mpv_render_context_render(mpv_render_context* ctx, mpv_render_param* params) {
      if (!ctx || !params) return MPV_ERROR_INVALID_PARAMETER;
      // ... extracts pointers ...
      return MPV_ERROR_SUCCESS; // <--- NO RENDERING, ZERO DECODED FRAMES!
  }
  ```
- **Architectural Violation**: Violates Absolute Rule #1 (NO FAKE ENGINE).
- **Remedy**: Eliminate the mock libmpv functions. Replace them with a transparent, genuine ATOMS Media Engine directly wired to real demuxers and decoders.

### 3.2 Decoupling of `mp4_demux.c` from Kernel Headers
- **Finding**: `third_party/media/mp4/src/mp4_demux.c` has a complete ISO base media file format parser, but includes:
  ```c
  #include "kernel/media/bospectra/memory/bospectra_memory.h"
  #include "kernel/media/bospectra/debug/bospectra_debug.h"
  ```
- **Remedy**: Decouple `mp4_demux.c` so that it depends only on standard C types (`stdint.h`, `stddef.h`, `string.h`) and uses the `BOSMediaStream` VFS adapter (`SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`).

### 3.3 Genuine Upstream Code Already Present in Repo
The repository already contains mature, battle-tested, upstream open-source code ready for userspace linking:
1. **FFmpeg `libavcodec` CABAC Tables & State Transitions**: `third_party/media/h264/src/h264bsd_cabac.c` (Michael Niedermayer, LGPLv2.1+).
2. **Hantro G1 / Android AOSP H.264 Core**: `third_party/media/h264/src/` (Apache 2.0), capable of full 1080p H.264 High Profile decoding.
3. **minimp3 Engine**: `third_party/audio/mp3/include/minimp3.h` (CC0 / Public Domain), capable of full MP3 stream decoding to 16-bit PCM.
4. **dr_wav & dr_flac Engines**: `third_party/audio/wav/include/dr_wav.h` and `third_party/audio/flac/include/dr_flac.h` (Public Domain / MIT-0).

### 3.4 Genuine Upstream Source Still Missing for Full FFmpeg Parity
1. General FFmpeg container demuxers (`libavformat` multi-format demuxing: MKV, AVI, TS, MOV).
2. High-performance software color scaler (`libswscale` / YUV420P to ARGB32 SIMD conversions).

---

## 4. Hardware Acceleration & CPU Fallback Analysis

- **Hardware Probe**: The existing HAL (`video_accel.c`) queries PCI vendors (`0x8086` Intel, `0x10DE` NVIDIA, `0x1002` AMD).
- **Operational Reality**: In QEMU and current H81 firmware environments, hardware command rings for NVDEC / VA-API are not yet initialized in Ring 3.
- **Protocol Mandate**: Phase 2 must explicitly report:
  ```text
  [MEDIA-P2] HW_ACCEL_PROBE=PCI_VENDOR_DETECTED
  [MEDIA-P2] HW_ACCEL_INIT=NOT_IMPLEMENTED
  [MEDIA-P2] CPU_FALLBACK=ACTIVE
  ```
  Zero fake hardware claims. Software CPU decoding must decode every frame accurately.

---

## 5. Risk Analysis & Mitigation

1. **Memory Exhaustion on 1080p Video**:
   - 1080p YUV420P frame = $1920 \times 1080 \times 1.5 = 3,110,400$ bytes (~3.1 MB) per frame.
   - A DPB (Decoded Picture Buffer) of 16 frames requires ~50 MB of RAM.
   - *Mitigation*: Ensure user address space heap allocations (`atoms_malloc` / `SYS_ALLOC`) have bounded DPB limits (max 4-8 reference frames for 1080p playback in memory-constrained environments).
2. **Corrupted Bitstream Safety**:
   - Untrusted input files must never trigger out-of-bounds pointer writes.
   - *Mitigation*: Bounded NAL unit size checks, validated slice headers, and error return codes propagating to userspace telemetry without crashing the process.

---

## 6. Conclusion & Roadmap

- **What must be deleted**: `userspace/libbos_media/mpv/mpv_events.cpp` (the fake MPV mock).
- **What must be refactored**: `mp4_demux.c` (remove kernel dependencies, bind to `BOSMediaStream`).
- **What must be reused**: `h264bsd` + FFmpeg CABAC engine, `minimp3`, `dr_wav`, `audio_user.c`, `atoms_syscall.h`.
- **What must be built**: `BOSMediaStream` (VFS AVIO bridge), `BOSMediaFrame` (YUV420P $\to$ ARGB32 converter), genuine pipeline controller.
