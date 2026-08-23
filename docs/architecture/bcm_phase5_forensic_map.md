# BCM Phase 5 Forensic Map: IRQ Composition Decoupling

## 1. Executive Summary & Root Cause Confirmation
In ATOMS OS, a catastrophic hardware shutdown and vCPU freeze occurred when interacting with multi-window workloads (such as entering "2 + 2" in Calculator while Explorer and Terminal were active). 

Forensic analysis proved that the root cause was **synchronous frame composition and 3.14 MB PCIe VRAM presentation being triggered from within Hardware Timer Interrupt 0 (`timer_tick_handler`) with `RFLAGS.IF = 0`**.

```
[Hardware Timer IRQ 0] 
  │ (RFLAGS.IF = 0)
  ▼
timer_tick_handler() [kernel/core/timer/src/timer.c:L52]
  │ (RFLAGS.IF = 0)
  ▼
BRE_DispatchPending() [kernel/core/brtsl/bre.c:L55]
  │ (RFLAGS.IF = 0)
  ▼
bre_input_pump_callback() [kernel/wm/bwe/src/bwe_core.c:L480]
  │ (RFLAGS.IF = 0)
  ▼
BWE_PumpEvents() [kernel/wm/bwe/src/bwe_core.c:L490]
  │ (RFLAGS.IF = 0)
  ▼
BWE_Compose() [kernel/wm/bwe/src/bwe_core.c:L796]  <-- CRITICAL ARCHITECTURAL VIOLATION
  │ (RFLAGS.IF = 0)
  ▼
BWE_ComposeFrame() [kernel/wm/bwe/renderer/bwe_compositor.c:L777]
  │ (RFLAGS.IF = 0)
  ▼
compose_window_recursive() [kernel/wm/bwe/renderer/bwe_compositor.c:L380]
  │ (RFLAGS.IF = 0)
  ▼
BOVISUAL_Graphics_SwapFull() [bovisual/Graphics/graphics.c:L311]
  │ (RFLAGS.IF = 0)
  ▼
AGDTE_Presenter_PresentBridgeBSPE() [kernel/graphics/AGDTE/src/agdte_presenter.c:L80]
  │ (RFLAGS.IF = 0)
  ▼
BSPE_PresentFrame() [kernel/graphics/BSPE/Present/bspe_present.c:L241]
  │ (RFLAGS.IF = 0)
  ▼
BSPE_DualPage_PresentFrame() [kernel/graphics/BSPE/Present/dual_page_present.c:L172]
  │ (RFLAGS.IF = 0)
  ▼
BSPE_VRAM_CopyEffectiveDamage() [kernel/graphics/BSPE/Present/vram_copy.c:L237]
  │ (RFLAGS.IF = 0)
  ▼
PCIe MMIO 3.14MB+ Framebuffer Copy (39.6 ms - 67.9 ms CPU stall)
```

Because the hardware timer tick period is 1 ms, a 39.6 ms - 67.9 ms stall inside a single timer ISR starved all system interrupts, prevented task scheduling, triggered hardware watchdogs, and caused total system failure.

---

## 2. Complete Caller & Callee Audit

### A. All Callers of `BWE_Compose()`
1. `kernel/wm/bwe/src/bwe_core.c:L796` inside `BWE_PumpEvents()`: **[IRQ PATH - MUST REMOVE]**
2. `kernel/core/syscall/src/services.c:L233, L239` inside `sys_service_gui_show_window()`: **[SYSCALL PATH - REPLACE WITH BCM DAMAGE REQUEST]**

### B. All Callers of `BWE_ComposeFrame()`
1. `kernel/wm/bwe/src/bwe_core.c:L810` inside `BWE_Compose()` compatibility wrapper.
2. `kernel/wm/bwe/src/bwe_core.c:L933` inside `BOHeart_Pulse()` (deprecated legacy hook).
3. `kernel/graphics/bgl/bgl.c:L116` inside `bgl_swap_buffers()`.
4. `kernel/wm/bcm/src/bcm_core.c:L356` inside `BCM_Process()`: **[AUTHORITATIVE PREEMPTIBLE WORKER PATH - TARGET]**

### C. All Callers of `BOVISUAL_Graphics_SwapFull()`
1. `kernel/wm/bwe/renderer/bwe_compositor.c:L1125` inside `BWE_ComposeFrame()`.
2. `kernel/engine/horse_engine.c:L53, L179`.

