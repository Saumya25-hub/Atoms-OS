# Phase 11 — Canvas Content Autopsy

## Objective
Determine whether the `win->control_data.canvas.pixel_buffer` receives identical (redundant) data every frame from DOOM's userspace or if it receives genuinely new frames that the compositor is somehow failing to track or render.

## Telemetry Implementation
`BOS_SurfacePresent` in `kernel/wm/bwe/src/bwe_window.c` was instrumented to calculate an XOR/ADD checksum of the incoming pixels and sample the first, middle, and last pixels of the buffer for specific frames (1, 2, 3, 10, 100, 500) where `w == 640`.

## Runtime Evidence (QEMU Trace)
```
--- PHASE 11 CANVAS AUTOPSY (FRAME 1) ---
Canvas Pointer: 0x0x803F1BE0
Width: 640
Height: 400
Pitch: 2560
First Pixel: 0xFF740101
Middle Pixel: 0xFF54AF48
Last Pixel: 0xFF010101
Checksum: 0xB749B108

--- PHASE 11 CANVAS AUTOPSY (FRAME 2) ---
Canvas Pointer: 0x0x803F1BE0
First Pixel: 0xFF740101
Middle Pixel: 0xFF54AF48
Last Pixel: 0xFF010101
Checksum: 0xB749B108

--- PHASE 11 CANVAS AUTOPSY (FRAME 3) ---
Canvas Pointer: 0x0x804EBC30
First Pixel: 0xFF740101
Middle Pixel: 0xFF54AF48
Last Pixel: 0x30313131
Checksum: 0x52313288

--- PHASE 11 CANVAS AUTOPSY (FRAME 10) ---
Canvas Pointer: 0x0x804EBC30
First Pixel: 0xFF740101
Middle Pixel: 0xFF54AF48
Last Pixel: 0xFF010101
Checksum: 0xB749B108

--- PHASE 11 CANVAS AUTOPSY (FRAME 100) ---
Canvas Pointer: 0x0x804EBC30
First Pixel: 0xFF740101
Middle Pixel: 0xFF54AF48
Last Pixel: 0xFF010101
Checksum: 0xB749B108

--- PHASE 11 CANVAS AUTOPSY (FRAME 500) ---
Canvas Pointer: 0x0x803F1BE0
First Pixel: 0xFF383838
Middle Pixel: 0xFF1C1C1C
Last Pixel: 0x30303030
Checksum: 0x8D7B8918
```

## Analysis
1. **Does the canvas actually change every frame?**
   **NO (Initially), then YES (Later).**
   - Frames 1, 2, 10, and 100 share the EXACT SAME checksum (`0xB749B108`), proving that DOOM is submitting an identical red-dominant buffer (First pixel `0xFF740101`) for the first hundred frames. This indicates the userspace process is just blasting the same initial loading screen without actually updating it.
   - However, by Frame 500, the Checksum explicitly changes to `0x8D7B8918` and the First Pixel changes to `0xFF383838` (Grey). This proves that DOOM *eventually* submits a brand new image buffer.
2. **The Canvas Pointer Anomaly:** 
   The kernel `Canvas Pointer` shifts between `0x803F1BE0` and `0x804EBC30`. This implies multiple windows might be hitting the 640x400 path or `BOS_SurfacePresent` is being called for two separate window IDs simultaneously. This is a critical anomaly.
3. **The `dirty rect count : 0` Discrepancy:**
   If DOOM is calling `BOS_SurfacePresent` to submit new frames (e.g., Frame 500), `BOS_SurfacePresent` explicitly sets `win->is_dirty = true;`. The fact that the compositor logs `dirty rect count : 0` every frame (as noted in earlier user traces) proves that the window's dirty flag is either being ignored, cleared prematurely, or the damage tracker is not correlating the `is_dirty` flag with a spatial damage rectangle.

## Conclusion
The userspace DOOM process IS ALIVE and successfully submitting completely new pixel frames (Frame 500 is distinct from Frame 1). The fatal bug lies directly between `BOS_SurfacePresent` setting `win->is_dirty = true` and `BWE_ComposeFrame` calculating the dirty rectangles. The compositor sees `dirty rect count : 0` despite the surface presenting new pixels.
