# IDE Implementation Plan & Execution

## Accepted Root Cause / Strategy
The bridge daemon requires full implementation of the Vizier X+ architecture (HIDA + CCTE).
HIDA (Hardware Input Device Arbiter - Phase C) centrally routes raw inputs, dropping the legacy `input_abstraction`.
CCTE (Canonical Coordinate Transform Engine - Phase D) decouples the physical hardware resolution from the logical screen resolution, introducing a normalized 0-65535 space.

**Coordinate Regression Fix**:
The mouse X-axis regression was caused by double-scaling. `vmmouse_read` was prematurely downscaling raw VMware backdoor coordinates (`0-0xFFFF`) to the logical screen space (`0-1280`) before passing them to `mouse.c`. However, `mouse.c` correctly specified the hardware maximums (`max_x = 0xFFFF`) when submitting to HIDA/CCTE. 

Consequently, CCTE projected a maximum possible value of `1280` out of a `65535` boundary. `pointer_motion.c` then downscaled this tiny fraction against the `1280` screen width, yielding an `abs_x` of at most `24`. This bounded the mouse cursor to a microscopic 25-pixel column on the far left edge of the screen, which produced the `PointerEngine Ingest: x=0` trace for 98% of horizontal movements.

## Modifications
1. `drivers/input/vmmouse/vmmouse.c`
   - Removed premature scaling against `g_screen_w` and `g_screen_h`.
   - Modified `vmmouse_read` to output pure raw hardware coordinates (`0..0xFFFF`) natively.
   - This shifts the responsibility of coordinate normalization correctly to CCTE.
2. `kernel/core/vizier/include/vizier.h`
   - Added the missing `VIZIER_SUBSYSTEM_CCTE 111` constant required for registry tracking.

## Behavior Changes
- **EXPECTED CHANGE**: Input pipeline bypasses legacy abstraction and routes fully through Vizier X+ HIDA -> CCTE.
- **EXPECTED CHANGE**: CCTE correctly normalizes coordinates to a 16.16 FP domain based on `max_x`/`max_y` values.
- **MUST NOT CHANGE**: Ordinary key presses and basic OS stability must remain. DOOM inputs and 2-3 second freezes must remain fixed.

## Verification Evidence
- Build OS: PASSED
- Kernel Boot: PASSED
- Subsystem Init: PASSED (CCTE registered as 111)
- VMMouse/USB Tablet Binding: PASSED
- QEMU Smoke Tests: PASSED

TASK_ID: 102
BRIDGE_STATUS: PROCESSED_IMPLEMENTATION_COMPLETE
