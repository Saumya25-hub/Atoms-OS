# Phase 07: Desktop Main Loop Pacing Autopsy

## Problem
Runtime evidence showed the Desktop main loop executing at an impossibly high rate (700,000 to 1,800,000+ iterations per second), consuming 100% CPU and starving other processes. A properly paced GUI running at ~60fps should only loop ~60 times per second (or slightly more if polling input, but never millions).

## The Pipeline Trace

1. **kernel main loop** (`kernel.c:746`)
2. `input_adapter_pump()`
3. `BWE_PumpEvents()`
4. **Pacing Check**:
   ```c
   uint64_t elapsed = current_ticks - last_frame_ticks;
   if (elapsed < 15) {
     if (15 - elapsed > 2) {
       scheduler_yield();
     }
     continue;
   }
   ```
5. `BOHeart_Pulse(hw_fb)`
6. `AGDTE_Pulse()`
7. Loop repeats

## The Root Cause

When `elapsed < 15`, the kernel correctly calls `scheduler_yield()` to wait for the next frame.

Inside `scheduler.c:277`:
```c
void scheduler_yield(void) {
    if (!current_task || current_task == idle_task_ptr) return;

    // Ping-pong prevention: enforce minimum 1 tick gap
    if (current_task->last_run_tick == scheduler_tick_count) {
        return; // Reject immediate re-yield in the same tick
    }
    current_task->last_run_tick = scheduler_tick_count;

    // Voluntarily clear the quantum to force a switch on the next tick
    current_task->quantum = 0;
    
    // Wait for the hardware timer to perform the safe context switch
    __asm__ volatile("sti");
    __asm__ volatile("hlt" : : : "memory");
}
```

### The Sequence of Failure

1. `kernel.c` calls `scheduler_yield()`.
2. `last_run_tick` is updated to `scheduler_tick_count`.
3. `hlt` is executed. The CPU sleeps.
4. An **unrelated interrupt** fires (e.g., PS/2 Mouse, Serial COM1, or Keyboard).
5. The CPU wakes up, processes the interrupt, and returns to `scheduler_yield()`.
6. `scheduler_yield()` finishes and returns to the main loop.
7. The main loop checks `elapsed < 15`. It is still true (the hardware timer hasn't ticked yet).
8. It calls `scheduler_yield()` **again**.
9. The "Ping-pong prevention" logic evaluates:
   ```c
   if (current_task->last_run_tick == scheduler_tick_count) {
       return; // Reject immediate re-yield in the same tick
   }
   ```
10. This is **TRUE** because the timer tick never fired, so `scheduler_tick_count` never incremented!
11. `scheduler_yield()` returns immediately without executing `hlt`.
12. The `while (1)` loop continues immediately.
13. Steps 7-12 repeat millions of times, creating an unthrottled busy-wait loop until the next timer tick.

## Verdict: FAIL
The pacing mechanism completely fails if any non-timer hardware interrupt wakes the CPU. The ping-pong prevention in `scheduler_yield` turns a well-intentioned `hlt` pause into a catastrophic 100% CPU busy-wait spin loop.
