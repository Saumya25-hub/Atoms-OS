# ATOMS OS / BOS — MEDIA SUBSYSTEM ARCHITECTURE & FORENSIC MAP
**Document ID**: `docs/media/MEDIA_ARCHITECTURE.md`  
**Subsystem**: Native Media Pipeline, Universal Hardware Video Acceleration HAL, BOSurface v2.5 Integration  
**Date**: September 10, 2026  
**Status**: UPDATED WITH UNIVERSAL HARDWARE VIDEO ACCELERATION ARCHITECTURE  

---

## 1. End-to-End Architecture Map

The following diagram maps the complete, unified data flow in ATOMS OS, integrating the Universal Video Acceleration HAL, multi-vendor GPU backends, and certified audio/video pipelines:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           APPLICATION / PRESENTATION LAYER                      │
│                                                                                 │
│   [Flagship Media Player]                          [Modern Userspace Apps]      │
│   bos_media_player.c (BWE Surface)                 C++ BOSurface v2.5 Apps      │
│   (Playback Controls, Timeline, Canvas)            (SmartLayout, Widgets, HW)   │
└───────────────────────┬─────────────────────────────────┬───────────────────────┘
                        │                                 │
                        ▼                                 ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          BOSPECTRA MULTIMEDIA ENGINE                            │
│                                                                                 │
│  [Unified Containers]                                                           │
│   ├── AVI Demuxer (avi_parser.c)    🟢 REAL (RIFF/AVI, MJPEG/PCM stream demux)  │
│   ├── MP4 Demuxer (mp4_parser.c)    🟢 REAL (minimp4, dual-stream video+audio)  │
│   └── MKV Demuxer (mkv_parser.c)    🟢 REAL (EBML elements, Cluster packets)    │
│                                                                                 │
│  [Synchronizer & Master Clock]                                                  │
│   ├── Monotonic Wall Clock          🟢 REAL (timer_get_ticks() * 1000ULL)       │
│   ├── A/V Sync Pacer                🟢 REAL (-40ms to +15ms drift tolerance)    │
│   └── Atomic Seek Engine            🟢 REAL (Queue flush, DPB reset, clock sync)│
└───────────────┬─────────────────────────────────────────────────┬───────────────┘
                │ Compressed Video                                │ Compressed Audio
                │ Elementary Packets                              │ Elementary Packets
                ▼                                                 ▼
┌───────────────────────────────────────────┐     ┌───────────────────────────────┐
│      BOS VIDEO ACCELERATION HAL           │     │     BOS CODEC REGISTRY        │
│      video_accel/common/                  │     │  WAV, MP3, FLAC, AAC, Vorbis  │
│                                           │     └───────────────┬───────────────┘
│  [Policy Engine & Capability Negotiator]  │                     │ Decoded PCM
│  Stream Profile/Level/Resolution Matching │                     ▼
└───────┬───────────┬───────────┬───────────┘     ┌───────────────────────────────┐
        │           │           │                 │   UNIVERSAL SOFTWARE MIXER    │
        │           │           │                 │  16.16 Fixed-Point Resampler  │
        ▼           ▼           ▼                 │  Channel Matrix (Mono/Stereo) │
   ┌─────────┐ ┌─────────┐ ┌─────────┐            └───────────────┬───────────────┘
   │  Intel  │ │ NVIDIA  │ │   AMD   │                            │ 48kHz Stereo PCM
   │ Backend │ │ Backend │ │ Backend │                            ▼
   │  VDBox  │ │  NVDEC  │ │   VCN   │            ┌───────────────────────────────┐
   └────┬────┘ └────┬────┘ └────┬────┘            │       AUDIO HARDWARE HAL      │
        │           │           │                 │  Intel HDA (Realtek Codec)    │
        +─────┬─────+─────┬─────+                 │  Intel AC97 Legacy Driver     │
              │           │                       └───────────────┬───────────────┘
              │           │ HW Video                              │ DMA Ring
              │           │ Surfaces                              ▼
              │           ▼                       ┌───────────────────────────────┐
              │  ┌──────────────────────┐         │      PHYSICAL AUDIO OUT       │
              │  │  GPU Video Surfaces  │         │   Line Out / Headphone Jack   │
              │  │  NV12 / P010 / HW    │         └───────────────────────────────┘
              │  └────────┬─────────────┘
              │           │
   Hardware   │           │ Direct Scanout /
   Fallback   │           │ Low-Copy Blit
              ▼           ▼
   ┌────────────────────────────────────┐
   │    BOS C99 SOFTWARE DECODERS       │
   │  H.264 (1080p Crop), HEVC, VP8, VP9│
   │  Planar YUV420P -> BT.601 / BT.709 │
   └──────────────────┬─────────────────┘
                      │ Rendered ARGB32 Canvas
                      ▼
   ┌────────────────────────────────────┐
   │       BOSURFACE v2.5 COMPOSITOR    │
   │  Unified Z-Stack Window Manager    │
   │  2560x1600 @ 32bpp Linear Framebuf │
   └──────────────────┬─────────────────┘
                      │ Scanout Swap
                      ▼
   ┌────────────────────────────────────┐
   │       PHYSICAL DISPLAY (GOP)       │
   │  DisplayPort / HDMI / eDP Screen   │
   └────────────────────────────────────┘
