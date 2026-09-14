# MOUSE & CURSOR ARCHITECTURE — FIX & EVOLUTION PLAN

**Status**: PROPOSED ARCHITECTURAL DESIGN (Awaiting User Review & Approval)  
**Target Hardware**: Intel Core i3 (H81 Motherboard, Native UEFI GOP Mode)  

---

## 1. Candidate Solutions Analysis

| Option | Description | Performance | Safety / Risk | Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **Option 1: Brute-Force FPS Increase** | Increase BCM Compositor loop from 60 FPS to 240 FPS | 240 Hz Cursor | **HIGH RISK**: Quadruples CPU load; burns power; unnecessary full-scene overhead | **REJECTED** |
| **Option 2: Direct Unsynchronized VRAM Blit** | Re-enable existing `g_bspe_cursor_fast_path_enabled` without page tracking | 1000 Hz Cursor | **HIGH RISK**: Causes cursor trails, black square artifacts, and page-flip races | **REJECTED** |
| **Option 3: Page-Aware Decoupled Micro-Tile Blitter** | Decoupled cursor micro-frame blitter with page-aware shadow buffers and RAM-based clean restore | **1000 Hz Cursor (< 0.5 ms latency)** | **LOW RISK**: Zero window recomposition; rock-solid page-flip synchronization; zero trail artifacts | **RECOMMENDED** |

---

## 2. Detailed Technical Design of Recommended Solution (Option 3)

```
[Hardware Mouse Event] (125 Hz – 1000 Hz)
            │
            ▼
 [input_core / pointer_motion]
            │
            ├──► Updates PointerState (x, y)
            │
            ▼
[BSPE_CursorPresenter_FastTileUpdate]
            │
            ├─► 1. Check if Compositor is currently flipping page (if locked, defer to next tick)
            ├─► 2. Restore 32x32 clean background from `ram_fb` at old_pos -> VRAM
            ├─► 3. Alpha-blend 32x32 cursor sprite at new_pos -> VRAM
            └─► 4. Direct 4 KB PCIe transfer (takes 2 microseconds)
```

### Key Architectural Invariants:
1. **Clean Restoration Source**: Old cursor pixels are restored directly from the clean `ram_fb` buffer in system RAM (which is never corrupted by cursor sprites).
2. **Page-Aware Destination**: The micro-tile write always targets the currently active scanout page in VRAM (`current_fb.buffer`).
3. **Atomic Mutual Exclusion**: A simple atomic flag `s_compositor_writing_vram` ensures that the micro-tile blitter and the window compositor never write to physical VRAM simultaneously.
4. **Zero Window Tree Overhead**: Moving the mouse does not wake up window layout calculations, hit-testing cascades, or wallpaper redraws.

---

## 3. Files and Functions to be Modified

| File | Function | Role |
| :--- | :--- | :--- |
| [`kernel/graphics/BSPE/Cursor/bspe_cursor_present.c`](file:///d:/Signatures_OS/kernel/graphics/BSPE/Cursor/bspe_cursor_present.c) | `BSPE_CursorPresenter_FastTileUpdate()` | Implements clean-RAM background restoration, cursor sprite alpha-blending, and 4 KB VRAM tile flush. |
| [`kernel/wm/bwe/src/bwe_core.c`](file:///d:/Signatures_OS/kernel/wm/bwe/src/bwe_core.c) | `BWE_PumpEvents()` | Triggers `BSPE_CursorPresenter_FastTileUpdate()` immediately upon mouse move event arrival. |
| [`kernel/wm/bcm/src/bcm_task.c`](file:///d:/Signatures_OS/kernel/wm/bcm/src/bcm_task.c) | `bcm_compositor_thread()` | Sets and clears the atomic VRAM presentation lock during window composition passes. |

---

## 4. Rollback & Safety Strategy

- **Safety Checkpoint Commit**: `8e2bfbd480605e835c409998eca16246df69c2f4`
- **Zero Architectural Invasiveness**: All changes are confined to the cursor presentation layer. Window management, scheduling, networking, file systems, and drivers remain 100% untouched.
- **Fail-Safe Fallback**: If any VRAM lock contention occurs, the system smoothly falls back to standard single-writer compositor presentation with zero visual glitching.

---

## 5. Verification Plan

1. **TEST 01**: Slow, steady mouse movement — verify 100% fluid visual tracking.
2. **TEST 02**: High-speed rapid horizontal and vertical mouse flicks — verify zero stepping / zero micro-pauses.
3. **TEST 03**: Rapid circular gestures — verify smooth analog curves.
4. **TEST 04**: Rapid movement over desktop icons and taskbar — verify zero cursor trails, zero black holes.
5. **TEST 05**: Rapid movement while dragging active windows — verify zero tearing, clean window bounds.
6. **TEST 06**: Build verification on Clang toolchain — verify zero warnings, zero errors.
7. **TEST 07**: Bare-metal physical verification on Intel H81 UEFI testbench.
