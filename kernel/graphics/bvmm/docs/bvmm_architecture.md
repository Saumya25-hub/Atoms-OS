# BOS VRAM Memory Manager (BVMM) — Architectural Specification

## Phase 1 — Core Foundation Layout

The **BOS VRAM Memory Manager (BVMM)** provides production-grade video memory management, scatter-gather aperture mapping, memory pooling, cross-process handle isolation, timeline fence tracking, and out-of-VRAM eviction.

### Directory Structure

```
kernel/graphics/bvmm/
├── include/
│   ├── bvmm_types.h        # Production data structure definitions & handle encoding
│   ├── bvmm_api.h          # Public C entrypoint declarations
│   └── bvmm.h              # Subsystem master include header
├── core/
│   ├── bvmm_init.c         # Subsystem lifecycle manager & telemetry stats
│   └── bvmm_core.c         # Core API entrypoint dispatcher & stub handles
├── heap/                   # Phase 2: TLSF & DRM-Style Range Allocator
├── pools/                  # Phase 3: Classified Memory Pools (Small, Medium, Large, Huge)
├── handles/                # Phase 5 & 7: Process Handle Table & Cross-Process Sharing
├── fences/                 # Phase 6: Asynchronous Timeline Fences
├── diagnostics/            # Phase 8 & 14: Telemetry & Forensic Diagnostics
├── vendor/                 # Phase 11: Vendor Drivers HAL Vtable Hooks
├── tests/                  # Certification & Unit Tests
└── docs/
    └── bvmm_architecture.md
```

### Data Structure Layout (Phase 1C)

- **Handle Format (`bvmm_handle_t`)**: 64-bit integer encoding:
  - `Bits [63..48]`: 16-bit Generation ID (Anti-Double Free Protection)
  - `Bits [47..32]`: 16-bit Flags & Domain Classification
  - `Bits [31..00]`: 32-bit Handle Index Table Entry
- **Memory Domains (`bvmm_domain_t`)**: `VRAM`, `GTT`, `SYSTEM`, `UPLOAD`, `READBACK`.
- **Memory Pools (`bvmm_pool_type_t`)**: `Small`, `Medium`, `Large`, `Huge`, `Transient`, `Upload`, `Readback`.
