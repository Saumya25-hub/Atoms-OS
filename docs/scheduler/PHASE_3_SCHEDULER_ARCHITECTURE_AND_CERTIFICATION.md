# ATOMS OS Phase 3 — Production Scheduler Architecture and Certification

## Status

**Implementation status: PASS for build and boot smoke validation.**

This phase extends the existing scheduler without replacing the legacy `Task` execution authority. The scheduler now provides explicit lifecycle states, typed intrusive queues, priority aging, round-robin policy selection, timer-driven wakeups, CPU accounting, diagnostics, consistency auditing, and Thread/Process Manager binding hooks.

A full production certification claim remains conditional on long-duration and SMP stress evidence that is not available from the bounded local runtime smoke test.

## Architecture Overview

```text
PIT IRQ 0
   -> timer_tick_handler()
      -> context_save_state()
      -> scheduler_on_tick()
         -> wake sleepers
         -> age ready tasks
         -> account CPU/runtime
         -> select next task
         -> update TSS/CR3/FPU state
      -> context_restore_state()

Process Manager (PCB)
   owns PID, parent/child relationship, process resources, process CPU time
           |
Thread Manager (TCB)
   owns TID, lifecycle metadata, thread statistics, join/detach state
           |
Scheduler Task
   owns executable CPU context, stacks, queue membership, priority, quantum
```

The `Task` remains the CPU execution object because the existing interrupt frame and user-mode loader ABI already depend on it. TCBs bind to Tasks rather than duplicating context-switch ownership.

## State Machine

Scheduler states are defined in [`TaskState`](../kernel/core/scheduler/include/task.h:9):

```text
NEW -> READY -> RUNNING
             |      |
             |      +--> READY       (preemption/yield)
             |      +--> WAITING     (wait)
             |      +--> SLEEPING   (timed sleep)
             |      +--> BLOCKED    (resource block)
             |      +--> TERMINATED (exit)
             |
             +--> BLOCKED / WAITING / TERMINATED

SLEEPING / WAITING / BLOCKED -> READY -> RUNNING
```

Illegal transitions are rejected by [`task_transition()`](../kernel/core/scheduler/src/task.c:40) and counted in scheduler diagnostics. The idle task cannot be blocked, slept, waited, or terminated.

## Queue Architecture

The scheduler owns five typed intrusive queues implemented by [`RunQueue`](../kernel/core/scheduler/include/runqueue.h:20):

| Queue | State | Purpose |
|---|---|---|
| Ready | `TASK_READY` | Runnable non-current tasks |
| Waiting | `TASK_WAITING` | Join/IPC/event wait foundation |
| Sleeping | `TASK_SLEEPING` | Timed wakeup ordered by wake tick scan |
| Blocked | `TASK_BLOCKED` | Resource or synchronization block foundation |
| Terminated | `TASK_TERMINATED` | Deferred reclamation by idle task |

Queue insertion is checked for duplicate membership, wrong-queue membership, invalid list metadata, and intrusive-list corruption. [`runqueue_validate()`](../kernel/core/scheduler/src/runqueue.c:128) verifies links, tail/head invariants, size, and task queue ownership.

## Scheduling Algorithms

[`SchedulerPolicy`](../kernel/core/scheduler/include/scheduler.h:13) supports:

1. **Priority aging** — highest effective priority runs first; equal priorities use oldest ready timestamp and then task ID for deterministic tie-breaking.
2. **Round robin** — FIFO ready queue rotation.
3. **Idle scheduling** — the immortal idle task is selected when no runnable work exists.

Priority is bounded to 32 levels. Aging increases effective priority after [`SCHEDULER_AGING_INTERVAL_TICKS`](../kernel/core/scheduler/include/scheduler.h:10) while never exceeding the maximum. Quantum defaults to `1 + priority / 4`, preserving short slices for low priorities and longer slices for high priorities.

## Context Switching

The existing x86-64 context ABI is preserved. The timer path saves the interrupt frame, C policy selects a task, and the ISR assembly restores the selected stack frame. Scheduler policy does not enter assembly. TSS kernel stack updates and address-space switching remain explicit in the dispatch path.

FPU/extended state is delegated to the existing CPU state hooks. Full lazy FPU policy and XSAVE feature negotiation remain a future hardening item.

## Timer Integration

[`timer_tick_handler()`](../kernel/core/timer/src/timer.c:13) advances the timer, saves the active context, calls [`scheduler_on_tick()`](../kernel/core/scheduler/src/scheduler.c:503), and restores the selected task. Each tick performs:

- current-task CPU accounting;
- sleeper expiration and wakeup;
- ready-queue wait accounting and priority aging;
- quantum decrement;
- preemption/dispatch decision;
- TSS, CR3, and extended-state handoff.

## Process and Thread Integration

The Thread Manager exposes scheduled creation helpers:

- [`ATOMS_Thread_CreateKernel()`](../kernel/core/thread/thread_manager.c:133)
- [`ATOMS_Thread_CreateUser()`](../kernel/core/thread/thread_manager.c:133)

These create a TCB, create the corresponding scheduler Task, assign process ownership, apply priority, bind the TCB to the Task, and populate stack metadata. Process CPU accounting is updated from the current Task owner PID.

