# Phase 12 — BOS_CANVAS_RENDER Forensic Autopsy

## Objective
Trace `bos_canvas_render` instruction-by-instruction to determine if it successfully modifies the RAM framebuffer, and if so, whether the pixels survive until `SwapBuffers`.

## Telemetry Evidence (QEMU Trace)
```
--- PHASE 12 BOS_CANVAS_RENDER FORENSIC ---
Source Pointer: 0x0x804EBC30
Destination Pointer (RAM FB): 0x0x90000000
Width: 640
Height: 400
Pitch: 7680
Clip Rect: X=100 Y=100 W=640 H=400
Before copy - Destination first pixel: 0x0xFF080E0F
...
After copy - Destination first pixel: 0x0xFF740101
After copy - Destination middle pixel: 0x0xFF54AF48
After copy - Destination last pixel: 0x0xFFAF95A1
...
Immediately before SwapBuffers - RAM FB first pixel: 0x0xCC000000
Immediately before SwapBuffers - RAM FB middle pixel: 0x0xFF54AF48
Immediately before SwapBuffers - RAM FB last pixel: 0x0xFFAF95A1
...
Immediately after SwapBuffers - VRAM first pixel: 0x0xCC000000
Immediately after SwapBuffers - VRAM middle pixel: 0x0xFF54AF48
Immediately after SwapBuffers - VRAM last pixel: 0x0xFFAF95A1
--- END PHASE 12 FORENSIC ---
```

## Question 1
**Does `bos_canvas_render` actually modify the RAM framebuffer?**
**YES**
Runtime proof shows the first destination pixel changing from `0xFF080E0F` (before copy) to `0xFF740101` (after copy). The DOOM pixels are successfully copied to the RAM framebuffer.

## Question 2
**Do the modified pixels survive until SwapBuffers?**
**NO** (Partial Overwrite)
While the middle and last pixels of the DOOM window successfully survived (`0xFF54AF48` and `0xFFAF95A1`), the **first pixel** was overwritten. It changed from `0xFF740101` to `0xCC000000` just before `SwapBuffers` was called.

## Question 3
**Identify the exact instruction that overwrites or clears them.**
The instruction responsible for the overwrite is in `kernel/wm/bwe/renderer/bwe_compositor.c`:

```c
// Inside draw_diagnostics_hud() which is called immediately after window composition
BWE_FillRect(fb, hud_rect.x, hud_rect.y, hud_rect.width, hud_rect.height, 0xCC000000); // Semitransparent black panel
```
The Diagnostic HUD draws a 360x320 rectangle at `X=10, Y=10` using `0xCC000000` (semi-transparent black). Because the DOOM window is positioned at `X=100, Y=100` (and its client area starts at `X=105, Y=135` due to the window border offset), the top-left region of the DOOM window is completely obliterated by the HUD's background fill.

As instructed, I have stopped immediately after identifying this first overwrite.
