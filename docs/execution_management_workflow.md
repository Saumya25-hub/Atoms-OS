# ATOMS OS Execution Management — Brain / Workflow

## Purpose

This document is the operational map for the integrated Process Manager, Thread Manager, and Scheduler. It is written for fast onboarding, deterministic debugging, and future AI-assisted maintenance.

## Source Layout

```text
kernel/core/execution/include/execution_contract.h   shared errors, traces, snapshots
kernel/core/execution/src/execution_contract.c       bounded runtime diagnostics
kernel/core/process/process_manager.h                PCB and process API
kernel/core/process/process_manager.c                PID, lifecycle, ownership, resources
kernel/core/thread/thread_manager.h                  TCB and thread API
kernel/core/thread/thread_manager.c                  TID and thread lifecycle
kernel/core/scheduler/include/task.h                 CPU execution object
kernel/core/scheduler/include/scheduler.h            scheduler public API
kernel/core/scheduler/src/scheduler.c                queues and policy
kernel/core/timer/src/timer.c                        preemption entry point
```

## Ownership Rules

| Object | Owner | Never do |
|---|---|---|
| PCB | Process Manager | free while child/thread references exist |
| TCB | Thread Manager | mutate scheduler queue directly |
| Task | Scheduler | free before queue unlink and context retirement |
| kernel stack | Task lifecycle | free from an active task |
| process address space | Loader/VM | switch CR3 without scheduler coordination |
| trace record | Execution Diagnostics | allocate or print from IRQ context |

The scheduler `Task` remains the CPU execution authority for compatibility with the existing interrupt and user-mode loader ABI. The TCB wraps its identity, lifecycle, statistics, TLS, affinity, and join metadata. A PCB owns the TCB IDs and process-level resource accounting.

## Initialization Flow

```text
heap -> PMM/VMM -> Process Manager -> Thread Manager -> Context Engine
     -> Scheduler queues/idle -> timer IRQ -> boot task -> loader/syscalls
```

The kernel currently initializes scheduler and timer in [`kernel_main()`](../kernel/kernel.c:991). Process/Thread manager initialization must occur before any loader path can create a PCB or TCB. Execution diagnostics must be initialized before state transitions are traced.

## State-Machine Contract

Process states:

```text
CREATED -> READY -> RUNNING
RUNNING -> READY | WAITING | SLEEPING | SUSPENDED | ZOMBIE | TERMINATED
WAITING/SLEEPING -> READY | SUSPENDED | TERMINATED
SUSPENDED -> READY | TERMINATED
ZOMBIE -> TERMINATED/CLOSED
```

Thread states:

```text
CREATED -> READY -> RUNNING
RUNNING -> READY | BLOCKED | SLEEPING | WAITING | SUSPENDED | DEAD
BLOCKED/SLEEPING/WAITING -> READY | SUSPENDED | DEAD
SUSPENDED -> READY | DEAD
DEAD -> FREE
```

All state changes must use the manager transition API. An invalid transition is an error, a diagnostic counter increment, and a structured trace record—not a silent assignment.

## Runtime Workflow

### Process creation

1. Validate caller and name.
2. Allocate a non-colliding PID from the configured range.
3. Initialize PCB security, memory, handles, environment, modules, resource counters, and creation tick.
4. Link the child to its parent.
5. Transition `CREATED -> READY`.
6. Create/register its initial thread.
7. Submit the associated Task to the scheduler ready queue.
8. Emit process-create and thread-create events.

### Scheduling

1. Timer IRQ increments the timer tick.
2. The interrupt path saves the current Task context.
3. Scheduler accounts CPU time and decrements the quantum.
4. Expired sleepers are moved to READY.
5. Priority and round-robin policy selects the next Task.
6. Old Task is requeued only if still runnable.
7. New Task becomes RUNNING; TSS stack and address space are updated.
8. Context restore returns through the established ISR ABI.

### Blocking / waiting

