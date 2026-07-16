# IDE Implementation Plan

## Accepted Root Cause / Strategy
The CLI findings correctly identify the need for a non-blocking, lockless structure for the Vizier Trace Ring (`g_vizier_trace_ring`). `Alt+F12` inside the IRQ 1 handler (`keyboard.c`) and the four exception paths in `exception.c` are safe hooks because `vizier_dump_diagnostic_snapshot()` utilizes `display_print()`, which is verified to be allocation-free and lockless. 

## Files to Modify
1. `kernel/core/vizier/src/vizier_core.c`
2. `kernel/drivers/keyboard/src/keyboard.c`
3. `kernel/core/interrupt/src/exception.c`

## Behavior Changes
- **EXPECTED CHANGE**: Pressing `Alt+F12` will trigger an immediate diagnostic dump of the Vizier subsystem registry to the screen/serial output without crashing the OS.
- **EXPECTED CHANGE**: A kernel panic (GPF, Page Fault, Assert, or generic exception) will automatically dump the Vizier registry and the trace ring contents right before halting.
- **MUST NOT CHANGE**: The routing of ordinary key presses must remain exactly as before. Existing panic dumps must not be disrupted. The registry must remain passive and authoritative over exactly nothing in Phase 1.

## Rollback Point
A clean Git commit representing Phase 0 baseline will be used if Phase 1 needs to be reverted.

## Verification Invariants
1. `g_vizier_trace_ring` uses `__atomic_fetch_add` or similar lockless arithmetic.
2. No heap allocations (`kmalloc`) occur in the snapshot dump.
4. Existing passive subsystems register successfully.

## Verification Evidence
- Build OS: PASSED
- Kernel Boot: PASSED
- Subsystem Init (Phase 0 Check): PASSED
- QEMU Smoke Tests: PASSED
- Diff Verification: PASSED (Zero unauthorized regressions touched)

TASK_ID: 101
BRIDGE_STATUS: IMPLEMENTATION_COMPLETE
