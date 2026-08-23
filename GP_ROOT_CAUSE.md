# ATOMS OS — POST-PHASE-5 #GP ROOT CAUSE & RESOLUTION

## 1. Absolute Root Cause

The post-Phase-5 `#GP(0x6E90)` fault occurred during mouse movement due to **premature interrupt unmasking inside the Hardware Timer ISR**:

1. In [`kernel/drivers/input/cursor/cursor_state.c`](file:///d:/Signatures_OS/kernel/drivers/input/cursor/cursor_state.c), the locking functions were implemented as:
   ```c
   static inline void cursor_state_lock(void) {
       __asm__ volatile("cli" : : : "memory");
   }

   static inline void cursor_state_unlock(void) {
       __asm__ volatile("sti" : : : "memory");
   }
   ```
2. In [`kernel/core/timer/src/timer.c`](file:///d:/Signatures_OS/kernel/core/timer/src/timer.c#L40-L43), `timer_tick_handler()` (the 1000 Hz hardware IRQ 0 handler) called:
   ```c
   /* 1000Hz Hardware IRQ Cursor Animation Tick (AppStarting / Wait Spinner) */
   extern void bos_cursor_tick(void);
   bos_cursor_tick();
   ```
3. `bos_cursor_tick()` queried and modified cursor state, invoking `cursor_state_unlock()`.
4. `cursor_state_unlock()` executed an unconditional **`sti`**, setting `RFLAGS.IF = 1` inside the Hardware Timer ISR.
5. While `timer_tick_handler()`, `scheduler_on_tick()`, and `BRE_DispatchPending()` were running with `IF=1`, incoming PS/2 Mouse hardware interrupts (IRQ 12 / Vector 44) or subsequent timer ticks interrupted the ISR in-flight.
6. A nested interrupt frame was pushed onto the task's kernel stack on top of the in-progress ISR frame.
7. Context saving recorded the top-of-stack from the nested interrupt frame, causing the stack frame alignment to become corrupted.
8. When the interrupted task was subsequently restored, `isr_common_stub` popped registers from the misaligned stack, and `iretq` attempted to pop a stack address offset (`0x6E90` / `0x9F00`) into `SS` / `CS`.
9. The CPU raised `#GP` with error code `0x6E90`.

---

## 2. Production Architectural Fix

1. **Flags-Preserving Locking**: Replaced the unconditional `cli` / `sti` pair in [`kernel/drivers/input/cursor/cursor_state.c`](file:///d:/Signatures_OS/kernel/drivers/input/cursor/cursor_state.c) with canonical `irq_save()` and `irq_restore(flags)` using `pushfq; pop %0; cli` and `push %0; popfq`.
2. If `cursor_state` is accessed from within an ISR (`IF=0`), `irq_restore()` keeps `IF=0`.
3. If `cursor_state` is accessed from task context (`IF=1`), `irq_restore()` restores `IF=1`.

---

## 3. Forensic Validation & Verification Results

1. **Reproduction Test**: Successfully reproduced the exact `#GP` fault using `scratch/reproduce_phase5_gp.py` prior to the patch.
2. **Post-Patch Verification**: Built cleanly and executed complete automated test suite with `scratch/reproduce_phase5_gp.py`:
   - Rapid mouse sweeps over taskbar: **PASS**
   - Window movement and drag: **PASS**
   - Application launches (Explorer, Terminal, Calculator): **PASS**
   - Continuous rapid clicks and arithmetic calculations: **PASS**
   - Total Fault Count: **0**
   - Syscall Loop (`SYS_GUI_POLL_EVENT` + `SYS_YIELD`): **100% STABLE**
