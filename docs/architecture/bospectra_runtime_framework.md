# BOSPECTRA V3 — Production Telemetry & Error Propagation Framework

## Architectural Overview

BOSPECTRA V3 Phase 9 & Phase 10 introduces a comprehensive **Telemetry, Performance, Health, and Structured Error Propagation Subsystem** (`kernel/media/bospectra/runtime/`). It renders the BOSPECTRA multimedia kernel observable, fault-aware, and self-recovering.

```
       BOSPECTRA Subsystems (Decoder, Renderer, Scheduler, Container, etc.)
                                    │
                  ┌─────────────────┴─────────────────┐
                  ▼                                   ▼
        Telemetry Engine & PM                     Error Manager
      (Live Metrics & FPS Tracking)           (Structured Error Objects)
                  │                                   │
                  ▼                                   ▼
        Runtime Health Engine                 Error Dispatcher
     (OK / WARNING / DEGRADED / FAIL)      (Routing, Logging & Recovery)
                  │                                   │
                  └─────────────────┬─────────────────┘
                                    ▼
                         Runtime Metrics & Console
```

---

## Subsystem Components

### 1. Telemetry Engine (`telemetry_engine.h / .c`)
* Real-time metrics collector tracking packets read/queued/dropped/decoded, frames decoded/presented/dropped/skipped, PTS/clock drift, decode/render/present durations, queue & pool usages.

### 2. Performance Monitor (`performance_monitor.h / .c`)
* Microsecond FPS calculator (current, average, target), frame time delta, worst/best frame variance, jitter, and pipeline latency across 24, 25, 29.97, 30, 50, 59.94, 60, and 120 FPS.

### 3. Statistics Manager (`statistics_manager.h / .c`)
* Lifetime counter manager recording session counts, total frames decoded/rendered, bytes read, per-decoder, per-renderer, and per-codec statistics.

### 4. Error Manager (`error_manager.h / .c`)
* Defines structured `BOSPECTRA_Error` objects containing error code, subsystem, severity (`INFO` to `FATAL`), category (`MEMORY` to `INTERNAL`), message, file/function/line, session ID, timestamp, recoverability flag, and suggested action string.

### 5. Error Dispatcher (`error_dispatcher.h / .c`)
* Central error router, logger, telemetry updater, health state updater, error history recorder, and automatic recovery policy execution engine.

### 6. Error Reporter (`error_reporter.h / .c`)
* Formatted human-readable error console report renderer.

### 7. Runtime Health (`runtime_health.h / .c`)
* Subsystem and global health evaluator computing status (`HEALTH_STATE_OK`, `HEALTH_STATE_WARNING`, `HEALTH_STATE_DEGRADED`, `HEALTH_STATE_FAILED`).

### 8. Runtime Metrics (`runtime_metrics.h / .c`)
* Comprehensive telemetry and health status dumper for diagnostic inspection.

---

## Runtime Telemetry Sample Output

```
================ BOSPECTRA TELEMETRY ================
Playback FPS     : 30
Target FPS       : 30

Frames Decoded   : 1420
Frames Presented : 1420
Frames Dropped   : 0

Pipeline Health  : HEALTH_OK (100%)
=====================================================
```
