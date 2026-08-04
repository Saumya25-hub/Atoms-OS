# BOSPECTRA V3 — Production Packet Queue & Pipeline Scheduling Engine Architecture

## Architectural Overview

BOSPECTRA V3 Phase 3 introduces a **Production Staged Multimedia Pipeline Subsystem** (`kernel/media/bospectra/pipeline/`). It replaces legacy single-tick synchronous execution (`Read -> Decode -> Render`) with isolated, queue-driven pipeline stages.

```
Disk Storage
   │
   ▼
Demux Engine
   │
   ▼
Packet Queue        (Lock-free SPSC ring buffer for demuxed packets)
   │
   ▼
Decode Queue        (Decoder scheduling & backpressure manager)
   │
   ▼
Decoder Engine
   │
   ▼
Frame Queue         (Decoded YUV/RGB frames managed via FramePool)
   │
   ▼
Renderer Queue      (Presentation pacing & dropped frame counter)
   │
   ▼
Color Engine → BWE Surface Compositor → Display Framebuffer
```

---

## Subsystem Components

### 1. Packet Queue (`packet_queue.h / .c`)
* Lock-free single-producer / single-consumer ring buffer storing `BOSPacket*` pointers.
* Tracks payload size, stream index, PTS/DTS timestamps, duration, EOS flags.
* Implements overflow and underflow telemetry counters.

### 2. Decode Queue (`decode_queue.h / .c`)
* Feeds packets into codec drivers.
* Manages decode ordering, codec backpressure, and decode failure tracking.

### 3. Frame Queue (`frame_queue.h / .c`)
* Ring buffer queue storing decoded `BOSFrame*` pointers allocated from `FramePool`.
* Enforces zero per-frame heap allocations.
* Manages acquire, release, recycle, and timestamp ordering operations.

### 4. Renderer Queue (`renderer_queue.h / .c`)
* Frame presentation queue.
* Manages presentation ordering, vsync pacing readiness, and dropped frame accounting.

### 5. Pipeline Scheduler (`scheduler.h / .c`)
* Central multimedia scheduler enforcing stage isolation (`SCHEDULER_STAGE_DEMUX`, `SCHEDULER_STAGE_DECODE`, `SCHEDULER_STAGE_RENDER`, `SCHEDULER_STAGE_PRESENT`).
* Dispatches work requests per stage cycle.

### 6. Pipeline Engine (`pipeline_engine.h / .c`)
* Pipeline execution brain managing pipeline contexts (`BOSPECTRA_PipelineContext`) and state machine transitions:
  * `PIPELINE_STATE_STOPPED`
  * `PIPELINE_STATE_RUNNING`
  * `PIPELINE_STATE_PAUSED`
  * `PIPELINE_STATE_DRAINING`
* Controls operations: `start`, `pause`, `resume`, `stop`, `flush`, `drain`, `recover`.

### 7. Queue Metrics Diagnostics (`queue_metrics.h / .c`)
* Telemetry printer producing diagnostic summaries during pipeline inspection:

```
============= PIPELINE METRICS =============
Pipeline State       : RUNNING

Packet Queue:
  Depth              : 0
  Overflows          : 0
  Underflows         : 0

Frame Queue:
  Depth              : 0

Renderer Queue:
  Submitted          : 120
  Presented          : 120
  Dropped            : 0
============================================
```

---

## Multi-threading Readiness

While Phase 3 operates within the kernel thread execution loop, every queue component is designed with lock-free SPSC primitives. The architecture natively supports multi-threaded worker separation:
* **Demux Thread**: Reads container streams and fills `PacketQueue`.
* **Decode Thread**: Pops from `DecodeQueue`, decodes via codec drivers, and pushes to `FrameQueue`.
* **Render Thread**: Pops from `RendererQueue` and presents frames to display surfaces.
