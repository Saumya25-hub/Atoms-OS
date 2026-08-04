# BOSPECTRA V3 — Production Multimedia Diagnostics & Pipeline Validation Framework

## Architectural Overview

BOSPECTRA V3 Phase 7 & Phase 8 introduces a self-diagnosing **Diagnostics and Pipeline Validation Subsystem** (`kernel/media/bospectra/diagnostics/`). It provides runtime memory debugging, ownership tree dumping, queue inspection, persistent event tracing, 28+ subsystem pipeline health validation, and a diagnostic console interface.

```
Diagnostic Console / CLI Interface
   │
   ├── bospectra diag         ──► Media Debugger Inspection
   ├── bospectra validate     ──► 28+ Subsystem Pipeline Health Validator
   ├── bospectra leaks        ──► Memory Leak Auditor
   ├── bospectra ownership    ──► Ownership Tree Dump
   ├── bospectra queues       ──► Queue Metrics & Overflow/Underflow Dump
   ├── bospectra sessions     ──► Active Session Status Inspector
   ├── bospectra trace        ──► Persistent Event Trace Log
   ├── bospectra resources    ──► Resource Table Dump
   └── bospectra health       ──► Health Score & Playback Readiness (YES/NO)
```

---

## Subsystem Components

### 1. Media Debugger (`media_debugger.h / .c`)
* Core memory and object inspector tracking active sessions, containers, files, packets, decode jobs, frames, ARGB frames, textures, surfaces, and queues.

### 2. Memory Validator (`memory_validator.h / .c`)
* Leak detection auditor pinpointing unreleased packets, frames, textures, surfaces, canvases, windows, decoders, renderers, queues, and sessions.

### 3. Ownership Dump (`ownership_dump.h / .c`)
* Implements `bospectra_dump_ownership_tree()` displaying tree hierarchy.

### 4. Resource Dump (`resource_dump.h / .c`)
* Implements `bospectra_dump_resources()` rendering object ID, owner, ref count, generation, state, queue, lifetime, and kernel memory address.

### 5. Queue Dump (`queue_dump.h / .c`)
* Inspects Packet Queue, Decode Queue, Frame Queue, and Renderer Queue (size, capacity, head/tail, waiting counts, underflows/overflows, stalled state).

### 6. Session Dump (`session_dump.h / .c`)
* Session status inspector printing state, PTS position, current frame/packet, decoder, renderer, surface, window, canvas, speed, dropped/presented frame counts.

### 7. Trace Engine (`trace_engine.h / .c`)
* Ring-buffer trace log engine recording timestamped subsystem events (`Packet Allocated`, `Packet Queued`, `Packet Decoded`, `Frame Allocated`, `Frame Converted`, `Frame Queued`, `Frame Presented`, `Playback Started/Paused/Stopped`).

### 8. Pipeline Validator (`pipeline_validator.h / .c`)
* Implements `bospectra_pipeline_validate()` auditing 28+ subsystems (`Memory Manager`, `Packet Pool`, `Frame Pool`, `Ref Manager`, `Ownership Manager`, `VFS`, `Container Registry`, `Probe Engine`, `AVI/MP4/MKV Drivers`, `Codec Registry`, `MJPEG/H264/MPEG2 Decoders`, `Color Engine`, `Renderer Registry`, `Software/OpenGL Renderers`, `Queues`, `Scheduler`, `Master Clock`, `Frame Scheduler`, `Display Scheduler`, `BWE Connection`, `Framebuffer`) and producing an OVERALL HEALTH SCORE & Playback Readiness status (`YES / NO`).

### 9. Diagnostic Console (`diagnostic_console.h / .c`)
* Structured diagnostic CLI dispatcher handling `diag`, `validate`, `leaks`, `ownership`, `queues`, `sessions`, `trace`, `resources`, `health` commands.

---

## Validation Sample Output

```
========== BOSPECTRA PIPELINE VALIDATION ==========
Memory Manager     : ✅ PASS
Packet Pool        : ✅ PASS
Frame Pool         : ✅ PASS
Reference Manager  : ✅ PASS
Ownership Manager  : ✅ PASS
VFS Interface      : ✅ PASS
Container Registry : ✅ PASS
Container Probe    : ✅ PASS
AVI Driver         : ✅ PASS
MP4 Driver         : ✅ PASS
MKV Driver         : ✅ PASS
Codec Registry     : ✅ PASS
MJPEG Decoder      : ✅ PASS
H264 Decoder       : ✅ PASS
MPEG2 Decoder      : ✅ PASS
Color Engine       : ✅ PASS
Renderer Registry  : ✅ PASS
Software Renderer  : ✅ PASS
OpenGL Renderer    : ✅ PASS
Packet Queue       : ✅ PASS
Decode Queue       : ✅ PASS
Frame Queue        : ✅ PASS
Renderer Queue     : ✅ PASS
Pipeline Scheduler : ✅ PASS
Master Clock       : ✅ PASS
Frame Scheduler    : ✅ PASS
Display Scheduler  : ✅ PASS
BWE Connection     : ✅ PASS

----------------- HEALTH REPORT -----------------
Subsystems Passed  : 28
Subsystems Failed  : 0
Critical Errors    : 0
PLAYBACK READY     : YES
==================================================
```
