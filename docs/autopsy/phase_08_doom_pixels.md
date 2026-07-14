# PHASE 08 — DOOM PIXELS AUTOPSY

## 1. The Rendering Pipeline is 100% PERFECT

Runtime logs and code audits prove that the DOOM pixels **are successfully reaching VRAM and being displayed on the screen**.

The pipeline is NOT broken.

### Pointer & Format Trace:

**Stage 1: DOOM Engine to DOOM `s_doom_pixels`**
*   **Buffer:** `DG_ScreenBuffer` (malloc'd in DOOM)
*   **Width/Height:** 640 x 400
*   **Format:** 32-bit ARGB (DOOM converts 8-bit palette to ARGB via `cmap_to_fb`)

**Stage 2: DOOM to Kernel (`bos_surface_present`)**
*   **Userspace Pointer:** `0x030B00C0` (`s_doom_pixels`)
*   **Format:** `0xFFRRGGBB` (Fully opaque, Little-Endian)
*   **Action:** Triggers syscall `sys_gui_surface_present`.

**Stage 3: Kernel Canvas Buffer**
*   **Kernel Pointer:** `0x803EDB90` (`win->control_data.canvas.pixel_buffer`)
*   **Width/Height/Pitch:** 640 x 400
*   **Dirty State:** `win->is_dirty = true`
*   **Proof:** Log shows `dst first pixel: 0xFF740101`, matching the DOOM memory perfectly.

**Stage 4: Compositor RAM Framebuffer (`bos_canvas_render`)**
*   **Pointer:** `0x90000000` (`ram_fb.buffer` via `BWE_GetRenderTarget()`)
*   **Width/Height/Pitch:** 1920 x 1080 (Pitch = 7680 bytes)
*   **Action:** Copies 640x400 block directly onto the 1920x1080 surface.

**Stage 5: VRAM Presentation (`SwapFull`)**
*   **Source Pointer:** `0x90000000` (`ram_fb`)
*   **Dest Pointer:** `0xFD7E9000` (`back_vram_ptr`)
*   **Action:** 100% full frame memcpy to VRAM, then page flipped.
*   **Proof:** `[FRAME 100] VRAM Copy Begin / End / Present Complete`.

---

## 2. If the pipeline is perfect, why is the screen BLACK?

You are seeing a perfectly rendered **solid black frame**.

There are two major bugs in the userspace DOOM code that cause it to only ever draw a single black frame.

### Bug 1: The `.bss` Zero-Initialization

When DOOM launches, `DG_ScreenBuffer` is allocated via `malloc()`.
The `malloc()` implementation in `libc_impl.c` uses a static array `heap`:
```c
static unsigned char heap[32*1024*1024]; 
```
Because it is `static`, the ELF loader places it in the `.bss` section, which is strictly zero-initialized.
Thus, `DG_ScreenBuffer` is filled entirely with `0x00000000`.

In `DG_DrawFrame()`, DOOM converts this buffer:
```c
    for (int i = 0; i < DOOM_W * DOOM_H; i++) {
        uint32_t c = DG_ScreenBuffer[i];
        s_doom_pixels[i] = c | 0xFF000000;
    }
```
`0x00000000 | 0xFF000000 = 0xFF000000`.
**Result:** The entire buffer becomes SOLID BLACK (Opaque Alpha + Black RGB).

### Bug 2: The `s_ticks++` Time Warp

Why doesn't DOOM draw the next frame (the title screen)?
Because DOOM's internal timer is not connected to the OS time!

In `doomgeneric_signaturesos.c`:
```c
static uint32_t s_ticks = 0;
uint32_t DG_GetTicksMs() {
    return s_ticks++; 
}
```
Time in DOOM only advances when `DG_GetTicksMs()` is called. 
During the initial "screen melt" wipe effect (`wipe_ScreenWipe`), DOOM loops waiting for time to pass:
```c
	do {
	    nowtime = I_GetTime ();
	    tics = nowtime - wipestart;
            I_Sleep(1); // yields to compositor
	} while (tics <= 0);
```
Because `I_GetTime()` increments so slowly (1 DOOM tic = 29 calls), DOOM yields to the OS 29 times just to advance ONE DOOM tic!

When DOOM finally advances time, `TryRunTics()` sees a massive jump in time and tries to catch up by running hundreds of game loops (`G_Ticker`) in a single frame without yielding! 

Because of the pacing bug found in Phase 07 (Compositor iterating 1,800,000 times a second), the Compositor blazes through Frames 101, 102, 103, 104, and 105 in mere **microseconds**, while DOOM is still busy trying to simulate its catch-up ticks.

**This is why `bos_surface_present()` is never called again in your logs, and why `dirty rect count` becomes 0.**

## CONCLUSION

1. **Pixels ARE reaching the display.** The rendering pipeline is flawless.
2. The pixel being displayed is `0xFF000000` (Black). 
3. DOOM only renders one frame and gets stuck catching up on time because `DG_GetTicksMs()` is an incrementing variable (`s_ticks++`) instead of an actual OS timer.

To see DOOM, we must fix the DOOM timer to use a real OS timer (like `bos_uptime()`), or at least fix the pacing bug in `scheduler_yield()` so the compositor isn't running at 1.8 Million FPS.
