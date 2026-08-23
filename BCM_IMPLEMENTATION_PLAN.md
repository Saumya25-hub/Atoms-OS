# BOS COMPOSITION MANAGER (BCM) — IMPLEMENTATION & MIGRATION PLAN

**Document ID:** `BCM_IMPLEMENTATION_PLAN.md`  
**Classification:** Phase 0 Architecture Deliverable (Design & Migration Roadmap)  
**Status:** **PLANNING ONLY — ZERO SOURCE CODE MODIFIED IN THIS PHASE**  

---

## 1. Files to Create

| Target File | Subsystem | Responsibility | Dependencies |
|---|---|---|---|
| `kernel/wm/bcm/include/bcm.h` | BCM Core Header | Public BCM types, state enums, API declarations, IRQ-safe macros | `<stdint.h>`, `<stdbool.h>` |
| `kernel/wm/bcm/include/bcm_internal.h` | BCM Internal Header | Internal damage ring buffers, coalescer structures, profiler hooks | `bcm.h`, `bwe.h` |
| `kernel/wm/bcm/src/bcm_core.c` | BCM State & Scheduling | State machine transitions, rate pacing (60 FPS), request coalescing | `bcm.h`, `timer.h` |
| `kernel/wm/bcm/src/bcm_damage.c` | BCM Damage Collector | Ingestion of window/cursor/theme damage, bounding box intersection | `bcm.h`, `bwe.h` |
| `kernel/wm/bcm/src/bcm_task.c` | BCM Compositor Task | Dedicated kernel compositor worker thread loop (`bcm_compositor_thread`) | `scheduler.h`, `bwe.h` |

---

## 2. Existing Files to Modify (Post-Phase 0 Migration)

| Existing File | Current Logic | Planned Target Modification |
|---|---|---|
| [`kernel/wm/bwe/src/bwe_core.c`](file:///d:/Signatures_OS/kernel/wm/bwe/src/bwe_core.c) | `BWE_PumpEvents()` calls `BWE_Compose()` synchronously | Replace synchronous `BWE_Compose()` with non-blocking `BCM_RequestDamage()`. |
| [`kernel/wm/bwe/src/bwe_window.c`](file:///d:/Signatures_OS/kernel/wm/bwe/src/bwe_window.c) | `BWE_InvalidateWindow()` modifies dirty rect arrays directly | Route window damage through `BCM_RequestWindowDamage(win_id)`. |
| [`kernel/wm/bwe/renderer/bwe_compositor.c`](file:///d:/Signatures_OS/kernel/wm/bwe/renderer/bwe_compositor.c) | Manages dirty rect array and full VRAM presentation inside `BWE_ComposeFrame()` | Delegate damage ingestion to BCM; `BWE_ComposeFrame()` renders only active BCM damage sets. |
| [`kernel/core/timer/src/timer.c`](file:///d:/Signatures_OS/kernel/core/timer/src/timer.c) | `timer_tick_handler()` calls `BRE_DispatchPending()` (which was rendering) | `timer_tick_handler()` calls `BCM_NotifyTimerTick()` (pure non-blocking tick notifier). |
| [`kernel/kernel.c`](file:///d:/Signatures_OS/kernel/kernel.c) | Spawns desktop shell and starts scheduler | Initialize BCM (`BCM_Init()`) and spawn dedicated kernel task `bcm_compositor_thread`. |
| [`arch/x86_64/interrupt/idt.c`](file:///d:/Signatures_OS/arch/x86_64/interrupt/idt.c) | `idt[vector].ist = 0` for all 256 gates | Arm TSS IST1 with 16 KB emergency stack for `#DF` (vector 8) and `#PF` (vector 14). |

---

## 3. Authoritative BCM API Surface to Introduce

```c
/* ========================================================================= */
/* BCM Core Lifecycle APIs (Called during kernel boot)                       */
/* ========================================================================= */
bcm_error_t BCM_Init(void);
bcm_error_t BCM_StartCompositorTask(void);

/* ========================================================================= */
/* IRQ-Safe Damage Ingestion APIs (Non-blocking, O(1), no memory allocation)  */
/* ========================================================================= */
void BCM_RequestDamage(int32_t x, int32_t y, int32_t width, int32_t height);
void BCM_RequestWindowDamage(uint32_t window_id);
void BCM_RequestCursorDamage(int32_t old_x, int32_t old_y, int32_t new_x, int32_t new_y);
void BCM_RequestFullRepaint(void);
void BCM_NotifyTimerTick(uint64_t tick_count);

/* ========================================================================= */
/* BCM Compositor Task Execution API (Runs strictly in Task Context, IF=1)   */
/* ========================================================================= */
void BCM_Process(void);
BCM_FrameState BCM_GetState(void);
const BCM_Telemetry* BCM_GetTelemetry(void);
```

---

## 4. Phased Migration Order

```text
Phase M1: Architecture & Header Definitions (BCM Types, State Enums, Structs)
  ↓
Phase M2: Dedicated Compositor Task & State Machine Implementation (bcm_core.c, bcm_task.c)
  ↓
Phase M3: Damage Ingestion & Coalescing Engine (bcm_damage.c)
  ↓
Phase M4: Decouple In-ISR BWE_Compose() from bwe_core.c (Forward to BCM)
  ↓
Phase M5: Arm TSS IST1 Emergency Stacks in idt.c & gdt.c
  ↓
Phase M6: End-to-End Multi-Window Stress Validation & QEMU / VMware / Bare-Metal Certification
```

---

## 5. Verification & Testing Strategy

1. **Timer ISR Duration Validation:** Measure `timer_tick_handler()` entry/exit under 5-window load. Must remain **< 0.05 ms** (50 µs).
2. **QEMU Automated Stress Test:** Run `reproduce_calc_crash.py` with Explorer, Terminal, Calculator open simultaneously, evaluating 50+ rapid button clicks.
3. **VMware Workstation Certification:** Verify zero `vCPU shutdown` warnings or Triple Faults.
4. **Physical Haswell H81 Hardware Certification:** Flash/PXE test image to real Haswell H81 motherboard, execute 100+ multi-window operations, verify stable power and 60 FPS UI responsiveness.

---

## 6. Rollback Strategy
If any regression occurs during migration:
1. Reverting `bwe_core.c` restores the previous direct call flow without affecting window structures or driver interfaces.
2. BCM is designed as an external modular coordinator: all existing BWE drawing functions (`BWE_ComposeFrame`, `compose_window_recursive`) retain their exact internal signatures.
