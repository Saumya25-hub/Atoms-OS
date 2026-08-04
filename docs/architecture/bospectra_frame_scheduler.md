# BOSPECTRA V3 — Production Frame Scheduler & Master Presentation Clock Architecture

## Architectural Overview

BOSPECTRA V3 Phase 4 introduces a **Master Clock & Production Frame Scheduler Subsystem** (`kernel/media/bospectra/scheduler/`). It decouples presentation rendering from compositor timer ticks (`BOHeart`), ensuring that frames are presented strictly when their presentation timestamp (`PTS`) coincides with the Master Media Clock time.

```
BOHeart Ticker Interrupt
   │
   ▼
Master Clock Update (33ms step @ 30 FPS pacing)
   │
   ▼
PTS Monotonic Validation
   │
   ▼
Timeline Position Authority
   │
   ▼
Frame Scheduler Evaluation
   ├── TOO_EARLY      ──► Hold frame in decoded queue
   ├── LATE_DROP      ──► Drop frame to catch up with Master Clock
   └── PRESENT_NOW    ──► Authorize Display Scheduler
                              │
                              ▼
                       Display Scheduler (Exclusive Presentation Call)
                              │
                              ▼
                       Software / OpenGL Renderer Presentation
```

---

## Subsystem Components

### 1. Master Media Clock (`scheduler_clock.h / .c`)
* Central timekeeping authority supporting `start`, `pause`, `resume`, `stop`, `reset`, `seek`, `speed scaling` (0.5x, 1.0x, 2.0x), and clock `drift` compensation.
* Exposes `bospectra_master_clock_get_media_time()`.

### 2. PTS Manager (`pts_manager.h / .c`)
* Monotonic timestamp validator (`bospectra_pts_validate`).
* Detects monotonic order violations, missing PTS entries, and duplicate timestamps across decoded frames.

### 3. Timeline Engine (`timeline_engine.h / .c`)
* Maintains current playback position, next presentation PTS, stream duration, EOS status, and looping flags.

### 4. Frame Scheduler (`frame_scheduler.h / .c`)
* Evaluates incoming decoded frame timestamps against current Master Clock position:
  * `FRAME_ACTION_TOO_EARLY`: Frame PTS > Master Clock + 15ms. Frame is retained in the queue.
  * `FRAME_ACTION_LATE_DROP`: Frame PTS < Master Clock - 40ms. Frame is dropped immediately to avoid cumulative delay.
  * `FRAME_ACTION_PRESENT_NOW`: Frame PTS aligns within presentation window. Authorized for immediate presentation.
  * `FRAME_ACTION_DUPLICATE`: Repeated frame presentation for low FPS source alignment.

### 5. Frame Pacer (`frame_pacer.h / .c`)
* Microsecond interval calculator supporting 24, 25, 29.97, 30, 50, 59.94, 60, and 120 FPS.
* Measures presentation jitter and applies catch-up/slow-down corrections.

### 6. Display Scheduler (`display_scheduler.h / .c`)
* Exclusive module authorized to invoke renderer presentation routines (`BOSPECTRA_Render_PresentFrame()`).

### 7. Scheduler Metrics (`scheduler_metrics.h / .c`)
* Telemetry printer producing diagnostic summaries during pipeline inspection:

```
============== FRAME SCHEDULER ==============
Master Clock         : 1200000 US
Playback Position    : 1200000 US

Frame Metrics:
  Presented Frames   : 36
  Dropped Frames     : 0
  Late Frames        : 0
  Duplicate Frames   : 0

Pacing & Clock:
  Target FPS         : 30
=============================================
```

---

## Memory & Performance Guarantees

* **Zero Per-Frame Allocation**: No `malloc` or heap churn occurs during PTS evaluation, frame pacing, or display scheduling.
* **Deterministic Jitter Control**: Microsecond-precision pacer tracking prevents micro-stuttering across variable screen refresh rates.
