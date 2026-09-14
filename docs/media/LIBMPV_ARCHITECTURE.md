# ATOMS OS — Native Media Engine (libmpv Integration) Architecture
**Document ID:** `docs/media/LIBMPV_ARCHITECTURE.md`  
**Subsystem:** ATOMS Native Media Subsystem (`libbos_media`, `mpv_adapter`, `BOSurface v2.5`, `BOSpectra`, Audio HAL)  
**Date:** September 12, 2026  
**Status:** ARCHITECTURAL SPECIFICATION & DESIGN  

---

## 1. Architectural Mission & Core Principles

The objective is to replace the fragile, ad-hoc, file-hardcoded media player with a robust, native, source-integrated media engine powered by `libmpv`. 

### Key Principles:
1. **Engine vs Application Separation:**  
   `libmpv` is strictly the low-level decoding and demuxing engine. ATOMS owns the application, UI presentation (`BOSurface v2.5` / `BWE` / `BCM`), filesystem integration (`BOFS` / `VFS` / `USB MSC`), audio output (`Audio HAL` / `SYS_AUDIO_CALL`), device abstraction, and telemetry.
2. **One Engine for Songs & Videos:**  
   A single unified engine handles both audio-only streams (MP3, WAV, FLAC, AAC, Opus, Vorbis) and full audiovisual containers (MP4, MKV, AVI, MOV, WebM, TS).
3. **No Host OS / Compatibility Layers:**  
   Zero reliance on POSIX runtimes, Linux userspace, Windows Win32 APIs, Wine, or `mpv.exe` processes. All interactions take place within the native ATOMS userspace process via the official C client and embedding APIs.
4. **License Integrity:**  
   Strict LGPLv2.1+ build (`-Dgpl=false`, LGPL FFmpeg, zero GPL plugins or non-free SDKs).

---

## 2. End-to-End Media Pipeline

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                      ATOMS MEDIA PLAYER / MEDIA CENTER UI                       │
│      userspace/apps/media_player/main.cpp (BOSurface v2.5 / BWE / BCM)          │
│   [Now Playing] [Song/Video Viewport] [Seek Bar] [Volume] [Track/Sub Controls]   │
└────────────────────────────────────────┬────────────────────────────────────────┘
                                         │
                                         ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           BOS MEDIA API (libbos_media)                          │
│   userspace/libbos_media/include/bos_media.h (BOSMediaPlayer C/C++ Interface)   │
│   - Session lifecycle: create(), open(), play(), pause(), seek(), destroy()     │
│   - Stream routing, track enumeration, metadata parsing, telemetry reporting   │
└────────────────────────────────────────┬────────────────────────────────────────┘
                                         │
                                         ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                      BOS LIBMPV ADAPTER (libbos_media/mpv)                      │
│   - mpv_instance.cpp: mpv_create(), mpv_initialize(), property observers       │
│   - mpv_uri.cpp: mpv_stream_cb_add_ro() bridging to ATOMS VFS syscalls         │
│   - mpv_video_output.cpp: mpv_render_context (SW blit / HW surface bridge)      │
│   - mpv_audio_output.cpp: audio buffer / packet extraction to BOS Audio HAL    │
│   - mpv_events.cpp: event loop handling property changes, EOF, errors          │
└──────────────────┬──────────────────────────────────────────┬───────────────────┘
                   │                                          │
       VFS Stream Callback                        Decoded Audio/Video Data
                   ▼                                          ▼
┌──────────────────────────────────────┐  ┌───────────────────────────────────────┐
│          ATOMS VFS BRIDGE            │  │          OUTPUT INTEGRATION           │
│  SYS_OPEN, SYS_READ, SYS_SEEK        │  │                                       │
│  - atoms://storage/USB0/...          │  │  Video Output:                        │
│  - /volumes/usb0/TEST.MP4            │  │    mpv_render_context_render()        │
│  - /TEST.MP4 (PXE / RAMDISK)         │  │    -> Direct 32bpp ARGB blit          │
│  - Real USB MSC / SCSI / FAT32       │  │    -> bos::Surface / VideoCanvas      │
└──────────────────────────────────────┘  │    -> BCM Linear Framebuffer Scanout │
                                          │                                       │
                                          │  Audio Output:                        │
                                          │    48kHz S16_LE Stereo PCM            │
                                          │    -> SYS_AUDIO_CALL (Stream Write)   │
                                          │    -> Audio HAL (Intel HDA DMA)       │
                                          └───────────────────────────────────────┘
```

---

## 3. Component Details

### 3.1 BOS Media API (`libbos_media`)
The stable, public ATOMS media interface isolating applications from decoder specifics:

```c
typedef struct BOSMediaPlayer BOSMediaPlayer;

typedef enum {
    BOS_MEDIA_STATE_IDLE,
    BOS_MEDIA_STATE_OPENING,
    BOS_MEDIA_STATE_BUFFERING,
    BOS_MEDIA_STATE_PLAYING,
    BOS_MEDIA_STATE_PAUSED,
    BOS_MEDIA_STATE_STOPPED,
    BOS_MEDIA_STATE_ERROR
} BOSMediaState;

