# BOSPECTRA V3 — Resource Ownership & Lifetime Management Architecture

## Architectural Overview

BOSPECTRA V3 Phase 5 & Phase 6 introduces an integrated **Resource Ownership, Reference Counting, and Lifetime Tracking Framework** (`kernel/media/bospectra/resource/`). It eliminates unowned objects, memory leaks, double frees, dangling pointers, and stale surfaces by enforcing single ownership, atomic reference counting, and end-to-end lifetime validation.

```
Application Context
   │
   ▼
Media Manager
   │
   ▼
Playback Session (Single Owner Authority)
   │
   ├── Container Context
   ├── Packet Queue (Tracks Packet Acquisitions/Transfers)
   ├── Decoder Context
   ├── Frame Queue (Tracks Decoded Frame Pool References)
   ├── Color Engine (ARGB Frame Conversion)
   ├── Renderer Queue
   ├── Texture / Surface Management
   └── Display Scheduler (BWE Presentation Surface)
```

---

## Subsystem Components

### 1. Ownership Manager (`ownership_manager.h / .c`)
* Enforces explicit single ownership for every multimedia resource.
* Maintains ownership table entries (`resource_id`, `owner_type`, `owner_id`, `creator_id`, `generation_id`).
* Supported owner types: `OWNER_TYPE_MEDIA_MANAGER`, `OWNER_TYPE_PLAYBACK_SESSION`, `OWNER_TYPE_CONTAINER`, `OWNER_TYPE_PACKET_QUEUE`, `OWNER_TYPE_DECODER`, `OWNER_TYPE_FRAME_QUEUE`, `OWNER_TYPE_COLOR_ENGINE`, `OWNER_TYPE_RENDERER_QUEUE`, `OWNER_TYPE_DISPLAY_SCHEDULER`, `OWNER_TYPE_COMPOSITOR_BWE`.

### 2. Reference Manager (`reference_manager.h / .c`)
* Manages atomic reference control blocks (`AddRef`, `Release`, `Acquire`, `Unref`, `Destroy`).
* Detects double-release attempts and unreferenced resource access.

### 3. Lifetime Tracker (`lifetime_tracker.h / .c`)
* Tracks resource states (`LIFETIME_STATE_ALLOCATED`, `LIFETIME_STATE_INITIALIZED`, `LIFETIME_STATE_READY`, `LIFETIME_STATE_ACTIVE`, `LIFETIME_STATE_QUEUED`, `LIFETIME_STATE_PRESENTED`, `LIFETIME_STATE_RELEASED`, `LIFETIME_STATE_DESTROYED`).
* Validates state machine transitions.

### 4. Resource Graph (`resource_graph.h / .c`)
* Maintains runtime hierarchical node links connecting parent owners to child resources.

### 5. Resource Validator (`resource_validator.h / .c`)
* Continuous safety engine checking for orphan objects, double ownership, dangling pointers, and invalid destroy orders.

### 6. Leak Detector (`leak_detector.h / .c`)
* Tracks total allocations, active live objects, releases, peak memory footprint, and pinpoints leaks upon session teardown.

### 7. Resource Metrics (`resource_metrics.h / .c`)
* Diagnostics engine producing resource summaries during pipeline inspection:

```
============= RESOURCE MANAGER =============
Packets / Frames Alive: 0
Peak Objects Alive    : 0

Reference Count Errors: 0
Ownership Violations  : 0
Memory Leaks          : 0
============================================
```

---

## Transfer & Lifetime Rules

1. **Explicit Acquisition & Release**: Resources must never be passed without explicitly calling `Acquire()` / `Release()`.
2. **Deterministic Cleanup**: On playback session destroy, all child resources (packets, frames, textures, surfaces) are released in strict reverse-creation order.
3. **Zero Heap Allocation**: Control structures in `resource/` are pre-allocated statically in kernel memory.
