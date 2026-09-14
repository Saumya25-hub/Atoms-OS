# ATOMS OS — Phase 2 Architecture Specification
**Subsystem:** Userspace Media Engine ➔ Real Mature Media Integration  
**Milestone:** Phase 2C (Architecture & Adapter Specification)  
**Date:** September 12, 2026  
**Status:** **APPROVED FOR IMPLEMENTATION (RULE 0 ENFORCED)**  

---

## 1. High-Level Architecture

Phase 2 replaces the mock libmpv layer with a **genuine, native, freestanding media engine** built on proven open-source foundations (FFmpeg CABAC + Hantro G1 H.264 + minimp4 + minimp3 + dr_wav + ATOMS Native Syscall Bridges):

```text
+-----------------------------------------------------------------------------------+
|                         RING-3 USERSPACE MEDIA PLAYER                             |
|                                                                                   |
|  userspace/apps/media_player/main.cpp (MediaCenterWidget & GUI Event Loop)        |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|                           ATOMS NATIVE MEDIA API                                  |
|                                                                                   |
|  userspace/libbos_media/include/bos_media.h                                       |
|  - bos_media_create(), bos_media_open(), bos_media_play(), bos_media_render()     |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
+-----------------------------------------------------------------------------------+
|                        BOS MEDIA PIPELINE CONTROLLER                              |
|                                                                                   |
|  userspace/libbos_media/src/bos_media_pipeline.cpp                                |
|  - Container Detection (MP4, MP3, WAV, FLAC)                                      |
|  - Stream Demuxing & Packet Dispatch                                              |
|  - Video/Audio Timestamp Tracking & Frame Clock Pacing                            |
+------------------------------------+----------------------------------------------+
                  |                                                  |
                  v                                                  v
+-----------------------------------+             +----------------------------------+
|      BOS MEDIA STREAM (AVIO)      |             |         VIDEO ACCEL HAL          |
|                                   |             |                                  |
|  userspace/libbos_media/stream/   |             |  Hardware Probe (PCI Bus)        |
|  - SYS_OPEN, SYS_READ, SYS_SEEK   |             |  -> HW_ACCEL_INIT: NOT_IMPL      |
|  - Strictly no host file APIs     |             |  -> CPU_FALLBACK: ACTIVE         |
+-----------------------------------+             +----------------------------------+
                  |                                                  |
                  v                                                  v
+-----------------------------------+             +----------------------------------+
|      CONTAINER DEMUXERS           |             |       VIDEO DECODER ENGINE       |
|                                   |             |                                  |
|  - MP4: mp4_demux.c (minimp4)     |             |  Hantro G1 H.264 Core (AOSP)     |
|    Parses ftyp, moov, trak, avcC, |             |  + FFmpeg libavcodec CABAC       |
|    stsz, stco to extract NALs     |             |  Outputs raw planar YUV420P      |
+-----------------------------------+             +----------------------------------+
                  |                                                  |
                  v                                                  v
+-----------------------------------+             +----------------------------------+
|       AUDIO DECODER ENGINE        |             |    COLOR CONVERSION & BLITTING   |
|                                   |             |                                  |
|  - MP3: minimp3 (MPEG-1/2/3)      |             |  YUV420P -> ARGB32 BT.709/BT.601 |
|  - WAV: dr_wav (PCM / Float)      |             |  Aspect-Ratio Letterbox Math     |
|  - FLAC: dr_flac (Lossless)       |             |  Frame CRC32 Telemetry           |
+-----------------------------------+             +----------------------------------+
                  |                                                  |
                  v                                                  v
+-----------------------------------+             +----------------------------------+
|         BOS AUDIO HAL             |             |       BOSURFACE v2.5 BLIT        |
|                                   |             |                                  |
|  userspace/libs/audio/audio_user  |             |  Mapped User Address 0x51800000  |
|  - SYS_AUDIO_CALL (ID=43)         |             |  - SYS_GUI_MAP_SURFACE (ID=20)   |
|  - 48kHz S16_LE Stereo Output     |             |  - SYS_GUI_SHOW_WINDOW (ID=18)   |
+-----------------------------------+             +----------------------------------+
                  |                                                  |
                  +-------------------------+------------------------+
                                            | (Hardware Syscalls)
                                            v
+-----------------------------------------------------------------------------------+
|                        RING-0 KERNEL COMPOSITOR & AUDIO                           |
|                                                                                   |
|  - BCM Compositor Thread (Authoritative 60 FPS Presentation)                      |
|  - Intel HDA Audio Mixer & DMA Engine                                             |
|  - Physical GPU / Display Controller                                              |
+-----------------------------------------------------------------------------------+
```