### D. Presentation Chain Callers
1. `AGDTE_Presenter_PresentBridgeBSPE()`: called by `BOVISUAL_Graphics_SwapFull()`.
2. `BSPE_PresentFrame()`: called by `AGDTE_Presenter_PresentBridgeBSPE()`.
3. `BSPE_DualPage_PresentFrame()`: called by `BSPE_PresentFrame()`.
4. `BSPE_VRAM_CopyEffectiveDamage()`: called by `BSPE_DualPage_PresentFrame()`.

---

## 3. Execution Context Boundaries & Invariants

```mermaid
graph TD
    subgraph Hardware IRQ Context [RFLAGS.IF = 0 (NON-PREEMPTIBLE)]
        A[timer_tick_handler] --> B[BRE_DispatchPending]
        B --> C[bre_input_pump_callback]
        C --> D[BWE_PumpEvents]
        D --> E[Pop Input Events]
        E --> F[BCM_RequestDamage / BCM_RequestCursorDamage]
        F --> G[Mark BCM Pending Damage]
        G --> H[Return from IRQ / Send EOI]
    end

    subgraph Preemptible Task Context [RFLAGS.IF = 1 (PREEMPTIBLE CPL=0)]
        I[bcm_compositor_thread] --> J[BCM_HasPendingDamage]
        J --> K[BCM_FrameDeadlineReached 60 FPS]
        K --> L[BCM_Process]
        L --> M[Execution Context Firewall Check IF=1]
        M --> N[BWE_ComposeFrame]
        N --> O[compose_window_recursive]
        O --> P[BOVISUAL_Graphics_SwapFull]
        P --> Q[BSPE VRAM Presentation]
        Q --> R[Yield / Sleep]
    end

    H -.->|Context Switch / IRETQ| I
```

---

## 4. Execution & Presentation Firewalls

1. **`BCM_Process()` Firewall**:
   - Checks `bcm_is_interrupt_enabled()`.
   - If `IF=0`: records `[BCM][SECURITY] COMPOSITION BLOCKED: IF=0`, increments `reentrancy_blocks`, and returns `BCM_ERR_INVALID_STATE`.
2. **`BWE_ComposeFrame()` Firewall**:
   - Checks `IF=1`. If `IF=0`, logs security violation and aborts immediately.
3. **`BOVISUAL_Graphics_SwapFull()` Firewall**:
   - Checks `IF=1`. If `IF=0`, logs `[BCM][SECURITY] PRESENTATION BLOCKED: IF=0` and aborts.
4. **`AGDTE_Presenter_PresentBridgeBSPE()` & `BSPE_DualPage_PresentFrame()` Firewalls**:
   - Enforce task-only execution. If called with `IF=0`, reject presentation and protect VRAM bus.

---

## 5. Exact Files & Functions to Modify in Phase 5

| File Path | Function | Change Description |
| :--- | :--- | :--- |
| `kernel/wm/bwe/src/bwe_core.c` | `BWE_PumpEvents()` | **Remove `BWE_Compose()` call completely.** |
| `kernel/core/syscall/src/services.c` | `sys_service_gui_show_window()` | Remove synchronous `BWE_Compose()`, rely on `BCM_RequestWindowDamage()`. |
| `kernel/wm/bcm/src/bcm_core.c` | `BCM_Process()` | Connect `BWE_ComposeFrame(vbe_get_framebuffer())` inside `BCM_STATE_COMPOSING` with full `IF=1` firewall. |
| `kernel/wm/bwe/renderer/bwe_compositor.c` | `BWE_ComposeFrame()` | Add `IF=1` defensive firewall check at entry. |
| `bovisual/Graphics/graphics.c` | `BOVISUAL_Graphics_SwapFull()` | Add `IF=1` defensive firewall check at entry. |
| `kernel/graphics/AGDTE/src/agdte_presenter.c` | `AGDTE_Presenter_PresentBridgeBSPE()` | Add `IF=1` defensive firewall check at entry. |
| `kernel/graphics/BSPE/Present/dual_page_present.c` | `BSPE_DualPage_PresentFrame()` | Add `IF=1` defensive firewall check at entry. |

---

## 6. Risk Analysis & Rollback Strategy

- **Risk**: Loss of frame update if an event does not register damage in BCM.
  - **Mitigation**: All window invalidations (`BWE_InvalidateWindow`), control clicks, cursor motions, and show/hide actions already call `BCM_RequestDamage()`.
- **Risk**: Stale cursor trails.
  - **Mitigation**: `BCM_RequestCursorDamage()` submits both previous and current 32x32 bounding boxes on every mouse movement.
- **Rollback**: Every change is localized to composition invocation sites and execution firewalls without altering window data structures or layout math.
