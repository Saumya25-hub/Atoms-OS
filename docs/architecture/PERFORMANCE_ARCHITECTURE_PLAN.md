# PERFORMANCE ARCHITECTURE PLAN — 4/5 APP MULTI-WINDOW STABILITY & VCPU PROTECTION
**Author:** ATOMS OS Architecture Team  
**Input:** `PERFORMANCE_FORENSIC_REPORT.md`  
**Classification:** Production Performance & Stability Design  
**Status:** TASK 2 COMPLETE — Architectural Specification (NO SOURCE CODE MODIFICATIONS YET)

---

## 1. Objectives & Safety Invariants

1. **Eliminate Virtual CPU Shutdown Permanently**: Ensure the hardware timer interrupt service routine (`timer_handler`) executes in $< 5\text{ }\mu\text{s}$ and NEVER blocks inside heavy graphics composition or PCIe memory transfers.
2. **Deterministic 1 $\to$ 5 Application Scaling**: Reduce 5-window composition time from **$58.8\text{ ms}$** down to **$\le 16.6\text{ ms}$** ($\ge 60\text{ FPS}$).
3. **95% PCIe VRAM Bandwidth Reduction**: Activate BSPE Partial VRAM Presentation so only changed rectangular regions are copied across PCIe instead of the full 3.14MB–16MB frame.
4. **Zero-Lag Interactive Mouse Dragging**: Implement safe mouse move event coalescing in the BWE event queue to prevent backlog and rubber-banding during fast window manipulation.
5. **Truthful & Reconciled Performance Telemetry**: Hook dirty area and VRAM byte counters directly into active presentation paths and harmonize frame counters.

---

## 2. Target Files & Surgical Change Matrix

| Target File | Subsystem | Planned Change | Rationale | Risk & Mitigation |
| :--- | :--- | :--- | :--- | :--- |
| `kernel/core/timer/src/timer.c` | Core Timer & ISR | Remove direct synchronous rendering dispatch from Timer ISR; ensure timer ISR only signals ticks and bounded non-graphical state. | Prevents ISR budget overrun, APIC IRR interrupt pile-up, and VMware vCPU shutdown. | **Low**: Keep `atoms_p7_tick` and scheduler accounting intact. |
| `kernel/wm/bwe/src/bwe_core.c` | BWE Event Pump | Add mouse move event coalescing in `BWE_PumpEvents()`; invoke composition safely outside interrupt context. | Prevents 50+ redundant window drag recalculations during heavy mouse motion. | **Low**: Strict preservation of button down/up and click event order. |
| `bovisual/Graphics/graphics.c` | Graphics Presentation | Enable `bspe_use_partial_present = true;` and forward active dirty rectangles into `BOGE_StagingFrame`. Hook `bos_profiler_record_mem_copy()`. | Replaces 3.14 MB brute-force copy with partial dirty rectangle transfers ($< 150\text{ KB}$). | **Low**: Automatic fallback to `LegacySwapFull` is retained if damage bounds are invalid. |
| `kernel/wm/bwe/renderer/bwe_compositor.c` | BWE Compositor Core | Add re-entrancy protection guard in `BWE_ComposeFrame()`; hook `bos_profiler_record_dirty_rect()`; optimize unified dirty clipping. | Prevents stack exhaustion, re-entrant frame draws, and duplicate multi-window passes. | **Low**: Zero changes to window hierarchy, title bars, borders, or taskbar layout. |
| `kernel/performance/statistics/stats_profiler.c` | PPE Telemetry Core | Correctly compute rolling bandwidth and average dirty region from recorded per-frame metrics. | Provides 100% truthful, verifiable performance reports. | **Zero**: Profiling telemetry only. |

---

## 3. Detailed Architectural Specifications

### Component A: Timer ISR & Composition Decoupling
- **Problem**: `timer.c:L52` was calling `BRE_DispatchPending()` $\to$ `BWE_PumpEvents()` $\to$ `BWE_ComposeFrame()`, locking the CPU inside IRQ 0 for 58 ms.
- **Architectural Fix**:
  - `timer.c` executes quick tick accounting and returns immediately.
  - Interactive GUI polling and composition are executed cooperatively in the main system/desktop task loop when dirty flags are set, with a re-entrancy lock guaranteeing that only one composition pass executes at a time.

### Component B: Mouse Move Event Coalescing
- **Problem**: Moving the mouse generates up to 1000 events/sec. When frame time is 50 ms, 50 mouse move events queue up, each triggering `BOS_SetBounds()`.
- **Architectural Fix**:
  - In `BWE_PumpEvents()`, when inspecting the event queue:
    - If consecutive `BWE_EVENT_MOUSE_MOVE` events are present without intervening button clicks, advance directly to the most recent coordinate $(X, Y)$.
    - Dispatch `BWE_ProcessMouseInteraction()` once with the coalesced position.
    - All `BWE_EVENT_MOUSE_DOWN`, `MOUSE_UP`, and `KEY_DOWN` events are never dropped or coalesced.

### Component C: BSPE Partial VRAM Damage Presentation
- **Problem**: `bspe_use_partial_present` was set to `false`, causing `BSPE_DualPage_PresentFrame` to execute `BOVISUAL_Graphics_LegacySwapFull_Backend()` (3.14 MB) on 100% of frames.
- **Architectural Fix**:
  - Set `bspe_use_partial_present = true;`.
  - Pass the merged `g_dirty_rects` and `g_dirty_rect_count` into `staging_frame`.
  - `BSPE_VRAM_CopyEffectiveDamage()` copies only the damaged scanlines.
  - Record transferred bytes into `bos_profiler_record_mem_copy(bytes, true)`.

### Component D: Compositor Re-Entrancy & Profiler Integration
- **Problem**: Nested compose calls could occur if a callback triggered an invalidation, risking stack overflow.
- **Architectural Fix**:
  - Implement `static bool s_is_composing = false;` in `BWE_ComposeFrame()`.
  - If `s_is_composing == true`, mark `s_pending_recompose = true` and return immediately.
  - Call `bos_profiler_record_dirty_rect(current_dirty.width, current_dirty.height)` for each damage box.

---

## 4. Rollback & Failsafe Plan

- **Automatic Presentation Fallback**: If partial VRAM copying detects invalid coordinates or corrupted bounds, `BSPE_DualPage_PresentFrame` automatically falls back to `BOVISUAL_Graphics_LegacySwapFull_Backend()`.
- **Zero Regression Guarantee**: Taskbar, Start Menu, Explorer, and Ring 3 isolation remain 100% untouched.

---
*End of PERFORMANCE_ARCHITECTURE_PLAN.md — Ready for Task 3: Patch Execution Phase (`PERFORMANCE_PATCH_REPORT.md`)*
