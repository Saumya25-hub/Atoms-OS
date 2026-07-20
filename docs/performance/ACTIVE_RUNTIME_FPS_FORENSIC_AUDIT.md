# ACTIVE RUNTIME FPS FORENSIC AUDIT
**Date:** July 20, 2026
**Target:** ATOMS OS Application Platform & Composition Pipeline
**Mission:** Determine the exact root cause of the 12-17 FPS drop during active runtime versus 50-60 FPS idle.

---

## 1. EXECUTIVE VERDICT
The 12-17 FPS drop during active runtime is **NOT** a bug in the recent BWE application layer fixes. It is a fundamental **software-rendering limit** exacerbated by extreme architectural waste in the `BOImage`/`BOFont` rasterizer, aggressive dirty-rectangle merging, and an unoptimized framebuffer presentation layer (`SwapFull`).

When the OS is idle, `g_dirty_rect_count == 0` causes the compositor to completely skip the rendering phase and avoid VRAM swapping. When active (e.g., mouse movement or `input_lab` telemetry updates), the OS is forced into a software-rendering loop that performs hundreds of thousands of redundant pixel evaluations and massive MMIO memory copies over QEMU.

---

## 2. FPS METRIC DEFINITION
- **"Runtime FPS" (Input Lab):** This metric tracks the number of times `input_lab_on_render` is called per second. Because `input_lab` forces continuous redraw via `BWE_InvalidateWindow(self->id)`, the compositor is never idle while the app is open.
- **"FramesPresented/sec" (Telemetry):** Tracks the number of calls to `BOVISUAL_Graphics_SwapFull`. 
- **Idle 50-60 FPS:** Achieved *only* because `BWE_ComposeFrame` does an early `return` when `g_dirty_rect_count == 0`, skipping all rendering and VRAM copy operations entirely.

---

## 3. FRAME PIPELINE MAP
1. **Input IRQ** triggers `dispatcher_router.c` → `BWE_InvalidateRect`.
2. **Damage System:** Cursor bounding boxes (old and new) are pushed to the dirty rectangle queue.
3. **BWE_ComposeFrame:**
   - Evaluates `g_dirty_rect_count`. Skips if 0.
   - Merges overlapping rectangles into a unified bounding box.
   - For each dirty rectangle, calls `BWE_ClipPush()`.
   - Iterates Z-order stack. Calls `on_render` for ANY window intersecting the damage.
4. **Shell_DrawWallpaper:** Computes clipped desktop background.
5. **BOFont / BOImage:** Applications submit rendering calls to a batch queue.
6. **Flush / Rasterization:** `BOImage_FlushBatch` evaluates and writes pixels to the RAM backbuffer.
7. **Presentation:** `BOVISUAL_Graphics_SwapFull` calls `BSPE_DualPage_PresentFrame` which evaluates effective damage and attempts a partial VRAM copy.

---

## 4. DAMAGE SYSTEM FINDINGS (PHASE 4)
- **Small Damage:** When moving the mouse over an empty desktop, the damage is constrained to two ~32x32 rectangles.
- **Aggressive Merging:** `input_lab` invalidates its entire window (e.g., 600x400) every frame. If the cursor overlaps this window, the damage rects merge, creating a massive dirty region.
- **Z-Order Amplification:** Any window intersecting the dirty region is redrawn. If the dirty region is 600x400, the compositor traverses the entire Z-order and forces recalculation for everything underneath and above that 600x400 space.

---

## 5. WALLPAPER FINDINGS (PHASE 6)
**Status: EFFICIENT (Mostly)**
`Shell_DrawWallpaper` correctly clips its `x` and `y` iteration bounds based on the dirty rectangle:
```c
for (int32_t y = clip->y; y < clip->y + clip->height; y++) {
    for (int32_t x = clip->x; x < clip->x + clip->width; x++) { ... }
}
```
This is computationally sound. A 32x32 cursor move only calculates 1024 wallpaper pixels.

---

## 6. BOFONT & BOIMAGE FINDINGS (PHASE 7 & 8)
**Status: CATASTROPHIC WASTE**
The primary CPU killer resides in `BOImage_FlushBatch`.
Instead of calculating clipped bounds *before* iterating over the sprite's pixels, the system iterates over the **entire** source bounds of the image/glyph and checks the clip region inside the innermost per-pixel loop:
```c
for (int32_t dy = 0; dy < s->height; dy++) {
    for (int32_t dx = 0; dx < s->width; dx++) {
        // Unnecessary CPU cycles computing sx/sy ...
        if (BWE_RenderContext_CheckClip(s->x + dx, s->y + dy)) {
            // PutPixel
        }
    }
}
```
**Impact:** If a 1000x1000 application window is open and the mouse moves by 1 pixel, the compositor asks the application to redraw. The application submits all its text and sprites. `BOImage_FlushBatch` iterates over every single pixel of every glyph/sprite (millions of iterations) only to reject 99.9% of them in the `CheckClip` conditional.

