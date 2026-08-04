# BOSPECTRA V3 — Production Multimedia Manager Architecture

## Architectural Overview

BOSPECTRA V3 introduces a decoupled, multi-tiered **Multimedia Manager Subsystem** (`kernel/media/bospectra/manager/`) positioned directly between upper application layers (such as `bos_media_player`) and low-level multimedia processing engines (parsers, decoders, frame memory, color conversion, software renderer).

```
Application Layer (BOS Media Player)
   │
   ▼
Media Manager (Pipeline Orchestrator & Diagnostics Inspector)
   │
   ├── Session Manager      (Session Handles, State Machine, Cleanup)
   ├── Container Manager    (Driver Registry & Dynamic Container Probing: AVI, MP4, MKV)
   ├── Decoder Manager      (Codec Registry & Metadata Routing: MJPEG, H.264, MPEG2)
   ├── Render Manager       (Surface Allocation, Texture Staging, Surface Presentation)
   ├── Sync Manager         (Presentation Master Clock, PTS Pacing, Speed Control)
   └── Resource Manager     (Allocation Tracking, Ownership, Leak Detection)
   │
   ▼
BOSPECTRA Native Engines (Parsers, Decoders, Frame Memory Pool, Color Engine, Renderer)
```

---

## Manager Subsystem Responsibilities

### 1. Media Manager (`media_manager.h / .c`)
* **Role**: Pipeline Orchestration & Diagnostics.
* **Non-Responsibilities**: Never decodes packets, parses container bytes, or draws pixels directly.
* **Features**:
  * `bospectra_media_open()`: Initiates media session opening.
  * `bospectra_media_inspect_pipeline()`: Runs the Pipeline Inspector telemetry check across all 9 pipeline stages (`Media Manager`, `Session`, `Container`, `Decoder`, `Frame Pool`, `Color Engine`, `Renderer`, `BWE`, `Display`).

### 2. Session Manager (`session_manager.h / .c`)
* **Role**: Session Lifecycle & State Machine.
* **Responsibilities**:
  * Assigns unique session handles (`bospectra_playback_session_id_t`).
  * Validates state machine transitions (`IDLE` → `OPENING` → `READY` → `PLAYING` / `PAUSED` → `STOPPED`).

### 3. Container Manager (`container_manager.h / .c`)
* **Role**: Container Registry & Automatic Probing.
* **Responsibilities**:
  * Auto-registers built-in OS container drivers (`AVI`, `MP4`, `MKV`) during system boot.
  * Probes file headers (`RIFF`, `ftyp`, `EBML`) to dynamically select the container driver without hardcoded fallback logic.

### 4. Decoder Manager (`decoder_manager.h / .c`)
* **Role**: Codec Registry & Dynamic Routing.
* **Responsibilities**:
  * Auto-registers built-in decoders (`MJPEG`, `H.264`, `MPEG2`).
  * Matches stream descriptors (`stream_desc.codec_name` / `codec_id`) to appropriate decoder driver vtables.

### 5. Render Manager (`render_manager.h / .c`)
* **Role**: Rendering Target & Surface Management.
* **Responsibilities**:
  * Manages software/OpenGL surface contexts and texture staging allocations.
  * Presents converted ARGB32 frames to BWE compositor canvas windows.

### 6. Sync Manager (`sync_manager.h / .c`)
* **Role**: Synchronization & Timing.
* **Responsibilities**:
  * Maintains master presentation clock.
  * Controls playback speed scaling and PTS frame pacing.

### 7. Resource Manager (`resource_manager.h / .c`)
* **Role**: Resource Ownership & Leak Telemetry.
* **Responsibilities**:
  * Tracks packet, frame, texture, surface, and context allocations.
  * Reference counts active memory objects and detects leaks during shutdown.

---

## Operational Data Flow

1. Application calls `bospectra_media_open("/Media/test.mp4", &session_id)`.
2. `MediaManager` requests `SessionManager` to allocate session.
3. `ContainerManager` probes magic bytes of file and selects `MP4` driver.
4. `DecoderManager` inspects video stream metadata and selects `H.264` decoder driver.
5. `RenderManager` allocates software surface for canvas target.
6. `MediaManager` runs `bospectra_media_inspect_pipeline()` and logs status (`PASS` / `FAIL`).