---

## 2. Component Specifications

### 2.1 `BOSMediaStream` (VFS Stream Abstraction)
- Interfaces:
  ```c
  typedef struct BOSMediaStream {
      int fd;
      uint64_t size;
      uint64_t position;
      int (*read)(struct BOSMediaStream* s, void* buf, size_t count);
      int (*seek)(struct BOSMediaStream* s, int64_t offset, int whence);
      int64_t (*tell)(struct BOSMediaStream* s);
      void (*close)(struct BOSMediaStream* s);
  } BOSMediaStream;
  ```
- Backed 100% by ATOMS VFS syscalls: `SYS_OPEN` (14), `SYS_READ` (15), `SYS_SEEK` (26), `SYS_CLOSE` (25).

### 2.2 Container Demuxing Pipeline
- Dynamic ISO box walker:
  1. `ftyp`: Verifies compatibility brands (`isom`, `iso2`, `mp41`, `mp42`, `dash`).
  2. `moov` $\to$ `trak`: Enumerates tracks (Video type `vide`, Audio type `soun`).
  3. `mdia` $\to$ `minf` $\to$ `stbl`:
     - `stsd` $\to$ `avcC`: Extracts NAL length size, SPS (Sequence Parameter Set), and PPS (Picture Parameter Set).
     - `stts`: Frame duration and time base.
     - `stsz`: Sample size table.
     - `stco` / `co64`: Chunk offset table.
     - `stsc`: Sample-to-chunk map.
- Sample extraction:
  - Dynamically calculates byte offset for sample $N$.
  - Converts MP4 4-byte length headers into Annex B start codes (`0x00 0x00 0x00 0x01`).

### 2.3 Video Decoding Pipeline (H.264 High Profile CABAC)
- Decoder state allocation:
  - Allocates `storage_t` using user heap (`malloc` / `SYS_ALLOC`).
  - Initializes with `h264bsdInit(storage, 1 /* no_reorder */)`.
- Feed sequence:
  1. Submit SPS NAL unit $\to$ extracts picture width, height, aspect ratio, profile, level.
  2. Submit PPS NAL unit $\to$ sets entropy coding mode (0 = CAVLC, 1 = CABAC).
  3. Submit Slice NAL units per frame.
  4. On `H264BSD_PIC_RDY`, retrieves planar YUV420P buffer via `h264bsdNextOutputPicture()`.

### 2.4 Color Conversion & BOSurface Frame Presentation
- ITU-R BT.709 matrix for HD video (1080p / 720p):
  $$\begin{aligned}
  R &= 1.164383(Y - 16) + 1.792741(V - 128) \\
  G &= 1.164383(Y - 16) - 0.213249(U - 128) - 0.532909(V - 128) \\
  B &= 1.164383(Y - 16) + 2.112402(U - 128)
  \end{aligned}$$
- Direct blit to client window surface mapped via `SYS_GUI_MAP_SURFACE`.
- Aspect-ratio letterboxing to prevent stretching or distortion.
- CRC32 verification over decoded frames.

### 2.5 Audio Decoding & Presentation
- MP3 stream decode via `minimp3`:
  - Parses MP3 frames from file offset.
  - Emits 16-bit signed stereo PCM samples.
  - Streams to kernel mixer via `audio_stream_write()` using `SYS_AUDIO_CALL`.
- WAV / FLAC stream decode via `dr_wav` and `dr_flac`.

### 2.6 Error Isolation & Failure Guarantees
- Out-of-range NAL units or truncated files return `BOS_MEDIA_ERROR_INVALID_BITSTREAM`.
- All allocations are bounded; DPB capped at 8 frames to prevent RAM exhaustion.
- Errors log structured telemetry:
  `[MEDIA-P2] FIRST_FAILURE=CORRUPT_SLICE`
  `[MEDIA-P2] FAILURE_COMPONENT=H264Decoder`
- Zero kernel panics; media player process exits or displays error without affecting the desktop shell.
