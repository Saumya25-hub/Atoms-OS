# ATOMS OS Execution Management Architecture

## Status

This document defines the first production integration boundary for the Process Manager, Thread Manager, and Scheduler. It is intentionally compatible with the existing x86-64 kernel rather than replacing the legacy `Task` scheduler in one unsafe step.

## Existing Baseline

- [`ATOMS_ProcessControlBlock`](../kernel/core/process/process_manager.h:29) is a fixed-table process registry with PID allocation, parent PID, address-space metadata, resource counters, zombie handling, and window cleanup.
- [`ATOMS_ThreadControlBlock`](../kernel/core/thread/thread_manager.h:28) is a fixed-table thread registry, currently separate from the scheduler's [`Task`](../kernel/core/scheduler/include/task.h:18) object.
- [`Task`](../kernel/core/scheduler/include/task.h:18) owns the runnable CPU execution context, kernel/user stacks, address space, queue node, and scheduler state.
- [`timer_tick_handler()`](../kernel/core/timer/src/timer.c:13) is the interrupt entry point that saves the current task context, runs scheduler policy, and restores the selected context.

## Integration Decision

The authoritative execution object remains `Task` during this phase because the boot path, user ELF loader, interrupt context ABI, and assembly switch path already depend on it. The PCB and TCB registries become ownership and observability layers around that object:

```text
Process Manager (PCB, PID, parent/child, resources)
                 |
                 +-- Thread Manager (TCB, TID, lifecycle, join metadata)
                              |
                              +-- Scheduler Task (CPU context, queue membership)
```

No subsystem may free a `Task` while it is referenced by a PCB/TCB or scheduler queue. Termination is two-phase:

1. mark dead and unlink from runnable/sleep queues;
2. reclaim stacks, extended state, Task, TCB, and PCB only after no execution reference remains.

## State Ownership

### Process

- Process Manager owns process state transitions, PID lifetime, parent/child metadata, process resource accounting, and exit/reap policy.
- Loader owns virtual-memory construction and loaded-module details.
- Window manager owns window objects but Process Manager requests cleanup for process-owned windows.

### Thread

- Thread Manager owns TID lifetime, thread state, TLS metadata, join/detach state, affinity metadata, and execution statistics.
- Scheduler owns runnable state, queue membership, quantum, CPU selection, and actual context switching.
- Context engine owns register-frame layout and stack preparation.

### Scheduler

- Scheduler owns `Task` queue membership and current CPU execution selection.
- Scheduler must not directly mutate PCB ownership fields except through explicit accounting hooks.

## Valid State Flows

Process:

```text
CREATED -> READY -> RUNNING
RUNNING -> WAITING | SLEEPING | SUSPENDED | ZOMBIE
ZOMBIE -> TERMINATED -> reclaimed
```

Thread:

```text
CREATED -> READY -> RUNNING
RUNNING -> READY | BLOCKED | SLEEPING | WAITING | SUSPENDED | DEAD
DEAD -> reclaimed
```

Invalid transitions return a structured error and emit a trace event in diagnostic builds. They do not silently overwrite state.

## Initialization Dependency Graph

```text
heap / PMM / VMM
       |
       +--> Process Manager
       |        |
       |        +--> Thread Manager
       |                 |
       |                 +--> Context Engine
       |                          |
       |                          +--> Scheduler queues
       |                                   |
       |                                   +--> Timer IRQ
       +--> loader / syscall / IPC / window cleanup hooks
```

The safe boot order is:

1. memory allocators and virtual memory;
2. Process Manager and Thread Manager tables;
3. context engine;
4. scheduler queues and idle task;
5. timer IRQ;
6. boot-task registration;
7. loader and syscall execution paths.

## Runtime Workflow / Fast Debug Path

When a process misbehaves, inspect in this order:

1. **Identity:** PID/TID exists and generation is not stale.
2. **Ownership:** TCB PID matches PCB and scheduler Task owner PID.
3. **State:** PCB state, TCB state, and Task state are compatible.
4. **Queue:** a READY task appears in exactly one ready queue; sleeping tasks appear only in the sleep queue.
5. **Context:** saved RSP is inside the task's kernel stack and CR3/PML4 matches the selected address space.
6. **Accounting:** CPU ticks advance for the selected task and its process.
7. **Cleanup:** terminated objects are not present in any queue and their resource counters reach zero.

Recommended evidence bundle for an AI/debugger:

```text
process dump
thread dump
scheduler dump
last 64 trace events
current task + saved RSP/RIP/CR3
queue membership audit
resource/leak audit
last structured error
```

## Diagnostics Contract

Every future public operation should expose:

- subsystem identifier;
- stable error code;
- severity;
- object identity (PID/TID/task ID);
- current state and requested state;
- timestamp/tick;
- recovery hint.

Diagnostics must be safe in interrupt context: no allocation, no filesystem access, and bounded output. Human-readable dumps remain supported; machine-readable snapshots should use fixed-size records and a caller-provided buffer.

## Current Limitations

- The existing scheduler has a single global ready queue and uses `Task` IDs as a legacy identity in several call sites.
- The current context C layer stores the interrupt frame pointer but does not yet perform a complete C-level register switch; the assembly ISR/context ABI remains authoritative.
- PCB and TCB tables are fixed-size and do not yet use generation-tagged handles.
- Synchronization primitives, SMP run queues, NUMA placement, and full user/kernel CPU accounting are reserved for the next implementation phase.
- The current build script explicitly enumerates every translation unit; new source files must be added there.

## Next Implementation Phase

Implement shared execution contracts first: stable state/error enums, bounded trace records, queue audit APIs, and diagnostic snapshots. Then extend PCB/TCB ownership and scheduler hooks without changing the existing user-mode context ABI.
