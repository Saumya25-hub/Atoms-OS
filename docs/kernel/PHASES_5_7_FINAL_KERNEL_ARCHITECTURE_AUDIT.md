# ATOMS OS Final Kernel Completion — Initial Architecture Audit for Phases 5–7

## 1. Audit scope and evidence policy

This document is the initial evidence-based architecture audit for the final-kernel work after the Phase 4 Ring 3 foundation. It covers:

- Phase 5: production syscall ABI, dispatch, validation, safe-copy, and thread-local entry state;
- Phase 6: CPU discovery, SMP startup, APIC/IOAPIC, per-CPU scheduling, synchronization, and IPIs;
- Phase 7: hardening, diagnostics, profiling, watchdogs, stress, and release verification.

No production code was changed. Source inspection, existing documentation, existing build scripts, and existing captured logs are the only evidence used. No new build or QEMU run was performed as part of this audit.

### 1.1 Status vocabulary

| Status | Meaning in this audit |
|---|---|
| **PASS** | The stated source property or captured runtime/build observation was directly inspected and supports the narrowly worded claim. |
| **FAIL** | Direct source evidence shows that a required completion property is absent, unsafe, contradictory, or non-functional. |
| **NOT VERIFIED** | The source may contain a foundation, script, log, or intended behavior, but the required runtime property was not directly demonstrated by sufficiently specific evidence. This is not treated as a pass. |

A prior document saying that something passed is not by itself runtime proof for this audit. The Phase 4 report explicitly limits its claims: it calls the foundation integrated but final certification conditional and denies proof of real CPL 3 execution, successful Ring 3 syscall return, SMP, long-duration operation, and complete runtime certification in [`PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md`](../usermode/PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md:3), [`PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md`](../usermode/PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md:186), and [`PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md`](../usermode/PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md:211).

## 2. Executive decision

### 2.1 Current mission status

| Area | Status | Evidence-based decision |
|---|---|---|
| Phase 3 Process/Thread/Scheduler foundation | **PASS** for source foundation | PCB, TCB, Task binding, lifecycle queues, accounting, diagnostics, TSS stack handoff, and CR3 switching are present. Runtime and SMP certification remain bounded. |
| Phase 4 Ring 3 foundation | **PASS** for source foundation | User selectors, TSS, user mappings, guard pages, exception attribution, user-copy helpers, and a user-task frame builder exist. |
| Proven CPL 3 application execution and return | **NOT VERIFIED** | No inspected log contains an unambiguous CPL 3 marker plus user RIP/RSP, syscall entry, syscall return, and user exit sequence. |
| Phase 5 production syscall boundary | **FAIL** | User pointers are directly dereferenced throughout the dispatcher; entry state uses a global scratch slot; return-state validation is absent; ABI and dispatch inconsistencies exist. |
| Phase 6 SMP execution | **FAIL** | No ACPI/MADT enumeration, AP startup, local APIC, IOAPIC, IPI, per-CPU TSS/current task/run queues, or AP scheduler path was found. |
| QEMU launch with 1/2/4 configured vCPUs | **PASS** for script availability | Existing scripts pass the requested vCPU count and capture serial output. |
| Actual 2/4-CPU kernel operation | **NOT VERIFIED** | Existing multi-vCPU logs only prove that QEMU launched and the BSP reached boot/scheduler markers; they do not prove AP startup or concurrent scheduling. |
| Phase 7 kernel hardening foundation | **PASS** in limited areas | Heap canaries/deadlock detection, scheduler consistency checks, execution traces, user exception counters, and subsystem heartbeats exist. |
| Kernel-wide watchdog/profiler/race certification | **FAIL** | There is no integrated kernel watchdog, lock dependency checker, IRQ-off latency tracker, per-CPU profiler, or deterministic SMP race suite. |
| Long-duration stability | **NOT VERIFIED** | A script named “long” sleeps for only 60 seconds, and no acceptance parser or long-duration kernel evidence was inspected. |
| Current build in this audit session | **NOT VERIFIED** | The build pipeline was inspected but not executed during this audit. |

### 2.2 Blocking conclusion

The codebase is suitable for a staged Phase 5 implementation that preserves the Phase 4 execution objects, but it is not ready for a Phase 6 SMP claim or a Phase 7 final-certification claim. Phase 5 must close the syscall and address-space boundary before SMP can safely multiply those paths. Phase 6 must establish CPU-local ownership before any multi-vCPU boot log can be interpreted as SMP evidence. Phase 7 must provide machine-verifiable evidence rather than relying on script names, elapsed sleeps, or generic boot markers.

## 3. Current-state architecture evidence

## 3.1 Process Manager, Thread Manager, and scheduler

### PASS — ownership model exists and should be preserved

- The executable CPU context remains in [`Task`](../../kernel/core/scheduler/include/task.h:30), including stack, saved RSP/RIP, address space, user flag, priority, affinity mask, accounting, owner PID, and intrusive queue node.
- The TCB contains thread identity, user/kernel stack metadata, register metadata, scheduling metadata, affinity, TLS base, statistics, diagnostics, and a bound Task pointer in [`ATOMS_ThreadControlBlock`](../../kernel/core/thread/thread_manager.h:78).
- User process construction creates one Task, one kernel stack, an IRET frame, a TCB binding, PCB user-image metadata, and scheduler submission in [`process_spawn()`](../../kernel/core/process/src/process.c:12).
- Process termination marks lifecycle state, terminates owned Tasks, closes owned surfaces, and chooses zombie or immediate reap behavior in [`ATOMS_Process_Terminate()`](../../kernel/core/process/process_manager.c:225).
- Process accounting and leak metadata exist in [`ATOMS_Process_AccountCPU()`](../../kernel/core/process/process_manager.c:382) and [`ATOMS_Process_AuditLeaks()`](../../kernel/core/process/process_manager.c:404).
- Scheduler tick processing performs accounting, sleeper wakeup, aging, quantum handling, task selection, extended-state handoff, TSS RSP0 update, and CR3 switch in [`scheduler_on_tick()`](../../kernel/core/scheduler/src/scheduler.c:606).
- Queue/current-task consistency checks exist in [`scheduler_validate_consistency()`](../../kernel/core/scheduler/src/scheduler.c:777).
- The Phase 3 report correctly describes the Task as execution authority and the TCB/PCB as management ownership in [`PHASE_3_SCHEDULER_ARCHITECTURE_AND_CERTIFICATION.md`](../scheduler/PHASE_3_SCHEDULER_ARCHITECTURE_AND_CERTIFICATION.md:25).

### FAIL — current ownership is single-CPU global state

