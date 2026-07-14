# Phase 04: Cursor Presentation Pipeline Autopsy

## Overview
This phase investigates why the software cursor remains invisible despite being correctly processed by the input pipeline. We traced the complete presentation lifecycle of a single cursor frame, from coordinate update to physical VRAM presentation.

## Stage 1: Cursor State Update
**Status: PASS**
- **Function:** `cursor_state_set_position`
- **Execution:** Called consistently via `bwe_input.c` when `INPUT_EVENT_TYPE_MOTION_ABSOLUTE` is received.
- **Evidence:** Updates `g_cursor_state.screen_x` and `screen_y`. Coordinates are bounded properly.

## Stage 2: Cursor Backend Selection
**Status: PASS**
- **Function:** `cursor_backend_is_hardware`
- **Execution:** Evaluated in `BSPE_CursorPresenter_OnCompositorRedraw` and `BWE_ComposeFrame`.
- **Evidence:** Initially sets hardware backend, but triggers fallback to `CURSOR_BACKEND_SOFTWARE` after the first `hw_driver_set_pos` returns `BSPE_ERR_UNSUPPORTED`. Returns `false` consistently thereafter, ensuring software rendering path executes.

## Stage 3: Cursor Bitmap Fetch
**Status: PASS**
- **Function:** `cursor_theme_get_bitmap` & `init_arrow_sprite`
- **Execution:** `s_bitmap` is populated during initialization via `BSPE_CursorPresenter_Init`.
- **Evidence:** The default arrow sprite is 16x24 pixels, correctly populated with opaque ARGB pixels (`0xFF000000` borders, `0xFFFFFFFF` body). No transparent-color anomalies.

## Stage 4: Software Cursor Renderer
**Status: PASS**
- **Function:** `cp_draw_box` (via `BVCursor_Draw` -> `cursor_engine_render_overlay` -> `BSPE_CursorPresenter_OnCompositorRedraw`)
- **Execution:** Called at the end of `BWE_ComposeFrame` (Step 17).
- **Evidence:** 
  - Retrieves `ram_fb` via `BOVISUAL_Graphics_GetBuffer()`.
  - Calculates perfectly mapped indices: `screen_idx = (draw_y + y) * 1920 + (draw_x + x)`.
  - Writes valid ARGB values into `ram_fb` using alpha-blending logic.

## Stage 5: BWE Compositor Integration & Async Race Condition
**Status: FAIL (CRITICAL BUG 1)**
- **Function:** `BWE_ComposeFrame` vs. `AGDTE_Pulse`
- **Root Cause:** Asynchronous single-buffer data race.
- **Evidence:**
  1. `BWE_ComposeFrame` redraws the desktop background over the dirty mouse rects in `ram_fb` (Step 15).
  2. `BVCursor_Draw` draws the cursor into `ram_fb` (Step 17).
  3. `BOVISUAL_Graphics_SwapFull` queues the `ram_fb` pointer to `AGDTE` asynchronously.
  4. `AGDTE` delays execution due to VSYNC pacing (`AGDTE_DECISION_WAIT_PACING`).
  5. The main loop spins and `BWE_ComposeFrame` executes again for the *next* frame.
  6. The background is redrawn *over* the cursor in `ram_fb`.
  7. `AGDTE_Pulse` finally executes the `BSPE_VRAM_CopyEffectiveDamage` for the *previous* frame, copying the newly restored desktop background instead of the cursor.

## Stage 6: BSPE Presentation Pipeline (Stationary Cursor)
**Status: FAIL (CRITICAL BUG 2)**
- **Function:** `BSPE_DualPage_PresentFrame`
- **Root Cause:** Aggressive early-out dropping frames with 0 damage.
- **Evidence:**
  - If the mouse stops moving, `BWE_ComposeFrame` sets `g_dirty_rect_count = 0`.
  - `BVCursor_Draw` draws the cursor.
  - `staging_frame.dirty_count` is set to 0.
  - `BSPE_DualPage_PresentFrame` sees `effective_count == 0` and returns `BSPE_OK` immediately, completely bypassing `BSPE_VRAM_CopyEffectiveDamage`.
  - Result: When the mouse is stationary, the cursor is drawn to `ram_fb` but *never* copied to VRAM.

## Conclusion
The cursor is invisible due to a classic **single-buffer concurrency bug** coupled with **asynchronous pacing**, and exacerbated by **damage-culling logic** dropping stationary frames. The software cursor is effectively erased from RAM before the display controller ever reads it, and dropped entirely when the mouse isn't moving.

### Evidence Score
- **Asynchronous Race Condition:** 5/5 (Proven via `AGDTE_Pulse` pacing vs `BWE_PumpEvents` single-buffer).
- **Damage Culling Drop:** 5/5 (Proven via `BSPE_DualPage_PresentFrame:203`).

### Next Steps
Do not guess. Awaiting explicit user instruction on whether to implement a fix (e.g., dual-buffering the RAM composite, moving cursor drawing to the AGDTE copy phase, or forcing synchronization).
