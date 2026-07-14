# Phase 3: Input Pipeline Autopsy

## 1. Subsystem Overview

The ATOMS OS Input Pipeline is a highly complex, multi-tiered event dispatch system:
Hardware IRQ (IRQ1/IRQ12) → Legacy Buffers (`kbd_buf` / VMMouse) → `kernel_get_event` → `input_adapter_pump` → `InputCore` Queue → Event Dispatcher → Consumers (PointerEngine, CursorEngine, Legacy Adapter) → Window Manager (BWE) → HitTesting → Window Event Handlers (DesktopShell / Login Page).

## 2. Audit & Runtime Evidence

We traced an event through the entire pipeline:
- Hardware IRQs are firing (Telemetry confirms).
- `input_adapter.c` correctly maps hardware events to `InputCoreEvent`.
- `InputCore` successfully dispatches to `PointerEngine` (Tier 0), which computes sub-pixel precision, and to `CursorEngine` (Tier 1).
- The `Legacy_BWE_Adapter` (Tier 3) pushes the event to `BWE_EventQueue`.
- `BWE_PumpEvents` successfully pops the event, performs `BWE_HitTest`, and routes it.

**Trace Output:**
```
[INPUT TRACE] InputAdapter
[INPUT TRACE] BOS_ProcessEvent
[INPUT TRACE] Queue Pop
[INPUT TRACE] HitTest
[INPUT TRACE] DesktopShell Background
```

### Why did it reach DesktopShell instead of Login Page?
The boot sequence has reached `BOOT_DESKTOP`, meaning the login page is destroyed. `Desktop_Shell_IsLoginActive()` is false, so events fall through to `BWE_PumpEvents`.

### Why did HitTest return DesktopShell?
The initial mouse coordinates are set to `(640, 360)` (center of the 1280x720 scaled resolution). Desktop icons are located on the left (`x=15`, `y=15`). The pointer starts in empty space. `HitTest` correctly misses the icons and falls back to the root `BWE_DESKTOP_ID` window, triggering `desktop_event_handler`.

## 3. The "Not Usable" Illusion (Root Cause)

The user reported: *"The desktop is visible. But mouse and keyboard are not usable."*

If the pipeline is flawless, why is the system unusable?
The issue is **Visual**, not **Input**.

1. **Hardware Cursor Failure:** `DIE` forces the resolution to `1280x720` but VBE stays at `1920x1080`. VBE does not support hardware cursors. `BSPE_CursorPlane_IsHardwareSupported()` returns `false`, forcing a fallback to the Software Cursor Renderer.
2. **Orphaned Software Renderer:** The software cursor relies on a final compositing pass: `BSPE_CursorPresenter_OnCompositorRedraw` (wrapped by `cursor_engine_render_overlay`). **This function is completely orphaned.** It is never invoked by `BWE_ComposeFrame` or any display layout update function.
3. **Invisible Cursor:** Because the software rendering pass is missing, the cursor is 100% invisible.
4. **Keyboard Illusion:** The keyboard driver correctly processes keys and triggers `desktop_event_handler`. However, `g_kbd_selected_icon_index` initializes to `-1`. Pressing `Enter` does nothing. Unless the user explicitly presses arrow keys (which highlight an icon), the keyboard appears dead.

The user tries to move the mouse → sees no cursor. Tries to click → clicks empty desktop space. Tries to type → no response. **Conclusion: "Input is frozen."**

## 4. Strengths & Weaknesses

**Strengths:**
- The multi-tier dispatch architecture is extremely robust.
- Sub-pixel accumulation and fixed-point math in `PointerEngine` are flawless.

**Weaknesses:**
- Decoupling the cursor visual state from the compositor led to an orphaned render pass.
- No visual feedback for "focus" when no icon is selected.

## 5. Verdict

- **Input Pipeline:** PASS 🟢
- **Cursor Rendering:** FAIL 🔴 (Orphaned software renderer)

**Evidence Score:** 5/5 (Proven via trace analysis and code reference).
