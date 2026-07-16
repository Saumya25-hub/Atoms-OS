# CLI Investigation Task

## Objective
Investigate the ATOMS OS codebase to prepare for Vizier X+ Engine Phase 1: Runtime Invariant & Diagnostic Snapshot Engine.

## Known Symptoms
- Phase 0 established the Vizier Core and passive registration, including a basic `vizier_dump_diagnostic_snapshot()`.
- Phase 1 requires implementing the `g_vizier_trace_ring[128]` for invariant tracking, and wiring the diagnostic snapshot to trigger on `Alt+F12` and inside `kernel_panic()`.

## Relevant Files / Subsystems
- `kernel/core/vizier/src/vizier_core.c` and `vizier.h`
- Keyboard interrupt handler (likely `kernel/drivers/input/keyboard.c` or similar)
- Kernel panic handler (likely `kernel/core/panic.c` or `kernel.c`)
- Serial output routines (`display.c` or `serial.c`)

## Questions to Answer
1. **Keyboard Hook:** Where exactly is the keyboard IRQ handled, and how can we intercept `Alt+F12` securely without interfering with regular application input?
2. **Panic Hook:** Where is `kernel_panic()` defined, and is it safe to call `vizier_dump_diagnostic_snapshot()` from within it (considering interrupts might be disabled)?
3. **Trace Ring:** Does `vizier_core.c` currently define `g_vizier_trace_ring`? If not, what is the best lockless way to implement it using static memory?
4. **Print Safety:** Are the existing `printf` / `serial_write` routines safe to call during a panic (i.e., do they use heap allocations or blocking mutexes that could deadlock)?

## Evidence Required
- Code snippets of the keyboard handler and panic function.
- Exact line numbers for the proposed hook insertions.
- Proof (code analysis) that serial printing during panic will not deadlock.

## Explicit Instructions
**DO NOT MODIFY PRODUCTION CODE UNLESS AUTHORIZED.**

BRIDGE_STATUS: PROCESSED_TASK_READY
TASK_ID: 101