- The scheduler owns global queues, a global current Task, one idle Task, one tick counter, and one diagnostic object in [`scheduler.c`](../../kernel/core/scheduler/src/scheduler.c:24).
- The public global [`current_task`](../../kernel/core/scheduler/include/scheduler.h:98) makes current execution state non-CPU-local.
- The Task affinity field exists in [`Task`](../../kernel/core/scheduler/include/task.h:51), but inspected scheduling selection does not enforce a CPU identity or affinity eligibility in [`select_next_task()`](../../kernel/core/scheduler/src/scheduler.c:237).
- The TCB has last-CPU and affinity metadata in [`ATOMS_ThreadSchedulingInfo`](../../kernel/core/thread/thread_manager.h:55), but no CPU-local dispatch ownership was found.
- Process and thread table critical sections primarily disable local interrupts. This prevents same-CPU interrupt interleaving but is not a general SMP lock. Examples include [`ATOMS_Process_Terminate()`](../../kernel/core/process/process_manager.c:225), [`ATOMS_Process_RegisterThread()`](../../kernel/core/process/process_manager.c:336), and the execution trace ring in [`ATOMS_Execution_Trace()`](../../kernel/core/execution/src/execution_contract.c:45).

### NOT VERIFIED — runtime scheduler certification

The source includes [`ATOMS_Execution_RunCertification()`](../../kernel/debug/execution_certification.c:15), but the inspected boot initialization does not invoke it. The boot path initializes execution/process/thread/scheduler/user mode in [`kernel.c`](../../kernel/kernel.c:995), while repository search found no runtime call to the execution or user-mode certification functions. Therefore compilation of those objects is a PASS, but their current runtime report is NOT VERIFIED.

## 3.2 Ring 3 and address-space foundation

### PASS — privilege and mapping foundations exist

- The GDT installs DPL 3 data and code descriptors and a TSS in [`gdt_init()`](../../arch/x86_64/gdt/gdt.c:40).
- TSS RSP0 can be changed on task dispatch using [`tss_set_kernel_stack()`](../../arch/x86_64/gdt/gdt.c:77).
- User address checks reject zero size, noncanonical addresses, low addresses, overflow, and addresses above the configured user maximum in [`ATOMS_UserMode_IsUserRange()`](../../kernel/core/usermode/user_mode.c:43).
- Per-page validation checks presence, user permission, write permission, and NX/execute policy in [`vmm_validate_user_range()`](../../kernel/core/memory/vmm/src/vmm.c:264).
- Safe-copy helper foundations exist in [`ATOMS_UserMode_CopyFromUser()`](../../kernel/core/usermode/user_mode.c:60) and [`ATOMS_UserMode_CopyToUser()`](../../kernel/core/usermode/user_mode.c:76).
- User-origin exception attribution records vector, RIP, RSP, flags, fault address, address space, PID, and TID before task/process termination in [`ATOMS_UserMode_HandleException()`](../../kernel/core/usermode/user_mode.c:107).
- User process creation constructs selectors and an IRET frame in [`process_spawn()`](../../kernel/core/process/src/process.c:66).

### FAIL — isolation is not yet a production boundary

- New process address spaces mark upper paging levels user-accessible and copy broad low-address kernel mappings, including kernel globals, heap, framebuffer, and APIC-related ranges, in [`vmm_create_address_space()`](../../kernel/core/memory/vmm/src/vmm.c:335). This matches the Phase 4 report’s explicit warning in [`PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md`](../usermode/PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md:58).
- The heap is mapped with user permission in [`heap_init()`](../../kernel/core/memory/heap/src/heap.c:136). Until page-walk permission behavior is fully audited and corrected, kernel heap isolation cannot pass.
- Address-space destruction recursively frees user-marked leaves and intermediate tables in [`vmm_destroy_address_space()`](../../kernel/core/memory/vmm/src/vmm.c:402), but copied/shared mapping ownership is not explicitly represented. Destruction safety across shared kernel structures is therefore NOT VERIFIED.
- The standalone transition path clears IF and contains an obsolete statement that no TSS exists in [`enter_usermode`](../../kernel/core/scheduler/src/enter_usermode.asm:10), while the current GDT does provide a TSS. This path is inconsistent with the user-task IRET frame, which enables interrupts in [`process_spawn()`](../../kernel/core/process/src/process.c:70). A single authoritative transition contract is required.

### NOT VERIFIED — actual Ring 3 execution

The inspected existing logs show ELF planning/loading and scheduler registration, for example [`audit_smp2.log`](../../audit_smp2.log:328) and [`audit_smp2.log`](../../audit_smp2.log:401), but they do not emit CPL, CS, user RIP/RSP, syscall entry/exit, or a user-process exit certificate. “Spawned” is not treated as proof of execution at CPL 3.

## 4. Phase 5 audit — syscall ABI, dispatch, validation, safe copy, and thread-local state

## 4.1 Existing syscall ABI

### PASS — basic ABI foundations

- Syscall IDs 0–42 and return constants are centrally listed in [`syscall.h`](../../kernel/core/syscall/include/syscall.h:6).
- The dispatcher rejects IDs at or above the declared maximum in [`syscall_handler()`](../../kernel/core/syscall/src/syscall.c:25).
- STAR, LSTAR, and FMASK setup and a SYSRET return path exist in [`syscall_init_asm`](../../kernel/core/syscall/src/syscall_entry.asm:18) and [`syscall_entry`](../../kernel/core/syscall/src/syscall_entry.asm:77).
- Legacy vector 128 is retained for compatibility in [`syscall_init()`](../../kernel/core/syscall/src/syscall.c:351).
- Userspace wrappers use the conventional x86-64 syscall register convention in [`syscalls.c`](../../userspace/libbos/src/syscalls.c:37) and GUI wrappers in [`syscalls_gui.c`](../../userspace/libbos_gui/src/syscalls_gui.c:17).

### FAIL — ABI contradictions and unreachable behavior

- The dispatcher rejects every ID at or above 43 before entering the switch, making the case for ID 300 unreachable in [`syscall_handler()`](../../kernel/core/syscall/src/syscall.c:25).
- The assembly comments and register shuffle describe six syscall arguments, but the C declaration accepts only five arguments after the ID in [`syscall_entry.asm`](../../kernel/core/syscall/src/syscall_entry.asm:95) and [`syscall.h`](../../kernel/core/syscall/include/syscall.h:64). A versioned, mechanically shared ABI definition is absent.
- Several IDs declared in [`syscall.h`](../../kernel/core/syscall/include/syscall.h:43), including GUI textbox, bounds, and destroy operations, have no inspected switch implementation and fall through to not implemented.
- The legacy vector remaps IDs 1–3, which overlaps the current ID table and creates a second semantic ABI in [`syscall_dispatcher_legacy()`](../../kernel/core/syscall/src/syscall.c:334). Kernel-only compatibility must be explicitly separated from user ABI dispatch.
- There is a second, unrelated gateway with its own syscall namespace and dispatcher in [`ATOMS_Syscall_Dispatch()`](../../kernel/core/syscall/syscall_gateway.c:14). It is compiled by [`build.ps1`](../../build.ps1:436), but it is not the Ring 3 syscall authority. Parallel gateways are an ownership and audit risk.

