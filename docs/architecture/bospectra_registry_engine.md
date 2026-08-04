# BOSPECTRA V3 — Dynamic Multimedia Registry Engine Architecture

## Architectural Overview

The **Dynamic Multimedia Registry Subsystem** (`kernel/media/bospectra/registry/`) serves as the central brain and single source of truth for the BOSPECTRA V3 framework. It eliminates all static routing assumptions, hardcoded driver links, and file extension checks by providing dynamic runtime registration, scoring-based probing, and capability matching.

```
Application Layer
   │
   ▼
Media Manager / Probe Engine
   │
   ├── Container Registry   (Dynamic format registration: AVI, MP4, MKV, WebM...)
   ├── Codec Registry       (Dynamic video/audio codec resolution: MJPEG, H.264, MPEG2...)
   ├── Renderer Registry    (Capability-based backend selection: Software, OpenGL, Vulkan...)
   └── Driver Registry      (Master central coordinator, duplicate guard, validation rules)
```

---

## Subsystem Components

### 1. `bospectra_registry.h / .c`
* Top-level entry point for initializing, shutting down, and inspecting the registry subsystem.
* Exposes `bospectra_v3_registry_init()`, `bospectra_v3_registry_shutdown()`, and `bospectra_v3_registry_dump_diagnostics()`.

### 2. `container_registry.h / .c`
* Maintains a dynamic array of registered container drivers (`BOSPECTRA_ContainerDriverRecord`).
* Enforces validation rules:
  * Rejects NULL callbacks (`probe`, `open`).
  * Rejects duplicate format names.
  * Rejects registration attempts exceeding maximum buffer capacity (`BOSPECTRA_MAX_CONTAINER_DRIVERS`).

### 3. `codec_registry.h / .c`
* Manages video and audio codec records (`BOSPECTRA_CodecDriverRecord`).
* Resolves decoders by codec name or numeric ID (`BOSPECTRA_CODEC_MJPEG`, `BOSPECTRA_CODEC_H264`, `BOSPECTRA_CODEC_MPEG2`) without hardcoded string branching in playback sessions.

### 4. `renderer_registry.h / .c`
* Manages software and hardware-accelerated rendering backends (`Software`, `OpenGL`, `Vulkan`, `HW Video`).
* Resolves the best available renderer based on requested capabilities and backend priorities.

### 5. `probe_engine.h / .c`
* Implements dynamic header byte inspection.
* Operates by reading the initial 512 bytes of a media stream, invoking the `probe()` callback of every registered container driver, evaluating confidence scores (0–100), and selecting the highest-scoring driver.

### 6. `driver_registry.h / .c`
* Master registry coordinator.
* Outputs the unified registry diagnostic summary during pipeline inspection:

```
============= BOSPECTRA REGISTRY =============

Containers Registered:
  - AVI (Ext: avi)
  - MP4 (Ext: mp4,m4v,mov)
  - MKV (Ext: mkv,webm)

Codecs Registered:
  - MJPEG
  - H264
  - MPEG2

Renderers Registered:
  - Software
  - OpenGL
==============================================
```

---

## Scalability & Extensibility

Adding a new media format (e.g. `AV1`, `HEVC`, `WebM`, `FLAC`) requires **zero modifications** to existing core framework files (`media_manager.c`, `playback_session.c`, `player_core.c`, `render_manager.c`). 

Developers only need to:
1. Implement the driver vtable.
2. Register the driver in `driver_registry` during boot.
