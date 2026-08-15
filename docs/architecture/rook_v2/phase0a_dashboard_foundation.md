# ROOK V2 ARCHITECTURE SPECIFICATION
## PHASE 0A: CERTIFICATION DASHBOARD FOUNDATION SPECIFICATION

```
================================================================================
ATOMS OS — ROOK V2 SCREEN MANAGEMENT & DISPLAY CONTRACT
PHASE 0A DELIVERABLE: CERTIFICATION DASHBOARD FOUNDATION
================================================================================
Standard:       Operating-System-Grade Telemetry & Surface Contract Integration
Target:         Universal Bare-Metal (Intel Haswell H81, AMD iGPU, NVIDIA PCIe, UEFI GOP)
Status:         ARCHITECTURE & DESIGN COMPLETE — READY FOR IMPLEMENTATION
Rule Compliance:Rule 1 (Documentation First), Rule 4 (Single Source Of Truth),
                Rule 5 (Zero Hardcoded Resolutions), Rule 6 (Preserve Stable Systems)
================================================================================
```

---

## 1. Executive Summary & Objective

**Phase 0A** establishes the visual and structural foundation of the **ROOK V2 Certification Dashboard**.

```
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                 ROOK V2 SCREEN LIFECYCLE SEQUENCE                               │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│  [ROOK_PAGE_BOOT_SPLASH]  ──>  [ROOK_PAGE_DASHBOARD]  ──>  [ROOK_PAGE_LOGIN]  ──>  [DESKTOP]    │
│  (Vector Chevron Logo)          (Phase 0A Static Grid)       (User Auth Screen)    (Window Mgr) │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

**Scope of Phase 0A:**
* Implement the static 8-panel visual layout grid using pure software rasterization.
* Register `ROOK_PAGE_DASHBOARD` as a first-class citizen in the ROOK page registry.
* Render all 8 diagnostic panels with initial baseline state (`STATUS: NOT CONNECTED`).
* Guarantee **zero dynamic heap allocations (`kmalloc`)** during rendering.
* Zero modifications to AGDTE, GOP firmware, VMM, HEAP, or Login V2 code.

---

## 2. Visual Architecture & 8-Panel Layout Specification

The dashboard subdivides the display canvas into **8 Proportional Telemetry Panels**:

```
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│ ♜ ATOMS OS — ROOK V2 CERTIFICATION DASHBOARD [PHASE 0A FOUNDATION]      [60.0 FPS | CPU0: HASWELL]│
├────────────────────────────────┬───────────────────────────────┬────────────────────────────────┤
│ 1. PHASE CERTIFICATION STATUS  │ 2. GEOMETRY AUTHORITY         │ 3. SURFACE CONTRACT            │
│  Phase 1: Geometry   [OFFLINE] │  Active Source: [DISCONNECTED]│  Surface Width:  --- px        │
│  Phase 2: Surface    [OFFLINE] │  Resolution:    --- x ---     │  Surface Height: --- px        │
│  Phase 3: Lifecycle  [OFFLINE] │  Active DPI:    ---           │  Surface Stride: --- px        │
│  Phase 4: Presenter  [OFFLINE] │  GOP Pitch:     --- px        │  Address:        0x00000000    │
│  Phase 5: Login V2   [OFFLINE] │  Conflict Check:[NOT SCANNED] │  Invariant:      [UNVERIFIED]  │
│  Phase 6: Wallpaper  [OFFLINE] │  Pitch Leak:    [NOT SCANNED] │  Status:                       │
│  Phase 7: Desktop    [OFFLINE] │                               │   STATUS: NOT CONNECTED        │
│  Phase 8: Hardware   [OFFLINE] │  STATUS: NOT CONNECTED        │                                │
├────────────────────────────────┼───────────────────────────────┼────────────────────────────────┤
│ 4. SCREEN LIFECYCLE MONITOR    │ 5. PRESENTATION ENGINE        │ 6. LIVE FAULT ANALYSIS         │
│  Current Screen: ROOK_DASHBOARD│  Hardware Pitch:  --- px      │  Last Error:       NONE        │
│  Active State:   INITIALIZING  │  Surface Width:   --- px      │  Failing Mod:      NONE        │
│  on_enter():     0 [OFFLINE]   │  Blit Mode:       QWORD DUAL  │  Error Code:       0x00000000  │
│  on_update():    0 [OFFLINE]   │  Present Latency: --- ms      │  Recommended Fix:  NONE        │
│  on_render():    0 [OFFLINE]   │  PCIe Barrier:    sfence OK   │                                │
│  Transitions:    0 [OFFLINE]   │  Frame Count:     0           │  STATUS: NOT CONNECTED         │
│                                │                               │                                │
│  STATUS: NOT CONNECTED         │  STATUS: NOT CONNECTED        │                                │
├────────────────────────────────┴───────────────────────────────┴────────────────────────────────┤
│ 7. FLIGHT RECORDER & ROLLING EVENT TIMELINE (Latest 6 Events of 256 Total)                      │
│  [+0.000000s] [DASHBOARD] Phase 0A Static Layout Grid Initialized                              │
│  [+0.000000s] [DASHBOARD] Memory Contract Asserted: Zero Heap Allocations                      │
│  [+0.000000s] [DASHBOARD] Telemetry Probes: STATUS: NOT CONNECTED                              │
│  [+0.000000s] [DASHBOARD] Waiting for Phase 0B Sensor Connection...                            │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│ 8. PANIC RECORDER FORENSICS BLACK-BOX: NOMINAL (NO PANIC ACTIVE)                                │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Subsystem Architecture (`kernel/shell/rook/debug/`)

