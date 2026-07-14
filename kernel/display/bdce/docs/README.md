# BDCE — BOS Display Contract Engine (Architectural Authority Layer)

> **Phase A Foundation (`Dormant Layer`)**  
> **Rule 0 Enforced:** Zero behavior change. Zero rendering change. Zero memcpy changes. Zero page-flip changes.

---

## Overview

The `kernel/display/bdce/` subsystem is the **Architectural Authority Layer** and **Display Constitution Enforcement Engine** for SignaturesOS (`v1.0`).

### Core Identity
$$\text{\textbf{BDCE owns Truth. BDCE never owns Algorithms.}}$$

`BDCE` is not a rendering engine, compositor, blitter, or scaling algorithm repository. It is the centralized governance layer that authoritatively maintains and enforces the **9 Pillars of Display State**:

1. `BDCE_PhysicalState`
2. `BDCE_LogicalState`
3. `BDCE_SurfaceState`
4. `BDCE_TemporalState`
5. `BDCE_FrameState`
6. `BDCE_PresentationState`
7. `BDCE_DamageState`
8. `BDCE_OwnershipState`
9. `BDCE_CapabilityState`

---

## Phase A Scope & Rules

In **Phase A**, `BDCE` is compiled and linked into `kernel.bin` in a **100% dormant state**.

* **Zero Subsystem Migration:** No existing subsystem (`DIE, AGDAE, BWE, AGDTE, BSPE, BOVISUAL, VBE`) queries or writes to `BDCE` during Phase A.
* **Zero Global Retirement:** `g_kernel_screen_width` remains untouched until Phase G.
* **Zero Behavioral Deviation:** The operating system boots, logs in, and renders with exact binary parity to pre-Phase A.

---

## Directory Layout

```text
kernel/display/bdce/
├── include/
│   ├── bdce_authority.h   ── Public API declarations and getter stubs
│   ├── bdce_context.h     ── Context container encapsulating all 9 State pillars
│   ├── bdce_snapshot.h    ── Immutable frame snapshot structure (`DisplaySnapshot`)
│   └── bdce_types.h       ── Core State structs, Rectangles, and Ownership enums
├── src/
│   └── bdce_authority.c   ── Phase A dormant stub implementation
└── docs/
    └── README.md          ── Subsystem documentation & constitutional overview
```