## 4.2 Pointer validation and safe copy

### PASS — helpers exist

The VMM and user-mode layers provide suitable primitives for a centralized policy: [`vmm_validate_user_range()`](../../kernel/core/memory/vmm/src/vmm.c:264), [`ATOMS_UserMode_ValidateAddress()`](../../kernel/core/usermode/user_mode.c:51), [`ATOMS_UserMode_CopyFromUser()`](../../kernel/core/usermode/user_mode.c:60), and [`ATOMS_UserMode_CopyToUser()`](../../kernel/core/usermode/user_mode.c:76).

### FAIL — the active syscall dispatcher bypasses the helpers

Direct user pointer use is widespread in [`syscall_handler()`](../../kernel/core/syscall/src/syscall.c:25):

- strings are directly read by write, open, spawn, mkdir, create, rename, delete, and GUI text paths at [`syscall.c`](../../kernel/core/syscall/src/syscall.c:39), [`syscall.c`](../../kernel/core/syscall/src/syscall.c:80), [`syscall.c`](../../kernel/core/syscall/src/syscall.c:96), and [`syscall.c`](../../kernel/core/syscall/src/syscall.c:218);
- read, readdir, keyboard/input events, GUI events, and filesystem writes pass user output/input buffers directly to kernel subsystems at [`syscall.c`](../../kernel/core/syscall/src/syscall.c:87), [`syscall.c`](../../kernel/core/syscall/src/syscall.c:137), [`syscall.c`](../../kernel/core/syscall/src/syscall.c:158), and [`syscall.c`](../../kernel/core/syscall/src/syscall.c:313);
- GUI extended argument arrays are directly dereferenced at [`syscall.c`](../../kernel/core/syscall/src/syscall.c:266) and [`syscall.c`](../../kernel/core/syscall/src/syscall.c:290);
- surface pixels are directly consumed at [`syscall.c`](../../kernel/core/syscall/src/syscall.c:323).

No central descriptor table states argument count, scalar width, pointer direction, maximum size, string limit, capability requirement, or copy policy. Therefore syscall pointer safety is FAIL even though helper primitives pass source inspection.

## 4.3 Entry and return state

### FAIL — global scratch and unsafe return

- User RSP is first written to one global slot before switching stacks in [`syscall_entry`](../../kernel/core/syscall/src/syscall_entry.asm:77). This is unsafe under interrupt nesting, reentry, and SMP.
- There is no swap to CPU-local GS state, no CPU-local syscall stack/frame pointer, and no per-thread syscall frame.
- The path returns through SYSRET without validating canonical user RIP and RSP, user selectors, forbidden RFLAGS bits, or whether the current Task was terminated/replaced while inside the syscall in [`syscall_entry.asm`](../../kernel/core/syscall/src/syscall_entry.asm:132).
- The active dispatcher may yield or terminate execution from inside a syscall in [`syscall.c`](../../kernel/core/syscall/src/syscall.c:35) and [`syscall.c`](../../kernel/core/syscall/src/syscall.c:64), but the entry frame has no explicit restart/termination contract.
- FMASK only clears IF in [`syscall_init_asm`](../../kernel/core/syscall/src/syscall_entry.asm:56); a final policy should also explicitly address direction, trap, alignment-check, and nested-task-related flags as applicable.

### Required Phase 5 design

1. Define one versioned public syscall ABI and one kernel dispatch metadata table.
2. Introduce a concrete syscall frame capturing user RIP, RSP, RFLAGS, syscall number, arguments, return value, CPU, PID, TID, nesting state, and termination state.
3. Store entry state in CPU-local memory and bind the active frame to the current Task/TCB.
4. Route every user pointer through bounded copy-in/copy-out helpers; never pass raw user pointers to VFS, GUI, input, or process subsystems.
5. Validate return state and use an IRET fallback for unsafe SYSRET state.
6. Separate legacy kernel compatibility from the Ring 3 ABI.
7. Emit deterministic syscall entry/exit/rejection counters and certification markers.

## 5. Phase 6 audit — CPU, SMP, APIC, per-CPU scheduling, synchronization, and IPIs

## 5.1 CPU discovery and startup

### PASS — basic CPU feature detection only

CPUID feature detection for FPU, FXSR, SSE, XSAVE, and AVX exists in [`cpu_features_init()`](../../arch/x86_64/cpu/cpu_features.c:15). This is a CPU capability foundation, not SMP discovery.

### FAIL — SMP platform foundation absent

Repository source search found no kernel implementation of ACPI RSDP discovery, MADT parsing, processor enumeration, local APIC setup, x2APIC, IOAPIC routing, AP trampoline, INIT-SIPI-SIPI, AP online acknowledgement, or IPI sending. The interrupt subsystem is explicitly PIC-based in [`irq_init()`](../../kernel/core/interrupt/src/irq.c:8), registers legacy IRQ lines, and sends PIC EOI in [`irq_dispatch()`](../../kernel/core/interrupt/src/irq.c:30).

The GDT and TSS are singleton globals in [`gdt.c`](../../arch/x86_64/gdt/gdt.c:8), not per CPU. The syscall MSRs are initialized only through the BSP boot call in [`kernel.c`](../../kernel/kernel.c:938); MSRs are CPU-local and would need AP initialization.

## 5.2 Per-CPU scheduler and IPI model

### FAIL

- Current Task, idle Task, queues, counters, pending switch reason, and diagnostics are singleton scheduler globals in [`scheduler.c`](../../kernel/core/scheduler/src/scheduler.c:24).
- TSS stack update uses one TSS in [`tss_set_kernel_stack()`](../../arch/x86_64/gdt/gdt.c:77).
- Timer scheduling is driven by the legacy PIT path and a single scheduler tick in [`timer_tick_handler()`](../../kernel/core/timer/src/timer.c:13).
- No reschedule IPI, TLB shootdown IPI, stop/panic IPI, call-function IPI, migration queue, load balancer, or per-CPU idle Task exists.
- Affinity metadata exists but is not enough to pass CPU-affinity behavior.

## 5.3 Synchronization

### PASS — isolated primitives exist

- Heap allocation has an atomic spinlock, recursion detection, owner diagnostics, pause loop, and deadlock timeout in [`heap_lock()`](../../kernel/core/memory/heap/src/heap.c:457).
- IPC supplies atomic spinlock, busy-wait mutex, reader-writer lock, and event primitives in [`ipc_sync.c`](../../kernel/ipc/sync/ipc_sync.c:16).
- Execution error counters use atomic operations in [`ATOMS_Execution_RecordError()`](../../kernel/core/execution/src/execution_contract.c:88).