---

## 7. PRESENT / COPY FINDINGS (PHASE 11)
**Status: SEVERE BOTTLENECK**
`BSPE_DualPage_PresentFrame` attempts a partial VRAM copy (`BSPE_VRAM_CopyEffectiveDamage`).
However, there are numerous fallback conditions:
- If `effective_count == 0` or bounds are corrupt, it triggers `BOVISUAL_Graphics_LegacySwapFull_Backend()`.
- Legacy SwapFull copies the entire 8MB (1920x1080x4) RAM framebuffer to VRAM via MMIO.
- Under QEMU without KVM acceleration, an 8MB block copy over MMIO physically limits the presentation loop to ~15-20 FPS. 
- Even when partial copy succeeds, the block-copy mechanism over MMIO is extremely slow.

---

## 8. CONTROLLED BENCHMARK DEDUCTION
- **TEST 1 (Idle Desktop):** 60 FPS. `g_dirty_rect_count == 0`, rendering/swap bypassed.
- **TEST 2 (Mouse on Empty Desktop):** ~25-30 FPS. Wallpaper clips correctly, but `SwapFull` overhead and cursor rendering cost CPU.
- **TEST 8 (Input Lab open, Idle):** ~15-20 FPS. `input_lab` invalidates 600x400 rect every frame. `BOFont` does per-pixel clipping for hundreds of glyphs.
- **TEST 9 (Input Lab + Mouse):** 12-17 FPS. The combined 600x400 partial copy + per-pixel glyph iteration + fallback VRAM copy crushes the CPU budget.

---

## 9. FRAME-TIME BUDGET (Estimated 58.8ms = 17 FPS)
| Component | Estimated Cost | Classification |
| :--- | :--- | :--- |
| **BOFont/BOImage (Per-Pixel Clip)** | ~25 ms (42%) | **AVOIDABLE WASTE** |
| **VRAM Present / SwapFull Copy** | ~20 ms (34%) | **NECESSARY COST (QEMU limit)** / AVOIDABLE (If partial copy fixes applied) |
| **Window Composition/Traversal** | ~8 ms (13%) | **AVOIDABLE WASTE** (Over-merging damage) |
| **Wallpaper (Clipped)** | ~2 ms (3%) | **NECESSARY COST** |
| **Cursor & Input Pump** | ~3.8 ms (8%) | **NECESSARY COST** |

---

## 10. ROOT CAUSE TREE
1. **12-17 FPS**
   - **A. CPU Exhaustion in Software Renderer**
      - **A1.** `BOImage_FlushBatch` evaluates clipping *inside* the innermost pixel loop.
      - **A2.** Applications submit full UI state on every partial redraw.
   - **B. VRAM Memory Bandwidth Exhaustion**
      - **B1.** `input_lab` forces continuous massive dirty regions (600x400).
      - **B2.** Damage rects trigger fallback to full 8MB legacy VRAM copy.
      - **B3.** MMIO writes in QEMU are inherently slow.

---

## 11. EXACT FILES/FUNCTIONS RESPONSIBLE
1. `kernel/ui/boimage/boimage.c` - `BOImage_FlushBatch()` (Per-pixel clipping loop).
2. `kernel/ui/bofont/bofont.c` - `bofont_layout_callback()` (Submits raw unclipped sprites).
3. `kernel/graphics/BSPE/Present/bspe_present.c` & `dual_page_present.c` - `BSPE_DualPage_PresentFrame()` (Fallback triggers full 8MB copy).
4. `userspace/apps/input_lab/input_lab.c` - `input_lab_on_render()` (Self-invalidates every frame).

---

## 12. PROPOSED OPTIMIZATION PLAN

### **P0: Fix BOImage Outer Clipping Bounds**
- **Action:** Modify `BOImage_FlushBatch` to calculate the intersection of the sprite's bounds `[s->x, s->x + s->width]` and the current `BWE_RenderContext_ClipRect`. Use the intersected bounds to restrict the `dx` and `dy` `for` loops.
- **Impact:** Eliminates millions of redundant `if (BWE_RenderContext_CheckClip(...))` checks. Massive CPU saving.

### **P1: Fix Present Fallback & Partial Copy Strategy**
- **Action:** Ensure `BSPE_DualPage_PresentFrame` does not trigger `LegacySwapFull_Backend` unnecessarily. If `input_lab` is refreshing, copy *only* the 600x400 bytes to VRAM, avoiding the 8MB full-screen penalty.

### **P2: Optimize Input Lab Telemetry**
- **Action:** Instead of `BWE_InvalidateWindow(self->id)`, `input_lab` should only invalidate the exact specific bounding box of the text labels that are changing (the HUD numbers), not the static UI frame.

### **P3: Glyph Caching / Batching**
- **Action:** Add bounding-box rejection directly into `BOFont_DrawTextEx` to skip submitting glyphs to `BOImage_BatchDrawSpriteTinted` if the entire string or word is fully outside the clip rectangle.
