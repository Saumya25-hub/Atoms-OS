# ATOMS OS — PHASE 5 REGRESSION ANALYSIS

## 1. Architectural Baseline Comparison

| Dimension | Pre-Phase 5 (Synchronous BWE) | Post-Phase 5 (BCM Asynchronous) |
| :--- | :--- | :--- |
| **Composition Context** | `timer_tick_handler()` (IRQ 0 ISR, `IF=0`) | `bcm_compositor_thread` (Task Context, `IF=1`) |
| **Task Preemption** | Low frequency during GUI interaction | High frequency: Task switching between userspace and compositor thread (Priority 31) |
| **Sleep / Yield Frequency**| Rare in compositor | 100-200 calls/sec via `scheduler_sleep(5)` & `scheduler_yield()` |
| **Interrupt Interleaving** | Heavy rendering inside ISR blocked all interrupts | Rendering runs with `IF=1`; interrupts (Timer IRQ 0, Mouse IRQ 12) fire concurrently |

---

## 2. Why Phase 5 Uncovered the Cursor State STI Defect

1. **Before Phase 5**:
   - `BWE_Compose()` was monopolizing the CPU inside `timer_tick_handler()` with `IF=0` for ~3-5ms.
   - Interrupts were completely masked during that entire multi-millisecond block.
   - Preemption and task switching between a dedicated high-priority worker and usermode tasks did not occur at high frequency.
2. **After Phase 5**:
   - `bcm_compositor_thread` runs with `IF=1`, yielding and sleeping frequently.
   - Usermode tasks (`desktop_shell`, `calc`) run and yield via `SYS_YIELD` $\to$ `scheduler_yield()`.
   - The Timer ISR (`timer_tick_handler`) runs at 1000 Hz, firing while tasks are halted in `sti; hlt`.
   - `timer_tick_handler` called `bos_cursor_tick()` $\to$ `cursor_state_unlock()` which executed `sti`.
   - Because `IF=1` was improperly enabled inside the ISR, every mouse movement during a tick fired IRQ 12 nested inside IRQ 0, corrupting the execution stack and triggering `#GP` on `iretq`.