typedef struct {
    char title[128];
    char artist[128];
    char album[128];
    char container[32];
    char video_codec[32];
    char audio_codec[32];
    int64_t duration_ms;
    int32_t video_width;
    int32_t video_height;
    double  video_fps;
    int32_t audio_sample_rate;
    int32_t audio_channels;
} BOSMediaMetadata;

typedef struct {
    int64_t audio_pts_ms;
    int64_t video_pts_ms;
    int64_t drift_ms;
    uint32_t decoded_frames;
    uint32_t presented_frames;
    uint32_t dropped_frames;
    float    cpu_percent;
    const char* first_failure_stage;
    const char* first_failure_reason;
} BOSMediaTelemetry;

BOSMediaPlayer* bos_media_create(void);
int bos_media_open(BOSMediaPlayer* player, const char* uri);
int bos_media_play(BOSMediaPlayer* player);
int bos_media_pause(BOSMediaPlayer* player);
int bos_media_stop(BOSMediaPlayer* player);
int bos_media_seek(BOSMediaPlayer* player, int64_t position_ms);
int bos_media_set_volume(BOSMediaPlayer* player, float volume);
int bos_media_set_video_surface(BOSMediaPlayer* player, void* surface_handle);
BOSMediaState bos_media_get_state(BOSMediaPlayer* player);
int bos_media_get_metadata(BOSMediaPlayer* player, BOSMediaMetadata* out_meta);
int bos_media_get_telemetry(BOSMediaPlayer* player, BOSMediaTelemetry* out_telemetry);
void bos_media_destroy(BOSMediaPlayer* player);
```

### 3.2 Filesystem & URI Bridge (`mpv_stream_cb_add_ro`)
`libmpv` is decoupled from OS filesystem routines via `mpv_stream_cb_add_ro()`:
- Custom protocols `atoms://`, `bofs://`, `fat32://`, and file paths `/...` are registered.
- Callbacks invoke ATOMS VFS syscalls:
  - `open_fn(cookie, uri, info)`: Resolves path and calls `__atoms_syscall2(SYS_OPEN, path, flags)`.
  - `read_fn(cookie, buf, size)`: Invokes `__atoms_syscall3(SYS_READ, fd, buf, size)`.
  - `seek_fn(cookie, offset)`: Invokes `__atoms_syscall3(SYS_SEEK, fd, offset, SEEK_SET)`.
  - `size_fn(cookie)`: Computes file size via seek to end (`SEEK_END`) and restores position.
  - `close_fn(cookie)`: Invokes `__atoms_syscall1(SYS_CLOSE, fd)`.
- Guarantees zero POSIX `fopen()`/`open()` assumptions inside `libmpv`.

### 3.3 Video Presentation Integration
1. **Render Context Setup:**  
   Uses `mpv_render_context_create()` with `MPV_RENDER_API_TYPE_SW`.
2. **Zero-Copy / Low-Copy Target:**  
   `mpv_render_param` passes the pointer to the window widget surface:
   - Format: `MPV_RENDER_PARAM_SW_FORMAT` = `"bgr0"` or `"rgb0"` (32bpp linear ARGB/XRGB).
   - Stride: Matches `bos::Surface::stride_bytes()`.
   - Dimensions: Scaled to fit letterboxed/pillarboxed viewport preserving aspect ratio.
3. **Pacing & Invalidation:**  
   `mpv_render_context_set_update_callback()` notifies ATOMS when a new frame is ready, triggering `window.invalidate()` for the BWE compositor.

### 3.4 Audio Presentation Integration
1. Decoded audio is standardized to 48,000 Hz, 16-bit Signed Little-Endian Stereo PCM.
2. `mpv_audio_output` feeds PCM chunks directly into `SYS_AUDIO_CALL`:
   - `ATOMS_AUDIO_OP_STREAM_CREATE`
   - `ATOMS_AUDIO_OP_STREAM_SET_FORMAT` (48kHz, S16_LE, 2ch)
   - `ATOMS_AUDIO_OP_STREAM_WRITE` (`AudioPcmPacket` buffer submitted to Intel HDA DMA ring).
3. Provides glitch-free hardware audio output matching the video presentation clock.

### 3.5 Universal Video Acceleration HAL Negotiation
1. During initialization, probe PCI Display controllers (Vendor `0x10DE` for NVIDIA RTX 4060, `0x8086` for Intel, `0x1002` for AMD).
2. If native hardware acceleration driver is initialized and functional, enable HW decode (`hwdec=auto`).
3. If HW decode fails or is not yet initialized for RTX 4060, automatically negotiate software decoding (`hwdec=no`) using FFmpeg's optimized C/assembly decoders.
4. **Zero black screens:** Fallback is immediate, seamless, and transparent to the user.