### FAIL — no kernel-wide SMP synchronization contract

- IPC mutexes spin instead of blocking and set owner to a constant kernel context in [`ipc_mutex_lock()`](../../kernel/ipc/sync/ipc_sync.c:51).
- There is no common lock type defining IRQ-safe, preempt-safe, recursive, owner-CPU, rank/order, contention, and timeout semantics.
- Scheduler queues, PCB/TCB tables, VMM page-table mutations, global TSS, syscall entry scratch, device state, and many diagnostics are not protected by an SMP-wide ownership policy.
- Local interrupt disable is repeatedly used as if it were a global lock; it is not sufficient after AP startup.
- No lock dependency checker, race injector, atomic memory-order policy, or documented interrupt-context lock hierarchy was found.

## 5.4 Existing 1/2/4-vCPU evidence

### PASS — launch scripts exist

- [`test_canary_smp1.ps1`](../../test_canary_smp1.ps1:1) launches one vCPU and captures serial output.
- [`test_canary_smp2.ps1`](../../test_canary_smp2.ps1:1) launches two configured vCPUs.
- [`test_canary_smp4.ps1`](../../test_canary_smp4.ps1:1) launches four configured vCPUs.

### NOT VERIFIED — true SMP

The existing two-vCPU log reaches scheduler registration in [`audit_smp2.log`](../../audit_smp2.log:401), and the repository contains similarly named two/four-vCPU logs. None of the inspected evidence contains discovered CPU count, APIC IDs, AP startup acknowledgements, per-CPU idle entry, per-CPU timer ticks, cross-CPU task dispatch, IPI send/receive, migration, or TLB shootdown evidence. QEMU accepting a multi-vCPU argument while only the BSP runs does not constitute SMP.

Eight CPUs are **not currently supported by inspected architecture or test scripts**. An 8-CPU gate must not be added until 4-CPU operation passes and platform limits are explicit.

## 6. Phase 7 audit — hardening, diagnostics, profiling, watchdog, and stress

## 6.1 Existing hardening and diagnostics

### PASS — usable foundations

- Heap header/rear canaries and metadata corruption diagnostics exist in [`heap.c`](../../kernel/core/memory/heap/src/heap.c:30) and [`verify_block_canaries()`](../../kernel/core/memory/heap/src/heap.c:305).
- Scheduler diagnostics include ticks, switches, utilization, wait metrics, queue counts, policy, and corruption counters in [`scheduler_get_diagnostics()`](../../kernel/core/scheduler/src/scheduler.c:799).
- Process leak-resource metadata is inspected by [`ATOMS_Process_AuditLeaks()`](../../kernel/core/process/process_manager.c:404).
- A bounded execution trace ring and error counters exist in [`execution_contract.c`](../../kernel/core/execution/src/execution_contract.c:12).
- User exception and privilege diagnostics exist in [`user_mode.c`](../../kernel/core/usermode/user_mode.c:107).
- Vizier supports subsystem heartbeat timestamps, violation counts, deadline misses, and a trace ring in [`vizier_record_heartbeat()`](../../kernel/core/vizier/src/vizier_core.c:102), [`vizier_report_violation()`](../../kernel/core/vizier/src/vizier_core.c:131), and [`vizier_check_deadlines_on_tick()`](../../kernel/core/vizier/src/vizier_core.c:160).
- A 1,000-cycle PCB/TCB metadata stress loop exists in [`ATOMS_BOSX_Run1000LaunchExitStressTest()`](../../kernel/core/loader/bosx_stress.c:15).

### FAIL — foundations are not final-kernel certification infrastructure

- Vizier deadline checks record degradation but do not constitute a kernel liveness watchdog, NMI watchdog, CPU heartbeat watchdog, forced diagnostic dump, or reset/recovery policy.
- No kernel-wide stack canary/guard-page checker for every Task kernel stack was found.
- No IRQ-off duration measurement, preemption-off duration measurement, scheduler latency histogram, syscall latency histogram, per-CPU utilization, migration statistics, lock contention profile, or IPI latency profile was found.
- No general lock dependency/order validator or SMP race detector was found.
- The process stress in [`bosx_stress.c`](../../kernel/core/loader/bosx_stress.c:15) creates PCB/TCB metadata but does not execute 1,000 real Ring 3 processes, perform syscalls, switch address spaces, fault, exit, and verify teardown.
- The boot path invokes the Phase 10 stress suite before later execution-manager initialization in [`kernel.c`](../../kernel/kernel.c:902), and that suite reinitializes Process and Thread Managers in [`ATOMS_RunPhase10_VerificationSuite()`](../../kernel/core/loader/bosx_stress.c:92). This is not a substitute for post-boot scheduler/Ring 3 stress.
- Kernel compilation disables compiler stack protection in the primary compile flags in [`build.ps1`](../../build.ps1:24). That may remain necessary for freestanding code, but it means final stack protection must be explicitly designed rather than assumed.

## 6.2 Existing runtime evidence boundaries

### PASS — bounded historical observations only

- [`audit_smp2.log`](../../audit_smp2.log:1) is direct evidence that a QEMU configuration labelled for two vCPUs reached GDT/IDT/PIC, memory, ELF-load, scheduler registration, context-save/restore, and a desktop loop on at least the BSP.
- [`phase4_certification_final.log`](../../phase4_certification_final.log:1) directly contains security and sandbox test output, but it ends before Ring 3/scheduler certification and is not evidence for this mission’s runtime gates.

### NOT VERIFIED

- No inspected log proves the Phase 4 user-mode certification function ran.
- No inspected log proves the execution certification function ran.
- No inspected log proves a successful Ring 3 syscall and return.
- No inspected log proves AP execution.
- No inspected log proves 1,000 real user-process lifecycles.
- No inspected log proves hours-long or overnight stability.
- No inspected log proves zero leaks under final Phase 5–7 workloads.

### FAIL — existing “long” mechanism is not long-duration certification

[`test_canary_smp2_long.ps1`](../../test_canary_smp2_long.ps1:1) runs for 60 seconds and force-stops QEMU. It has no log parser, heartbeat gate, fail-marker scan, exit-code protocol, periodic resource snapshots, or acceptance summary. Its name must not be interpreted as evidence.

## 7. Compatibility constraints preserving Phase 4 foundations

The following constraints are mandatory for Phases 5–7:

1. **Preserve Task as executable context authority.** Extend [`Task`](../../kernel/core/scheduler/include/task.h:30) or bind CPU-local metadata to it; do not create a second independent register/context owner.
2. **Preserve PCB/TCB identity and lifecycle ownership.** Continue using [`ATOMS_ProcessControlBlock`](../../kernel/core/process/process_manager.h:55) and [`ATOMS_ThreadControlBlock`](../../kernel/core/thread/thread_manager.h:78) for process/thread metadata and bind every executable thread to exactly one Task.
3. **Preserve current Task state numeric ABI values 0–4.** They are explicitly marked ABI-stable in [`TaskState`](../../kernel/core/scheduler/include/task.h:9).
4. **Preserve the existing x86-64 interrupt-frame contract until assembly and C definitions are changed atomically.** Coordinate [`isr_stubs.asm`](../../arch/x86_64/interrupt/isr_stubs.asm:83), [`context_switch.asm`](../../kernel/core/scheduler/src/context_switch.asm:8), and process frame creation in [`process_spawn()`](../../kernel/core/process/src/process.c:66).
5. **Preserve user selector ordering required by SYSRET.** User data precedes user code in [`gdt_init()`](../../arch/x86_64/gdt/gdt.c:54).
6. **Preserve legacy INT 0x80 only as an explicitly scoped compatibility path.** It must not bypass Ring 3 policy or silently remap the production ABI.
7. **Preserve user virtual-address constants and guard-page behavior unless a versioned ABI migration is introduced.** Current validation and stack contracts are documented in [`PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md`](../usermode/PHASE_4_RING3_ARCHITECTURE_AND_CERTIFICATION.md:42).
8. **Do not weaken exception containment.** User faults must remain process-scoped while kernel faults retain panic diagnostics through [`ATOMS_UserMode_HandleException()`](../../kernel/core/usermode/user_mode.c:107).
9. **Do not infer privilege from a boolean alone in certification.** Runtime evidence must read saved CS/CPL or equivalent hardware state; [`ATOMS_UserMode_CurrentPrivilege()`](../../kernel/core/usermode/user_mode.c:31) currently reports the Task flag rather than reading CPL.
10. **Keep BSP/PIC fallback until APIC routing is proven.** APIC introduction must have a reversible single-CPU path and must not remove the known one-CPU boot path prematurely.
11. **No AP scheduling before CPU-local GDT/TSS, syscall MSRs, current Task, idle Task, and interrupt state are initialized.**
12. **No final isolation claim while broad user-accessible kernel mappings remain.**

## 8. Prioritized Phase 5–7 implementation sequence

## Phase 5 — secure and certify the uniprocessor user/kernel boundary

### P5.1 Freeze and specify one syscall ABI

- Generate or define one authoritative syscall-number and metadata table shared by kernel and userspace.
- Record argument count/type/direction/maximum, blocking behavior, privilege/capability policy, and return semantics.
- Remove or isolate unreachable ID 300 behavior and reconcile declared-but-unimplemented IDs.
- Define legacy INT 0x80 as kernel compatibility only or route it through exactly the same validated metadata.

**Exit gate:** compile-time table checks prove unique IDs, matching maximum, matching wrappers, and implementation coverage.

### P5.2 Introduce a real syscall frame and CPU-local entry substrate

- Add a stable assembly/C syscall-frame layout.
- Add BSP CPU-local storage first, even before SMP, so the entry ABI does not need redesign in Phase 6.
- Save all required user state before touching shared memory.
- Bind PID/TID/Task and entry CPU to each frame.
- Add nesting/reentry checks and terminated-task return handling.

**Exit gate:** assembly-offset tests and runtime entry/exit frame dumps agree; no global user-RSP scratch remains.

### P5.3 Centralize validation and safe copy

- Implement bounded string copy, scalar structure copy, array/range copy, and chunked large-buffer transfer.
- Convert all pointer-bearing syscalls before enabling untrusted user workloads.
- Copy event and directory outputs to kernel temporaries before copy-out.
- Validate width/height/stride multiplication and total byte limits for surface submission.
- Return consistent errors for bad IDs, null pointers, noncanonical pointers, overflow, unmapped pages, wrong permissions, and partial-copy faults.

**Exit gate:** every pointer-bearing metadata row maps to a tested copy policy; direct raw user-pointer subsystem calls are zero.

### P5.4 Harden return and address-space isolation

- Validate user RIP/RSP canonicality and user-range membership before SYSRET.
- Sanitize RFLAGS and add an IRET fallback.
- Correct upper-level and leaf user permissions so kernel code/data/heap/device mappings are inaccessible from CPL 3.
- Define shared-kernel page-table ownership and safe address-space teardown.
- Add kernel/user stack guard pages and stack canaries.

**Exit gate:** deliberate user reads/writes/executes against kernel/null/guard/NX/RO mappings terminate only the user process; teardown returns memory to baseline.

### P5.5 Produce explicit Ring 3 certification

- Launch a minimal deterministic user ELF.
- Emit hardware-derived CPL/CS and user RIP/RSP markers.
- Execute scalar, copy-in, copy-out, bad-pointer, bad-ID, yield/sleep, spawn/exit, and fault tests.
- Prove syscall entry and return to the same user thread.
- Run 100 and 1,000 real user-process lifecycle tests with resource snapshots.

**Exit gate:** one machine-parsed report distinguishes every PASS/FAIL/NOT VERIFIED item and fails closed on missing markers.

## Phase 6 — introduce SMP without changing the user ABI

### P6.1 Platform discovery and APIC foundation

- Add ACPI RSDP/XSDT/RSDT and MADT parsing with checksums and bounds validation.
- Enumerate enabled processors, local APIC IDs, IOAPICs, and interrupt-source overrides.
- Implement local APIC enable/EOI/timer and IOAPIC redirection while retaining PIC fallback.
- Emit topology and routing diagnostics.

**Exit gate:** one-CPU QEMU passes in PIC fallback and APIC mode; discovered CPU count and APIC IDs match QEMU configuration.

### P6.2 CPU-local architecture and AP startup

- Define a fixed-size CPU-local structure containing CPU/APIC ID, online state, GDT/TSS, syscall entry state, current/idle Task, scheduler queues or queue pointer, tick/accounting, interrupt nesting, and IPI counters.
- Establish GS-based access and initialize BSP local state.
- Implement a low-memory AP trampoline and INIT-SIPI-SIPI sequence.
- Initialize per-AP GDT/TSS/IDT, syscall MSRs, FPU state, local APIC timer, idle Task, and CPU-local stack before marking online.

**Exit gate:** 1/2/4 CPU logs show unique APIC IDs and every configured CPU entering its own idle loop. Eight CPUs remain out of scope until explicitly supported.

### P6.3 SMP-safe scheduler and synchronization

- Choose and document per-CPU run queues with ordered locking and global aggregation.
- Enforce affinity at selection and migration.
- Add remote enqueue, reschedule IPI, idle wakeup, periodic or demand load balancing, and migration diagnostics.
- Replace local-IRQ-only protection of shared PCB/TCB/VMM/scheduler structures with common IRQ-safe spinlocks or blocking primitives as context permits.
- Add lock rank/order assertions and contention counters.

