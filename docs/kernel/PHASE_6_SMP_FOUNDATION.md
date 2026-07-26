# Phase 6 SMP Foundation — Implementation and Evidence Report

## 1. Scope and evidence policy

This report records the compatibility-preserving Phase 6 foundation implemented after the completed Phase 5 syscall work in [`PHASE_5_SYSCALL_BOUNDARY_HARDENING.md`](PHASE_5_SYSCALL_BOUNDARY_HARDENING.md:1), following the architecture constraints in [`PHASES_5_7_FINAL_KERNEL_ARCHITECTURE_AUDIT.md`](PHASES_5_7_FINAL_KERNEL_ARCHITECTURE_AUDIT.md:321).

The implementation deliberately preserves BSP execution, the PIC interrupt path, Process Manager and Thread Manager ownership, Task as the executable context authority, scheduler state values, Ring 3 selectors, and the Phase 5 syscall ABI. It does not claim that APs execute or that concurrent scheduling occurs.

Status vocabulary:

- **PASS**: directly established by inspected source or an observed build result, narrowly scoped to the stated property.
- **FAIL**: a required property remains absent or unsafe.
- **NOT VERIFIED**: code or scaffolding exists, but direct runtime evidence was not observed.

## 2. Implemented architecture

### 2.1 CPU topology and ACPI MADT discovery — PASS for source foundation

[`smp.c`](../../arch/x86_64/smp/smp.c:1) now provides bounded ACPI discovery:

- scans the EBDA first and then the legacy BIOS ACPI window for the RSDP on 16-byte boundaries;
- validates the ACPI 1.0 checksum and, for revision 2+, the extended checksum and bounded RSDP length;
- selects XSDT when valid and available, otherwise RSDT;
- validates SDT length and checksum before table use;
- locates and parses the MADT;
- enumerates enabled/online-capable local APIC processors, IOAPIC descriptors, interrupt-source overrides, local APIC address overrides, and bounded x2APIC entries;
- caps topology storage at eight CPUs, four IOAPICs, and sixteen overrides and records truncation rather than writing out of bounds;
- deduplicates APIC IDs;
- explicitly selects a one-CPU BSP fallback when valid ACPI/MADT information is unavailable or unusable.

[`ATOMS_CPUTopology`](../../arch/x86_64/smp/smp.h:52) distinguishes discovered, configured, online, and scheduling CPU counts. A configured QEMU vCPU count is not accepted as proof of an online CPU.

### 2.2 Bounded per-CPU ownership — PASS for BSP foundation

[`ATOMS_PerCPU`](../../arch/x86_64/smp/smp.h:73) supplies fixed-size ownership slots for:

- logical CPU and APIC IDs;
- absent/discovered/configured/starting/online/failed state;
- BSP identity and scheduler-enabled state;
- interrupt/preemption depth foundations;
- current and idle Task pointers;
- scheduler queue pointer foundation;
- TSS and syscall entry ownership fields;
- scheduler, context-switch, migration, and IPI counters.

[`atoms_smp_initialize_bsp()`](../../arch/x86_64/smp/smp.c:351) makes exactly one CPU online, associates the existing Phase 5 TSS with the BSP slot, and leaves every AP configured but offline. This preserves the existing singleton TSS/syscall assembly while preventing it from being used concurrently by an AP.

The scheduler continues exporting the compatibility global [`current_task`](../../kernel/core/scheduler/include/scheduler.h:100), while [`scheduler_current_task()`](../../kernel/core/scheduler/src/scheduler.c:579) consults CPU-local ownership first. Scheduler assignment points bind current and idle Tasks to the BSP CPU-local slot.

### 2.3 Local APIC and IPI message foundation — PASS for source abstraction; NOT VERIFIED at runtime

[`atoms_lapic_probe()`](../../arch/x86_64/smp/smp.c:427) detects architectural APIC support, reads the APIC base MSR, records xAPIC/x2APIC mode, and obtains the BSP APIC ID.

The abstraction includes:

- opt-in local APIC software enablement without replacing PIC routing;
- local APIC EOI;
- bounded message classes for reschedule, TLB shootdown, call-function, stop, and diagnostic requests;
- fixed vectors and xAPIC ICR send support;
- online-target validation, delivery-pending timeout, send-failure accounting, and per-CPU sent/received counters.

