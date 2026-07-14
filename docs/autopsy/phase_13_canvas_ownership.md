# Phase 13 — Canvas Ownership Autopsy

## Objective
Investigate window ownership consistency between `BOS_SurfacePresent` (which receives DOOM's frames) and `compose_window_recursive` (which renders frames to the screen).

## Telemetry Evidence (QEMU Trace)

### Frame Submission Pipeline (`BOS_SurfacePresent`)
```text
--- PHASE 13 AUTOPSY: BOS_SurfacePresent ---
Window ID: 4107
Window Pointer: 0x0x178C78
Canvas Pointer: 0x0x803F1BE0
Width: 640
Height: 400
```

### Rendering Pipeline (`compose_window_recursive`)
```text
--- PHASE 13 AUTOPSY: compose_window_recursive ---
Window ID: 4108
Window Pointer: 0x0x1793A0
Canvas Pointer: 0x0x804EBC30
Width: 640
Height: 400
```

## Question
**Is the compositor rendering the EXACT SAME window whose canvas was updated by `BOS_SurfacePresent`? Or is `BOS_SurfacePresent` writing into one window while `compose_window_recursive` renders another?**

**ANSWER: THEY ARE DIFFERENT WINDOWS.**

Runtime proof clearly shows a massive disconnect:
1. `BOS_SurfacePresent` is continuously writing DOOM's frames to **Window ID 4107** (Canvas Pointer: `0x803F1BE0`).
2. `compose_window_recursive` is continuously drawing **Window ID 4108** (Canvas Pointer: `0x804EBC30`) to the screen.

Because DOOM is rendering into Window 4107 but the Window Manager is compositing Window 4108, the live pixels submitted by DOOM never reach the actual screen composition.