**Exit gate:** cross-CPU runnable work wakes idle CPUs; affinity and migration tests pass; no Task is simultaneously current or queued on multiple CPUs.

### P6.4 TLB, teardown, and stop coordination

- Add TLB shootdown generation/acknowledgement and address-space CPU residency tracking.
- Add stop/panic IPIs and coherent multi-CPU diagnostic snapshots.
- Make process/thread teardown wait for remote references and active syscall frames.

**Exit gate:** concurrent mapping/unmapping and process exit tests pass on 2/4 CPUs with complete IPI acknowledgements and no stale translations.

## Phase 7 — hardening, observability, stress, and release gates

### P7.1 Kernel-wide invariants and watchdogs

- Add per-CPU heartbeat and scheduler-progress watchdogs.
- Add kernel-stack guard/canary validation on switch and termination.
- Add IRQ-off/preempt-off duration tracking and thresholds.
- Add lock-order, ownership, recursion, timeout, and held-lock-on-schedule checks.
- Add panic-stop coordination and complete per-CPU crash snapshots.

### P7.2 Profiling and diagnostics

- Add syscall count/error/latency histograms per ID and CPU.
- Add scheduler wake-to-run latency, run-queue depth, migrations, steals, IPI latency, idle/busy time, and starvation metrics.
- Add page-fault, copy fault, TLB shootdown, lock contention, and allocation high-water metrics.
- Export snapshots through a bounded debug interface without exposing privileged memory to userspace.

### P7.3 Deterministic stress and fault injection

- Add syscall malformed-input fuzz cases and page-boundary copy cases.
- Add process/thread create/exit/fault races.
- Add scheduler transition, affinity, migration, and remote wakeup races.
- Add VMM map/unmap/teardown races and TLB shootdown timeout injection.
- Add lock contention and forced preemption tests.
- Run 1/2/4 CPU matrices; add 8 only after a declared supported-CPU limit and 4-CPU pass.

### P7.4 Long-duration and final certification

- Replace sleep-only scripts with parameterized harnesses, heartbeat monitoring, timeout classification, log parsing, and machine-readable summaries.
- Capture periodic memory, Task, queue, CPU, lock, syscall, IPI, and error snapshots.
- Treat missing markers, QEMU early exit, timeout, panic, stale heartbeat, leak growth, queue corruption, lost IPI, and AP offline state as failures.
- Produce a final certification document containing exact artifact hashes, command lines, CPU matrix, duration, result parser output, and known limitations.

## 9. Proposed ownership and file map

The entries below are proposed; they do not imply current implementation.

| Ownership domain | Existing files to preserve/modify | Proposed files | Primary responsibility |
|---|---|---|---|
| Public syscall ABI | [`syscall.h`](../../kernel/core/syscall/include/syscall.h), [`syscalls.c`](../../userspace/libbos/src/syscalls.c), [`syscalls_gui.c`](../../userspace/libbos_gui/src/syscalls_gui.c) | [`syscall_abi.h`](../../kernel/core/syscall/include/syscall_abi.h), [`syscall_abi.h`](../../userspace/libbos/include/syscall_abi.h), or generated shared header | Version, IDs, argument schema, error contract |
| Syscall entry/frame | [`syscall_entry.asm`](../../kernel/core/syscall/src/syscall_entry.asm), [`syscall.c`](../../kernel/core/syscall/src/syscall.c) | [`syscall_frame.h`](../../kernel/core/syscall/include/syscall_frame.h), [`syscall_validate.c`](../../kernel/core/syscall/src/syscall_validate.c), [`syscall_copy.c`](../../kernel/core/syscall/src/syscall_copy.c) | Entry state, central validation, safe copy, return validation |
| CPU-local state | [`gdt.c`](../../arch/x86_64/gdt/gdt.c), [`cpu_features.c`](../../arch/x86_64/cpu/cpu_features.c) | [`percpu.h`](../../arch/x86_64/cpu/percpu.h), [`percpu.c`](../../arch/x86_64/cpu/percpu.c), [`percpu_asm.inc`](../../arch/x86_64/cpu/percpu_asm.inc) | GS-based CPU identity, TSS, syscall and scheduler locals |
| ACPI/topology | none found | [`acpi.h`](../../arch/x86_64/acpi/acpi.h), [`acpi.c`](../../arch/x86_64/acpi/acpi.c), [`madt.c`](../../arch/x86_64/acpi/madt.c) | Firmware discovery and validated topology |
| APIC/IOAPIC/IPI | PIC path in [`irq.c`](../../kernel/core/interrupt/src/irq.c) | [`lapic.c`](../../arch/x86_64/apic/lapic.c), [`ioapic.c`](../../arch/x86_64/apic/ioapic.c), [`ipi.c`](../../arch/x86_64/apic/ipi.c) | Interrupt controller, timers, remote coordination |
| AP startup | current boot code under [`boot/`](../../boot/) | [`smp.c`](../../arch/x86_64/smp/smp.c), [`ap_trampoline.asm`](../../arch/x86_64/smp/ap_trampoline.asm) | INIT-SIPI-SIPI and AP online state |
| SMP scheduler | [`scheduler.c`](../../kernel/core/scheduler/src/scheduler.c), [`runqueue.c`](../../kernel/core/scheduler/src/runqueue.c), [`task.h`](../../kernel/core/scheduler/include/task.h) | [`scheduler_smp.c`](../../kernel/core/scheduler/src/scheduler_smp.c), [`scheduler_cpu.h`](../../kernel/core/scheduler/include/scheduler_cpu.h) | Per-CPU queues, affinity, migration, balancing |
| Synchronization | heap lock in [`heap.c`](../../kernel/core/memory/heap/src/heap.c), IPC primitives in [`ipc_sync.c`](../../kernel/ipc/sync/ipc_sync.c) | [`spinlock.h`](../../kernel/core/sync/spinlock.h), [`spinlock.c`](../../kernel/core/sync/spinlock.c), [`mutex.c`](../../kernel/core/sync/mutex.c), [`lockdep.c`](../../kernel/core/sync/lockdep.c) | Common lock semantics and diagnostics |
| TLB coordination | [`vmm.c`](../../kernel/core/memory/vmm/src/vmm.c) | [`tlb.c`](../../arch/x86_64/mm/tlb.c), [`tlb.h`](../../arch/x86_64/mm/tlb.h) | Residency, shootdown, acknowledgement |
| Watchdog/profiling | Vizier in [`vizier_core.c`](../../kernel/core/vizier/src/vizier_core.c), execution trace in [`execution_contract.c`](../../kernel/core/execution/src/execution_contract.c) | [`kernel_watchdog.c`](../../kernel/debug/kernel_watchdog.c), [`kernel_profile.c`](../../kernel/debug/kernel_profile.c), [`kernel_invariants.c`](../../kernel/debug/kernel_invariants.c) | Progress checks, latency metrics, invariant failures |
| Certification | [`execution_certification.c`](../../kernel/debug/execution_certification.c), [`user_mode_certification.c`](../../kernel/debug/user_mode_certification.c) | [`syscall_certification.c`](../../kernel/debug/syscall_certification.c), [`smp_certification.c`](../../kernel/debug/smp_certification.c), [`final_kernel_certification.c`](../../kernel/debug/final_kernel_certification.c) | Runtime reports with explicit statuses |
| Host harness | [`build.ps1`](../../build.ps1), current canary scripts | [`test_kernel_matrix.ps1`](../../tools/test_kernel_matrix.ps1), [`verify_kernel_log.ps1`](../../tools/verify_kernel_log.ps1) | Build, QEMU CPU matrix, timeout, parsing, artifacts |
| Architecture records | Phase 3/4 documents | [`docs/kernel/`](../kernel/) follow-up certification documents | Stable contracts, evidence, release decision |