Local APIC mode is not enabled by the conservative boot integration. IOAPIC routing, local APIC timer operation, and live IPI delivery are **NOT VERIFIED**. x2APIC is discovered but sending through x2APIC MSRs is not implemented.

### 2.4 SMP synchronization primitive — PASS for source foundation

[`spinlock.h`](../../kernel/core/sync/spinlock.h:1) and [`spinlock.c`](../../kernel/core/sync/spinlock.c:1) add a common non-recursive spinlock with:

- atomic acquisition/release;
- CPU owner identity;
- IRQ-save/restore variants;
- rank metadata reserved for ordered-lock policy;
- acquisition, contention, and spin counters;
- recursive acquisition fail-closed behavior.

This is a foundation, not a claim that every PCB, TCB, VMM, scheduler, or device-state critical section has been converted. Kernel-wide lock conversion is **NOT VERIFIED** and remains incomplete.

### 2.5 Scheduler affinity and migration foundation — PASS for conservative source behavior

[`Task`](../../kernel/core/scheduler/include/task.h:30) remains the sole executable context authority and now carries assigned CPU, last CPU, and migration count metadata in addition to the preserved affinity mask.

The existing BSP run queue remains authoritative. Selection now enforces affinity and assigned-CPU eligibility in [`select_next_task()`](../../kernel/core/scheduler/src/scheduler.c:246). [`scheduler_set_task_affinity()`](../../kernel/core/scheduler/src/scheduler.c:584) rejects empty/unsupported masks. [`scheduler_request_migration()`](../../kernel/core/scheduler/src/scheduler.c:601) only accepts an online target, rejects migration of the current Task, updates accounting, and requests a reschedule IPI for a remote target.

No AP is online, so remote migration correctly fails today. Per-core run-queue pointers and counters exist, but independent AP run queues, stealing, load balancing, and concurrent dispatch are **NOT VERIFIED**.

### 2.6 AP startup scaffolding and truthful state — PASS for safe non-start policy

[`atoms_smp_prepare_aps()`](../../arch/x86_64/smp/smp.c:385) is an explicit safety gate. It records discovered APs as configured but does not issue INIT-SIPI-SIPI. The repository still lacks a proven low-memory AP trampoline and per-AP GDT/TSS/IDT, syscall MSRs, kernel/syscall stacks, FPU state, local timer, and idle Task initialization sequence.

Therefore:

- AP startup execution: **NOT VERIFIED**;
- AP online acknowledgement: **NOT VERIFIED**;
- concurrent scheduler execution: **NOT VERIFIED**;
- full SMP operation: **NOT VERIFIED**.

This is intentional: sending SIPIs before those prerequisites would violate the architecture audit and risk corrupting Phase 5 state.

## 3. Boot integration and compatibility

[`kernel_main()`](../../kernel/kernel.c:809) invokes discovery after the BSP GDT/TSS is initialized and before interrupts are enabled. The integration is read-only with respect to firmware and does not replace PIC setup, IRQ dispatch, PIT scheduling, Ring 3 setup, or syscall initialization.

Boot diagnostics emitted by [`atoms_smp_print_diagnostics()`](../../arch/x86_64/smp/smp.c:514) separately report:

- discovery source: ACPI MADT or BSP fallback;
- discovered CPU count;
- configured CPU count;
- online CPU count;
- scheduling CPU count;
- per-CPU APIC ID and BSP/AP status;
- a foundation-invariant self-test result accompanied by the explicit text `AP execution NOT VERIFIED`.

The self-test only checks bounded topology, unique APIC IDs, one online BSP, and CPU-local invariants. It does not certify SMP.

## 4. Build integration and evidence

[`build.ps1`](../../build.ps1:1023) compiles the SMP and common spinlock translation units, and the canonical kernel link includes both objects in [`build.ps1`](../../build.ps1:1192).

Command executed from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Observed evidence:

- Phase 6 objects compiled and linked;
- kernel payload before sector padding: 1,686,404 bytes;
- required kernel sectors: 3,294, within the configured 4,091-sector limit;
- padded [`kernel.bin`](../../build/kernel.bin) size: 1,686,528 bytes;
- [`OS.img`](../../build/OS.img) generated at 67,108,864 bytes;
- [`SignaturesOS.vdi`](../../build/SignaturesOS.vdi), [`SignaturesOS.vmdk`](../../build/SignaturesOS.vmdk), and [`SignaturesOS.vmx`](../../build/SignaturesOS.vmx) were generated with the same build timestamp;
- an additional syntax-only check of the SMP, spinlock, and scheduler translation units returned exit code 0.

