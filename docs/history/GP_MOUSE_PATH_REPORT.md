# ATOMS OS — MOUSE PATH FORENSIC REPORT

## 1. Complete Path Trace: Mouse Event to Scheduler

```mermaid
sequenceDiagram
    participant Mouse as PS/2 Mouse Controller (8042)
    participant PIC as 8259 PIC
    participant IDT as IDT Vector 44 (IRQ 12)
    participant ISR as isr_common_stub
    participant Handler as mouse_irq_handler()
    participant HIDA as HIDA Event Queue
    participant BWE as BWE Event Pump
    participant BCM as BCM Damage Pipeline
    participant Sched as Scheduler & Context Manager

    Mouse->>PIC: Sends packet byte (IRQ 12)
    PIC->>IDT: Signals Vector 44
    IDT->>ISR: Enters isr44 -> isr_common_stub
    ISR->>Handler: Calls mouse_irq_handler(regs)
    Handler->>HIDA: hida_push_relative(dx, dy, buttons)
    Handler-->>ISR: Returns new_rsp = 0
    ISR-->>Sched: IRETQ restores execution context

    Note over BWE,BCM: User Task calls Syscall 22 (SYS_GUI_POLL_EVENT)
    BWE->>BCM: BCM_RequestCursorDamage(old_x, old_y, new_x, new_y)
    BCM->>BCM: Marks dirty regions & wakes bcm_compositor_thread
    Note over Sched: Next Timer Tick (IRQ 0) schedules bcm_compositor_thread
```

---

## 2. Interaction with BCM and Asynchronous Composition

1. **User Mode Polling**:
   - Applications (`desktop_shell`, `calc`) poll events via `SYS_GUI_POLL_EVENT` (Syscall 22) and yield via `SYS_YIELD` (Syscall 3).
2. **Damage Notification**:
   - `BWE_PumpEvents()` records cursor movement and submits a bounded damage request: `BCM_RequestCursorDamage()`.
3. **BCM Compositor Wakeup**:
   - `bcm_compositor_thread` (Kernel Thread, Priority 31) sleeps for bounded intervals (`scheduler_sleep(5)`).
   - When pending damage exists, `bcm_compositor_thread` wakes and executes `BCM_Process()` $\to$ `BWE_ComposeFrame()` in dedicated task context with `IF=1`.
4. **Vulnerability in Cursor State Lock**:
   - `bos_cursor_tick()` called from `timer_tick_handler()` invoked `cursor_state_unlock()`.
   - `cursor_state_unlock()` previously executed unconditional `__asm__ volatile("sti")`.
   - When the user moved the mouse during a timer tick, mouse interrupt (IRQ 12) preempted the timer tick handler, triggering a nested interrupt stack frame corruption.