All new translation units must be added explicitly because [`build.ps1`](../../build.ps1:24) manually enumerates compilation and the link line in [`build.ps1`](../../build.ps1:1188) manually enumerates objects.

## 10. Build and runtime verification plan

## 10.1 Build verification

1. Run the complete [`build.ps1`](../../build.ps1:1) pipeline from a clean output directory.
2. Require successful assembly/compilation/link of kernel entry, interrupt/context/syscall assembly, Phase 5–7 objects, userspace wrappers, minimal certification ELF, and final test ELF.
3. Preserve existing kernel payload and disk checks in [`build.ps1`](../../build.ps1:1194) and image signature/alignment checks in [`build.ps1`](../../build.ps1:1420).
4. Add ABI-generation consistency checks before compilation.
5. Add symbol/map checks for exactly one production syscall dispatcher, expected per-CPU symbols, AP trampoline placement, and no global syscall scratch symbol.
6. Archive the kernel map, image hash, tool versions, build output, and generated ABI manifest.

**Current status:** build mechanism source **PASS**; current audit-session build result **NOT VERIFIED**.

## 10.2 Runtime matrix

| Configuration | Required before run | Required evidence | Current status |
|---|---|---|---|
| 1 CPU, PIC fallback | Phase 5 complete | Boot, CPL 3 launch, syscall return, faults contained, 100/1,000 process lifecycle, no leaks | **NOT VERIFIED** |
| 1 CPU, APIC mode | LAPIC/IOAPIC foundation | Topology=1, APIC timer/EOI, same Phase 5 suite | **NOT VERIFIED** |
| 2 CPUs | AP startup and per-CPU scheduler | CPU0/CPU1 online, unique APIC IDs, per-CPU idle/current Task, IPI and cross-CPU wake, affinity/migration, TLB shootdown | **NOT VERIFIED** |
| 4 CPUs | 2-CPU gates pass | CPU0–CPU3 online, concurrent stress, balancing, race and teardown tests, complete IPI acknowledgements | **NOT VERIFIED** |
| 8 CPUs | Explicit support decision after 4-CPU pass | Same gates plus supported-limit declaration and scale diagnostics | **Not in initial supported matrix** |

## 10.3 Machine-verifiable serial contract

Each run should emit one structured header containing build ID, image hash, ABI version, requested CPU count, discovered CPU count, online CPU count, interrupt mode, and test profile. Each test must emit exactly one terminal result with PASS, FAIL, or NOT VERIFIED. The host parser must reject:

- missing terminal result;
- duplicate or out-of-order result;
- panic, page fault in kernel, GPF in kernel, triple-fault/reset signature, or deadlock timeout;
- requested/discovered/online CPU mismatch;
- AP heartbeat loss;
- queue invariant failure;
- lost or timed-out IPI/TLB acknowledgement;
- nonzero leak delta;
- unexpected QEMU exit;
- host timeout without a deliberate completion marker.

## 10.4 Phase-specific runtime scenarios

### Phase 5 scenarios

- minimal user ELF enters CPL 3 and performs getpid, uptime, yield, sleep, write, and exit;
- valid cross-page copy-in and copy-out;
- null, low, kernel, noncanonical, overflow, unmapped, RO, and NX cases;
- string without terminator at maximum length;
- file/event/GUI structure marshalling;
- faulting user process dies while a sibling continues;
- 100 then 1,000 real user-process launches with address-space and resource baseline comparison.

### Phase 6 scenarios

- AP online/offline timeout handling;
- one pinned worker per CPU and affinity rejection;
- remote wake from CPU0 to each AP;
- forced queue imbalance and migration;
- concurrent process exit during remote execution;
- concurrent map/unmap with TLB shootdown;
- reschedule, stop, and diagnostic IPIs;
- lock-order inversion injection and contention telemetry.

### Phase 7 scenarios

- malformed syscall corpus;
- scheduler state-transition fuzzing;
- memory pressure and allocation failure injection;
- repeated user faults and process teardown;
- IRQ/preemption-off threshold injection;
- watchdog-stall injection on one CPU;
- sustained mixed VFS, GUI, input, process, and IPC workloads;
- long-duration run with periodic snapshots and bounded growth criteria.

## 11. Risk register