1. Validate ownership and current state.
2. Remove Task from the ready queue.
3. Transition TCB/PCB to BLOCKED, WAITING, or SLEEPING.
4. Record wake condition/deadline in scheduling metadata.
5. Select another Task; idle Task is the fallback.
6. Wakeup transitions to READY and requeues exactly once.

### Exit / cleanup

1. Mark the thread kill-pending and stop future scheduling.
2. Unlink it from every scheduler queue.
3. Account final CPU time and record exit code.
4. Decrement the PCB thread/resource counters.
5. When the last thread exits, terminate the process and close owned windows/handles/modules.
6. Preserve a ZOMBIE only when a live parent can reap it.
7. Reparent children to PID 1 when necessary.
8. Reap only after no scheduler, thread, or parent reference remains.
9. Free stacks, extended state, Task, TCB, and PCB in that order.

## Fast Debug Workflow

Use this sequence; do not start by guessing at the context switch assembly.

1. **Identity:** confirm PID/TID/task ID and generation.
2. **Cross-link:** PCB thread list contains TID; TCB PID matches PCB; TCB Task owner PID matches PCB.
3. **State:** compare PCB, TCB, and Task states against the state machine.
4. **Queue:** READY means exactly one ready-queue membership; SLEEPING means exactly one sleep-queue membership; RUNNING means current Task.
5. **Context:** saved RSP is inside the kernel stack; RIP is a valid known entry/trampoline; CR3/PML4 belongs to the selected process.
6. **Timer:** ticks advance and the timer handler reaches scheduler policy.
7. **Accounting:** current Task, TCB, and PCB CPU counters advance consistently.
8. **Cleanup:** no DEAD/TERMINATED object remains in a queue; resource counters reach zero.

### Evidence bundle

```text
process dump
thread dump
scheduler runtime dump
execution trace ring (latest 64 records)
current Task: id/state/owner_pid/rsp/rip/pml4
queue membership audit
process/thread manager diagnostics
last structured error and recovery hint
```

## Structured Errors

Every public operation returns a stable [`ATOMS_ExecutionErrorCode`](../kernel/core/execution/include/execution_contract.h:14) where practical. A diagnostic record carries module, code, severity, object ID, owner ID, current/requested state, timestamp, cause, and recovery guidance.

## Machine-Readable Diagnostics

[`ATOMS_Execution_TraceCopy()`](../kernel/core/execution/include/execution_contract.h:91) copies a bounded fixed-size ring without heap allocation. [`ATOMS_Execution_GetDiagnostics()`](../kernel/core/execution/include/execution_contract.h:94) reports capacities, active objects, context switches, ticks, and errors. Human-readable dumps are secondary views of the same ownership state.

## Certification Matrix

| Test | Required observation |
|---|---|
| Process create | unique PID, parent link, READY state, creation trace |
| Thread create | unique TID, PCB registration, READY state |
| Yield | current Task remains valid; next ready Task selected |
| Priority | higher effective priority wins; equal priority rotates |
| Sleep/wakeup | no early wake; exactly one requeue |
| Context switch | saved/restored RSP and current Task identity agree |
| Termination | queues unlink; exit trace; no active execution reference |
| Wait/reap | parent validation; exit code preserved; zombie disappears |
| Leak detection | dead object resource totals are zero |
| 1,000 processes | allocator collision-free until configured capacity |
| 10,000 threads | allocator/table behavior deterministic; failed creates are structured |
| Long runtime | counters do not regress or wrap unexpectedly |

## Known Limitations

- The current build uses a manually enumerated source list in [`build.ps1`](../build.ps1:419); new execution source files must be added explicitly.
- The legacy scheduler uses one global ready queue; per-priority and per-CPU queues are the next scheduler step.
- The current Task context ABI remains authoritative; complete register-save/restore changes require coordinated assembly and interrupt testing.
- Synchronization objects, SMP, NUMA, ring-3 signals, and generation-tagged external handles are reserved for the next phase.

## Next Development Step

Finish the TCB implementation and bind every created TCB to its scheduler Task. Then add scheduler accounting/diagnostic hooks and initialization calls, followed by build certification and stress tests.
