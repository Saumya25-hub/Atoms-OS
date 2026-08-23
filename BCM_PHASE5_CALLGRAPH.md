# BCM Phase 5 Static Forensic Call Graph

## 1. Static Analysis: Composition & Presentation Callers

```
========================================================================================
FUNCTION: BWE_ComposeFrame(const BVFramebuffer* hw_fb)
----------------------------------------------------------------------------------------
[CALLER 1] BCM_Process() in kernel/wm/bcm/src/bcm_core.c:L356
  - Context: Task Context (bcm_compositor_thread)
  - RFLAGS.IF: 1 (Preemptible)
  - CPL: 0
  - Verdict: AUTHORIZED (Authoritative Compositor Path)

[CALLER 2] BWE_Compose() in kernel/wm/bwe/src/bwe_core.c:L810
  - Context: Compatibility Wrapper
  - Inbound Callers: 0 (All IRQ/Syscall triggers decoupled to BCM_RequestDamage)
  - Verdict: INERT COMPATIBILITY STUB

[CALLER 3] BOHeart_Pulse() in kernel/wm/bwe/src/bwe_core.c:L933
  - Context: Deprecated legacy hook
  - Inbound Callers: 0
  - Verdict: INERT STUB

[CALLER 4] bgl_swap_buffers() in kernel/graphics/bgl/bgl.c:L116
  - Context: Ring 3 / Task Mode OpenGL context
  - RFLAGS.IF: 1
  - Verdict: AUTHORIZED
========================================================================================

========================================================================================
FUNCTION: BOVISUAL_Graphics_SwapFull(const BVFramebuffer* hw_fb)
----------------------------------------------------------------------------------------
[CALLER 1] BWE_ComposeFrame() in kernel/wm/bwe/renderer/bwe_compositor.c:L1125
  - Context: Task Context (via BCM_Process -> BWE_ComposeFrame)
  - RFLAGS.IF: 1 (Guaranteed by BWE_ComposeFrame & BOVISUAL_Graphics_SwapFull firewalls)
  - Verdict: AUTHORIZED

[CALLER 2] Horse Engine in kernel/engine/horse_engine.c:L53, L179
  - Context: Task Mode Engine
  - RFLAGS.IF: 1
  - Verdict: AUTHORIZED
========================================================================================

========================================================================================
FUNCTION: AGDTE_Presenter_PresentBridgeBSPE(const BOGE_StagingFrame* boge_frame, ...)
----------------------------------------------------------------------------------------
[CALLER 1] BOVISUAL_Graphics_SwapFull() in bovisual/Graphics/graphics.c:L345
  - Context: Task Context
  - RFLAGS.IF: 1 (Firewalled)
  - Verdict: AUTHORIZED
========================================================================================

========================================================================================
FUNCTION: BSPE_DualPage_PresentFrame(BSPE_DamageTrackerHandle dt, const BOGE_StagingFrame* frame)
----------------------------------------------------------------------------------------
[CALLER 1] BSPE_PresentFrame() in kernel/graphics/BSPE/Present/bspe_present.c:L241
  - Context: Task Context (via AGDTE_Presenter_PresentBridgeBSPE)
  - RFLAGS.IF: 1 (Firewalled)
  - Verdict: AUTHORIZED
========================================================================================

========================================================================================
FUNCTION: BSPE_VRAM_CopyEffectiveDamage(...)
----------------------------------------------------------------------------------------
[CALLER 1] BSPE_DualPage_PresentFrame() in kernel/graphics/BSPE/Present/dual_page_present.c:L258
  - Context: Task Context
  - RFLAGS.IF: 1 (Firewalled)
  - Verdict: AUTHORIZED
========================================================================================
```

---

## 2. Hardware IRQ Context Guarantee

$$\forall\ \text{Paths from Hardware IRQ 0 / ISR} \implies \text{Composition Reachability} = \mathbf{ZERO}$$
$$\forall\ \text{Paths from Hardware IRQ 0 / ISR} \implies \text{VRAM Copy Reachability} = \mathbf{ZERO}$$