Thread suspend/resume/sleep/yield paths now request scheduler queue operations instead of only mutating TCB state.

## Diagnostics and Debugging

The public [`SchedulerDiagnostics`](../kernel/core/scheduler/include/scheduler.h:24) snapshot includes:

- ticks and context switches;
- voluntary/involuntary switches;
- idle and busy ticks;
- queue lengths for all queues;
- wakeups, aging boosts, average wait, and longest runtime task;
- current task/process and quantum usage;
- duplicate, corruption, illegal-transition, and dead-task counters;
- CPU utilization in hundredths of a percent;
- consistency status and active policy.

Debug entry points:

- [`scheduler_dump_runtime_diagnostics()`](../kernel/core/scheduler/src/scheduler.c:865)
- [`scheduler_dump_queues()`](../kernel/core/scheduler/src/scheduler.c:916)
- [`scheduler_dump_tasks()`](../kernel/core/scheduler/src/scheduler.c:939)
- [`scheduler_dump_task_info()`](../kernel/core/scheduler/src/scheduler.c:967)
- [`scheduler_validate_consistency()`](../kernel/core/scheduler/src/scheduler.c:809)

## Certification Tests

The existing execution certification report was extended with scheduler checks in [`ATOMS_Execution_RunCertification()`](../kernel/debug/execution_certification.c:16):

- scheduler initialization and idle task validity;
- policy switching between round robin and priority aging;
- queue/current-task diagnostic consistency;
- queue consistency audit;
- idle stack and priority contract;
- priority-level and aging interval contract;
- diagnostics counter coherence;
- existing process lifecycle, thread lifecycle, transition validation, and leak checks.

The complete build invokes these sources through [`build.ps1`](../build.ps1:419), and the resulting kernel image is generated by the existing image pipeline.

## Verification Evidence

### Build

The full [`build.ps1`](../build.ps1:1) pipeline completed successfully:

- kernel compilation and linking completed;
- userspace ELFs completed;
- raw image generated;
- boot signature, kernel offset, sector count, and image alignment validated;
- VDI, VMDK, and VMX artifacts generated.

The only observed messages were pre-existing warnings from DOOM pointer casting, an empty libc loop body, and Microsoft CRT `fopen` deprecations. No scheduler compilation, link, or undefined-symbol errors were reported.

### Runtime smoke test

A QEMU single-vCPU boot smoke test reached:

- `[BWE_INFO] ATOMS Process Manager initialized`;
- `[SCHED] Production scheduler initialized`;
- `TMR & AME OK`;
- loader certification tests;
- IPC certification tests;
- display/BWE initialization and desktop shell startup.

No scheduler panic, context fault, page fault, general-protection fault, queue audit failure, or scheduler error was observed in the captured runtime log before the bounded test stopped.

## Known Limitations

1. The available runtime evidence is a bounded smoke test, not a 24-hour stability proof.
2. SMP remains a compatibility design target; per-core run queues, inter-processor reschedule IPIs, load balancing, and CPU migration are not implemented.
3. Full FPU/XSAVE lazy-state management is not yet certified.
4. Waiting and blocked queues are implemented as lifecycle/queue foundations; event-specific synchronization wake reasons remain subsystem-owned.
5. The scheduler currently scans sleeper and ready queues linearly; large-scale 10,000-task latency benchmarking is not yet demonstrated.
6. Deferred task reclamation relies on idle execution and must not reclaim an object still externally referenced by a future generalized reaper.
7. The existing interrupt/context ABI must remain synchronized with [`isr_stubs.asm`](../arch/x86_64/interrupt/isr_stubs.asm:83) and [`context_switch.asm`](../kernel/core/scheduler/src/context_switch.asm:8).

## Future Roadmap

### Phase 3 hardening

- Add a dedicated scheduler stress harness for 100, 1,000, and 10,000 task metadata operations.
- Add deterministic simulated-tick unit tests for quantum, aging, sleep, wake, block, resume, and termination.
- Add queue poisoning/canary validation and explicit dead-task reference audits.
- Add long-duration QEMU watchdog execution and leak snapshots.

### Phase 4 / Ring 3

- Complete syscall-driven thread/process creation through the TCB-to-Task binding API.
- Enforce process address-space ownership and user/kernel stack separation at every dispatch.
- Add scheduler-aware IPC wait and wake contracts.

### SMP

- Introduce per-CPU scheduler state and idle tasks.
- Add CPU affinity enforcement, migration, load balancing, and reschedule IPIs.
- Make queue and diagnostics ownership CPU-local with global aggregation.

### Production hardening

- Add XSAVE/XRSTOR capability negotiation and lazy FPU policy.
- Add priority inheritance for synchronization primitives.
- Add real-time deadline policy and CPU-budget enforcement.
- Add formal state/queue invariant tests and fuzzed transition sequences.

## Certification Decision

**Phase 3 implementation: build-certified and boot-smoke-certified.**

**Phase 3 production certification: conditional / not yet final**, because 24-hour runtime stability, 10,000-task stress, race audit, and SMP execution evidence require dedicated test infrastructure beyond the completed bounded verification. The codebase is structurally prepared for those tests and for the Ring 3 handoff, but those claims should not be marked fully certified without the corresponding runtime evidence.