**Canonical build: PASS.** No Phase 6 compile or link failure remained.

## 5. Runtime evidence

No QEMU or hardware run was performed in this implementation session. Consequently, the newly emitted serial diagnostics were not directly observed.

| Property | Status | Evidence boundary |
|---|---|---|
| Source-level bounded MADT parser and fallback | **PASS** | Direct source inspection and successful build |
| BSP-only compatibility architecture | **PASS** | Boot path retains PIC/PIT and marks only BSP online |
| Runtime ACPI table discovery | **NOT VERIFIED** | No boot log captured |
| Runtime topology matching configured QEMU CPUs | **NOT VERIFIED** | No QEMU run captured |
| Local APIC enable/EOI | **NOT VERIFIED** | Abstraction exists but boot retains PIC mode |
| IPI send/receive | **NOT VERIFIED** | No online AP and no runtime evidence |
| AP trampoline and INIT-SIPI-SIPI | **FAIL / intentionally not implemented** | Required safe per-AP prerequisites are absent |
| AP online acknowledgement | **NOT VERIFIED** | No AP startup path |
| AP idle-loop entry | **NOT VERIFIED** | No AP startup path |
| Concurrent per-core scheduling | **NOT VERIFIED** | Only BSP is online/scheduling |
| Affinity eligibility on BSP | **PASS for source** | Selector enforces mask and assigned CPU |
| Remote migration | **NOT VERIFIED** | Correctly unavailable without online AP |
| Per-core independent run queues/load balancing | **FAIL / foundation only** | Queue pointer exists; operational queues are not enabled |
| IOAPIC routing | **FAIL / foundation discovery only** | IOAPICs are enumerated but not programmed |
| TLB shootdown completion protocol | **FAIL / message class only** | No generation/acknowledgement protocol |
| Existing Ring 3/syscall runtime behavior | **NOT VERIFIED in this session** | Build compatibility only; no runtime test |

## 6. Changed files

- [`arch/x86_64/smp/smp.h`](../../arch/x86_64/smp/smp.h:1): bounded topology, CPU-local, APIC, and IPI contracts.
- [`arch/x86_64/smp/smp.c`](../../arch/x86_64/smp/smp.c:1): ACPI/MADT discovery, explicit fallback, BSP ownership, AP safety gate, local APIC/IPI foundations, diagnostics, and self-test.
- [`kernel/core/sync/spinlock.h`](../../kernel/core/sync/spinlock.h:1): common SMP spinlock interface.
- [`kernel/core/sync/spinlock.c`](../../kernel/core/sync/spinlock.c:1): atomic, owner-aware, IRQ-safe spinlock implementation.
- [`kernel/core/scheduler/include/task.h`](../../kernel/core/scheduler/include/task.h:30): CPU assignment and migration metadata without changing stable Task state values.
- [`kernel/core/scheduler/include/scheduler.h`](../../kernel/core/scheduler/include/scheduler.h:62): affinity and migration APIs.
- [`kernel/core/scheduler/src/scheduler.c`](../../kernel/core/scheduler/src/scheduler.c:1): CPU-local current/idle ownership hooks, affinity-aware selection, and conservative migration foundation.
- [`kernel/kernel.c`](../../kernel/kernel.c:1): early BSP-safe discovery and truthful diagnostics.
- [`build.ps1`](../../build.ps1:1023): explicit compilation and linking of Phase 6 objects.

## 7. Decision

**Phase 6 compatibility-preserving SMP foundation: PASS for source architecture and canonical build.**

**Actual SMP execution: NOT VERIFIED.** Multiple configured vCPUs are not treated as a pass. No AP was started, no AP online marker was observed, no IPI was observed, and no Task was observed executing concurrently on another CPU.

**Remaining operational SMP work: FAIL / incomplete** for AP trampoline/startup, per-AP GDT/TSS/syscall initialization, IOAPIC routing, local APIC timer, per-core operational run queues, remote wakeup, load balancing, TLB shootdown acknowledgements, and coordinated stop/panic operation. These gaps are stated explicitly and are not hidden by the successful build or the source self-test.
