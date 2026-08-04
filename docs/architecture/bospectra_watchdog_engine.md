# BOSPECTRA V3 — Production Multimedia Watchdog & Self-Healing Engine Architecture

## Architectural Overview

BOSPECTRA V3 Phase 11 introduces a production-grade **Multimedia Watchdog and Self-Healing Engine** (`kernel/media/bospectra/watchdog/`). Acting as the highest authority for runtime monitoring, it eliminates infinite black screens, hidden decoder deadlocks, frozen presentation clocks, and silent queue stalls through continuous tick evaluation and a 6-level escalation recovery model.

```
┌────────────────────────────────────────────────────────────────────────┐
│                   BOSPECTRA WATCHDOG ENGINE                            │
│                                                                        │
│   ┌─────────────────────┐   ┌─────────────────────┐   ┌──────────────┐ │
│   │   Watchdog Rules    │   │  Stage Monitor      │   │ History Log  │ │
│   │  (Deadlock/Black    │   │ (Disk -> Display    │   │ (Ring Buffer)│ │
│   │   Screen Detection) │   │   Progress Audit)   │   │              │ │
│   └──────────┬──────────┘   └──────────┬──────────┘   └───────┬──────┘ │
└──────────────┼─────────────────────────┼──────────────────────┼────────┘
               │                         │                      │
               ▼                         ▼                      ▼
┌────────────────────────────────────────────────────────────────────────┐
│                      RECOVERY ESCALATION ENGINE                        │
│                                                                        │
│  Level 1: Retry Frame/Packet                                           │
│  Level 2: Flush Queues (Packet/Decoder/Frame)                          │
│  Level 3: Reset Subsystem (Decoder/Renderer/Color)                     │
│  Level 4: Restart Session                                              │
│  Level 5: Notify Application                                           │
│  Level 6: Kernel Panic (ONLY for memory/ownership/ref corruption)       │
└────────────────────────────────────────────────────────────────────────┘
```

---

## Subsystem Components

### 1. Watchdog Engine (`watchdog_engine.h / .c`)
* Core watchdog service driving inspection ticks across active playback sessions.

### 2. Watchdog Rules (`watchdog_rules.h / .c`)
* Evaluates rules for:
  - `WATCHDOG_DECODER_TIMEOUT`: 0 frames decoded in 200 consecutive ticks.
  - `WATCHDOG_RENDER_TIMEOUT`: 0 frames presented while decoded frames are queued.
  - `WATCHDOG_PACKET_STALL`: Packet queue full for 100 consecutive cycles.
  - `WATCHDOG_FRAME_STALL`: Frame queue full for 100 consecutive cycles.
  - `BLACK_SCREEN_DETECTED`: Framebuffer hash unchanged for 120 frames while presentation is active.

### 3. Watchdog Monitor (`watchdog_monitor.h / .c`)
* Audits stage-by-stage progression (`Disk` -> `Packet` -> `Decode` -> `Frame` -> `Color` -> `Texture` -> `Surface` -> `BWE` -> `Display`).

### 4. Watchdog Recovery (`watchdog_recovery.h / .c`)
* Executes 6-level escalation policies and resets failing subsystems.

### 5. Watchdog History (`watchdog_history.h / .c`)
* Ring-buffer log tracking failure events, recovery levels, and resolution status.

### 6. Watchdog Metrics (`watchdog_metrics.h / .c`)
* Watchdog metrics dumper rendering live watchdog health and recovery status.

### 7. Watchdog Console (`watchdog_console.h / .c`)
* Watchdog CLI dispatcher handling `watchdog`, `metrics`, `history`, `recover`, `pipeline` commands.

---

## Escalation Policy

1. **Level 1 (Retry)**: Re-evaluate frame/packet readiness.
2. **Level 2 (Flush)**: Flush packet/decoder/frame queues.
3. **Level 3 (Reset Subsystem)**: Re-initialize decoder, color engine, or renderer context.
4. **Level 4 (Restart Session)**: Re-create playback session and demuxer.
5. **Level 5 (Notify Application)**: Raise application-visible alert.
6. **Level 6 (Kernel Panic)**: Triggered ONLY on unrecoverable heap/ownership/reference corruption (never for recoverable playback stalls).