```
kernel/shell/rook/debug/
├── dashboard.h    # ROOK V2 Certification Dashboard interface & structures
└── dashboard.c    # 8-panel static layout rasterizer & page callbacks
```

### Technical Invariants:
1. **Zero-Heap Rule:** All panel coordinate calculations and text formatters use the stack and static BSS variables.
2. **Surface Contract Compliance:** Drawing uses dense indexing `fb[py * width + px] = color`. Zero hardware pitch math.
3. **Typography:** Uses standard embedded `g_font8x16_stub` bitmap font.

---

## 4. Protected Core Invariance Guarantee

Under **Rule 6 of Protocol V2.0**, all 11 foundational certified kernel systems remain **100% untouched**:

```
[PROTECTED SUBSYSTEM AUDIT — 0% TOUCH POLICY]
├── 1. CPU Features Engine (Haswell Detection) ─────── [UNTOUCHED 🔒]
├── 2. GDT Engine (Global Descriptor Table) ────────── [UNTOUCHED 🔒]
├── 3. SMP Engine (APIC Multi-Core Discovery & IPIs) ─ [UNTOUCHED 🔒]
├── 4. IDT Engine (Interrupts, ISRs, Exceptions) ───── [UNTOUCHED 🔒]
├── 5. PIC Engine (Legacy 8259A Remap & IRQ0/1) ────── [UNTOUCHED 🔒]
├── 6. PMM Engine (Physical Memory Bitmap) ─────────── [UNTOUCHED 🔒]
├── 7. VMM Engine (PML4 Page Tables & Virtual Memory)  [UNTOUCHED 🔒]
├── 8. Heap Allocator (kmalloc/kfree Stage A & B) ──── [UNTOUCHED 🔒]
├── 9. Scheduler & Multitasking Engine ─────────────── [UNTOUCHED 🔒]
├── 10. AGDTE Surface Plane Compositor ─────────────── [UNTOUCHED 🔒]
└── 11. UEFI Bootloader (BOOTX64.EFI) ──────────────── [UNTOUCHED 🔒]
```

---

## 5. Certification & Pass/Fail Criteria for Phase 0A

```
================================================================================
 PHASE 0A CERTIFICATION MATRIX
================================================================================
 [CRITERION 1] STATIC 8-PANEL PRESENTATION:
   - Dashboard successfully displays 8 cleanly bounded telemetry panels.
   - Every panel clearly displays "STATUS: NOT CONNECTED".
   - Status: MANDATORY PASS

 [CRITERION 2] ZERO HEAP ALLOCATION:
   - Zero kmalloc/kfree calls occur during dashboard rendering.
   - Status: MANDATORY PASS

 [CRITERION 3] CLEAN NAVIGATION SEQUENCE:
   - System transitions cleanly: BOOT SPLASH ➔ DASHBOARD ➔ LOGIN.
   - Status: MANDATORY PASS

 [CRITERION 4] CLEAN BUILD & REGRESSION-FREE:
   - Build compiles with 0 warnings, 0 errors.
   - Status: MANDATORY PASS
================================================================================
```

*Phase 0A Architecture Specification Complete.*