| ID | Risk | Evidence | Impact | Required mitigation | Current status |
|---|---|---|---|---|---|
| R1 | Raw user pointers reach kernel subsystems | Direct dereferences in [`syscall.c`](../../kernel/core/syscall/src/syscall.c:39) | Kernel memory disclosure/corruption and kernel faults | Metadata-driven bounded copy-in/out for every pointer | **FAIL** |
| R2 | Global syscall user-RSP scratch races | [`syscall_entry.asm`](../../kernel/core/syscall/src/syscall_entry.asm:77) | Cross-thread/CPU stack corruption | CPU-local syscall frame before SMP | **FAIL** |
| R3 | Unsafe SYSRET state | [`syscall_entry.asm`](../../kernel/core/syscall/src/syscall_entry.asm:132) | GPF or privilege-return failure | Canonicality/RFLAGS validation and IRET fallback | **FAIL** |
| R4 | Broad user-visible kernel mappings | [`vmm_create_address_space()`](../../kernel/core/memory/vmm/src/vmm.c:335), [`heap_init()`](../../kernel/core/memory/heap/src/heap.c:136) | Ring 3 kernel-memory access | Correct upper-level/leaf permissions and isolation tests | **FAIL** |
| R5 | Shared page-table ownership ambiguity | [`vmm_destroy_address_space()`](../../kernel/core/memory/vmm/src/vmm.c:402) | Double free, leak, or shared mapping destruction | Explicit ownership/refcount policy and teardown tests | **NOT VERIFIED** |
| R6 | Two independent syscall gateways | [`syscall.c`](../../kernel/core/syscall/src/syscall.c:25), [`syscall_gateway.c`](../../kernel/core/syscall/syscall_gateway.c:14) | Policy bypass and namespace drift | Declare one production authority; isolate/remove duplicate path | **FAIL** |
| R7 | ABI declaration/implementation drift | [`syscall.h`](../../kernel/core/syscall/include/syscall.h:12), [`syscall.c`](../../kernel/core/syscall/src/syscall.c:25) | Unreachable or wrong syscall behavior | Generated table and compile-time coverage | **FAIL** |
| R8 | Singleton scheduler/TSS on SMP | [`scheduler.c`](../../kernel/core/scheduler/src/scheduler.c:24), [`gdt.c`](../../arch/x86_64/gdt/gdt.c:11) | Immediate multi-CPU corruption | CPU-local scheduler, idle, GDT/TSS, and entry state | **FAIL** |
| R9 | Local IRQ disable mistaken for SMP lock | [`execution_contract.c`](../../kernel/core/execution/src/execution_contract.c:17) and manager critical sections | Data races across CPUs | Common IRQ-safe locks and lock hierarchy | **FAIL** |
| R10 | No APIC/IPI/TLB shootdown | PIC-only [`irq.c`](../../kernel/core/interrupt/src/irq.c:1) | No functional SMP or coherent mappings | APIC/IOAPIC/IPI/TLB architecture | **FAIL** |
| R11 | Affinity metadata not enforced | [`Task`](../../kernel/core/scheduler/include/task.h:51), [`select_next_task()`](../../kernel/core/scheduler/src/scheduler.c:237) | Incorrect placement and migration | Eligibility checks and affinity tests | **FAIL** |
| R12 | Transition-path contradiction around IF/TSS | [`enter_usermode.asm`](../../kernel/core/scheduler/src/enter_usermode.asm:31), [`process_spawn()`](../../kernel/core/process/src/process.c:70) | Triple fault, stalls, or inconsistent Ring 3 behavior | One authoritative entry path and frame contract | **FAIL** |
| R13 | Certification objects compiled but not invoked | [`build.ps1`](../../build.ps1:419), boot path in [`kernel.c`](../../kernel/kernel.c:995) | False confidence from build presence | Explicit runtime invocation and serial report | **NOT VERIFIED** |
| R14 | Existing SMP scripts only configure QEMU | [`test_canary_smp2.ps1`](../../test_canary_smp2.ps1:5), [`test_canary_smp4.ps1`](../../test_canary_smp4.ps1:5) | Multi-vCPU launch mislabeled as SMP pass | Require AP online, per-CPU work, and IPI evidence | **NOT VERIFIED** |
| R15 | “Long” run is 60 seconds with no parser | [`test_canary_smp2_long.ps1`](../../test_canary_smp2_long.ps1:5) | Unsupported stability claims | Parameterized long harness and acceptance parser | **FAIL** |
| R16 | Busy-wait synchronization lacks scheduler integration | [`ipc_mutex_lock()`](../../kernel/ipc/sync/ipc_sync.c:51) | CPU waste, starvation, priority inversion | Blocking mutex/wait queues and priority policy | **FAIL** |
| R17 | Heap is hardened locally but remains user-mapped | [`heap.c`](../../kernel/core/memory/heap/src/heap.c:136) | Canary detection occurs after privilege boundary failure | Remove user access and certify mapping hierarchy | **FAIL** |
| R18 | Initialization order/reinitialization obscures stress meaning | [`kernel.c`](../../kernel/kernel.c:902), [`bosx_stress.c`](../../kernel/core/loader/bosx_stress.c:92) | Tests may reset state rather than validate live kernel | Dedicated post-init certification profile | **FAIL** |
| R19 | Logs with generic PASS labels may concern unrelated subsystems | [`phase4_certification_final.log`](../../phase4_certification_final.log:91) | Evidence misattribution | Test IDs, domains, structured results, artifact hashes | **NOT VERIFIED** |
| R20 | No coordinated multi-CPU panic/stop snapshot | No IPI implementation found; current panic paths halt locally | Lost evidence and continuing corruption | Stop IPI, per-CPU snapshot, bounded acknowledgement | **FAIL** |

## 12. Certification gates and explicit non-claims

### 12.1 What may be claimed now

- **PASS:** Process, Thread, Task, scheduler, GDT/TSS, VMM user-validation, safe-copy helper, and exception-diagnostic foundations are present in source.
- **PASS:** The build script compiles and links the Phase 3/4 source objects and builds userspace ELFs.
- **PASS:** QEMU scripts exist for configured 1/2/4-vCPU launches.
- **PASS:** Historical logs show bounded BSP boot and scheduler markers under some multi-vCPU QEMU configurations.

### 12.2 What must not be claimed now

- Ring 3 execution: **NOT VERIFIED**.
- Successful production syscall entry/return from Ring 3: **NOT VERIFIED**.
- Complete syscall pointer safety: **FAIL**.
- Strong user/kernel memory isolation: **FAIL**.
- Safe per-thread/per-CPU syscall state: **FAIL**.
- Actual SMP startup or scheduling: **FAIL** in source and **NOT VERIFIED** at runtime.
- APIC/IOAPIC/IPI/TLB shootdown support: **FAIL**.
- 2/4-CPU stability: **NOT VERIFIED**.
- 8-CPU support: **NOT VERIFIED** and not in the initial supported matrix.
- Long-duration stability: **NOT VERIFIED**.
- Final Phase 5, Phase 6, or Phase 7 certification: **NOT VERIFIED**.
- Current clean build result during this audit: **NOT VERIFIED**.

## 13. Final audit decision

**Architecture audit: COMPLETE.**

**Phase 5 readiness: CONDITIONAL.** The Phase 4 primitives are adequate to begin implementation, but production syscall work must start with ABI ownership, CPU-local entry state, safe copy, return validation, and page-table isolation.

**Phase 6 readiness: BLOCKED by Phase 5 and missing platform substrate.** There is no present SMP architecture beyond metadata and multi-vCPU QEMU launch scripts.

**Phase 7 readiness: FOUNDATION ONLY.** Existing diagnostics are useful inputs, but final hardening requires kernel-wide invariants, per-CPU observability, watchdogs, deterministic stress, machine-verifiable test harnesses, and long-duration evidence.

The required implementation order is Phase 5 secure uniprocessor boundary, Phase 5 runtime certification, Phase 6 topology/APIC/CPU-local/AP startup, Phase 6 scheduler and IPI coherence, then Phase 7 hardening and release certification. Any broader claim would exceed the inspected evidence.