```

---

## 2. Universal Hardware Video Acceleration Architecture

### 2.1 Philosophy & Vendor Neutrality
The Video Acceleration HAL acts as a vendor-neutral mediator between the BOSpectra media stream decoder manager and physical graphics processors:
- **No Single Vendor Bias**: The system does not assume an NVIDIA GPU exists, nor does it assume Intel or AMD.
- **Hardware Agnostic Interface**: Applications and the BOSpectra pipeline interact strictly with `bos_video_accel_*` APIs.
- **Dynamic Device Binding**: The HAL queries the kernel GPU manager (`kernel/graphics/gpu/core/gpu_manager.c`) for registered PCI display controllers (Class 0x03).

### 2.2 Vendor Backend Implementations

#### A. Intel Media Backend (`video_accel/intel/`)
- **Supported Architectures**: Gen7/Gen7.5 (Haswell), Gen8 (Broadwell), Gen9 (Skylake/Kaby Lake/Coffee Lake), Gen11 (Ice Lake), Gen12/Xe-LP (Tiger Lake/Alder Lake/Iris Xe).
- **Hardware Blocks**: MFX (Multi-Format Codec Engine), HCP (HEVC/VP9 Engine), VDBox instances (`VCS0`..`VCS3`).
- **Command Submission**: Ring buffer batch buffers mapped into GTT (`MI_BATCH_BUFFER_START`, `MI_FLUSH_DW`).
- **Surface Formats**: Tile-Y / Tile-4 NV12 (8-bit) and P010 (10-bit).

#### B. NVIDIA NVDEC Backend (`video_accel/nvidia/`)
- **Supported Architectures**: Pascal (GTX 10-Series), Turing (RTX 20-Series / GTX 16-Series), Ampere (RTX 30-Series), Ada Lovelace (RTX 40-Series).
- **Hardware Blocks**: Dedicated on-die NVDEC engines (decoupled from CUDA cores).
- **Command Submission**: Pushbuffer FIFO channel submission, semaphore synchronization.
- **Surface Formats**: NV12, P010 in dedicated VRAM. Direct zero-copy display scanout.

#### C. AMD VCN Backend (`video_accel/amd/`)
- **Supported Architectures**: GCN 4.0/5.0 (Polaris, Vega), RDNA 1.0/2.0 (Navi 1x/2x), RDNA 3.0 (Navi 3x).
- **Hardware Blocks**: Video Core Next (VCN 1.0, 2.0, 3.0, 4.0).
- **Command Submission**: Indirect Buffers (IB) enqueued to VCN Ring with completion fences.
- **Surface Formats**: NV12, P010 in GTT or on-card VRAM.

#### D. Software Fallback Backend (`video_accel/software/`)
- **Supported Codecs**: H.264 (Baseline/Main with 1080p macroblock crop), HEVC (H.265 NAL parser), VP8, VP9, MJPEG.
- **Color Conversion**: Fast integer pre-computed lookup tables for ITU-R BT.601 (SD) and ITU-R BT.709 (HD $\ge 720\text{p} / 1080\text{p}$).

---

## 3. Surface & Memory Hierarchy

To prevent wasteful GPU $\to$ CPU $\to$ GPU copy taxes, the surface subsystem distinguishes three operational domains:

| Memory Domain | Location | Accessibility | Use Case |
|---|---|---|---|
| `BOS_SURF_DOMAIN_CPU_RAM` | Host System Memory | CPU Read/Write; GPU via Host DMA | C99 Software decoders, audio PCM buffers |
| `BOS_SURF_DOMAIN_GTT_MAPPED` | Pinned System RAM | GPU Hardware Bus Master DMA; CPU Read | Intel VDBox coherent output, low-copy presentation |
| `BOS_SURF_DOMAIN_VRAM_LOCAL` | Dedicated On-Card Video RAM | High-bandwidth GPU Engine only; CPU via Aperture | NVIDIA NVDEC & AMD VCN decode surfaces; zero-copy scanout |

---

## 4. Hardware vs Software Stream Negotiation Policy

For every incoming media stream:
1. **Codec Identification**: Container demuxer determines FourCC (`avc1`, `hvc1`, `vp09`, `vp08`, `av01`).
2. **Capability Matching**:
   - Matches against primary GPU capability tables.
   - Verifies profile, level, chroma format (4:2:0 vs 4:4:4), bit depth (8-bit vs 10-bit), and frame dimensions.
3. **Decision & Routing**:
   - **Supported**: Allocates hardware decoder session on active vendor backend. Surfaces allocated in GPU memory domain.
   - **Unsupported / Limits Exceeded**: Routes immediately to the native C99 software decoder. Emits explicit fallback telemetry with exact reason.

---

## 5. Synchronization & Pacing Model

- **Hardware Fences**: Decoding operates asynchronously. The CPU advances pipeline clocks while the GPU processes bitstream slices. Presentation is gated on fence sequence verification.
- **Monotonic Clock**: Master clock ticks are derived from CPU hardware time stamps (`timer_get_ticks() * 1000ULL`).
- **A/V Sync Tolerance Window**:
  - $\Delta = \text{audio\_pts} - \text{video\_pts}$
  - In-Sync: $-40\,\text{ms} \le \Delta \le +15\,\text{ms}$.
  - Lagging ($\Delta > +15\,\text{ms}$): Drop non-reference video frame to catch up.
  - Leading ($\Delta < -40\,\text{ms}$): Delay presentation to align with audio.
- **Atomic Seek**: Flushes container demuxer, demux queue, decoded frame queue, decoder DPB, audio buffer, and resets the master clock.

---

## 6. Subsystem Verification & Certification Status

| Subsystem | Host Harness | UEFI QEMU | Physical Hardware | Milestone Verdict |
|---|---|---|---|---|
| **Universal Audio Engine (M1/M2)** | **PASS** (15/15 files) | **PASS** (HDA + AC97) | **CERTIFIED** (H81 Lynx Point) | **PASS** |
| **Video Bitstream Decoders (M4)** | **PASS** (1080p crop, math) | **PASS** (H.264/HEVC/VP8/VP9) | **PENDING BARE METAL** | **RUNTIME-VALIDATED** |
| **Unified Media Pipeline (M5)** | **PASS** (Pacer math) | **PASS** (Dual-stream MP4) | **PENDING BARE METAL** | **RUNTIME-VALIDATED** |
| **Video Accel HAL (Universal)** | **PASS** (Caps & policy) | **PASS** (Multi-backend routing)| **PENDING BARE METAL** | **SPECIFICATION LOCKED** |
