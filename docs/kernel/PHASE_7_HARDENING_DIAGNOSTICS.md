# Phase 7 Kernel Hardening, Diagnostics, Profiling, Watchdog, and Bounded Stress

## 1. Scope and evidence policy

This report records the Phase 7 reliability/observability foundation implemented after the Phase 5 syscall boundary and Phase 6 BSP-compatible SMP foundation. It follows the constraints in [`PHASES_5_7_FINAL_KERNEL_ARCHITECTURE_AUDIT.md`](PHASES_5_7_FINAL_KERNEL_ARCHITECTURE_AUDIT.md:359), preserves the compatibility claims in [`PHASE_5_SYSCALL_BOUNDARY_HARDENING.md`](PHASE_5_SYSCALL_BOUNDARY_HARDENING.md:1), and does not exceed the SMP evidence boundary in [`PHASE_6_SMP_FOUNDATION.md`](PHASE_6_SMP_FOUNDATION.md:136).

Status vocabulary:

- **PASS**: directly established by source inspection or an observed command/build result, narrowly scoped to the stated property.
- **FAIL**: the required property is absent, incomplete, or unsafe.
- **NOT VERIFIED**: scaffolding exists but the runtime property was not directly observed.

No QEMU or hardware run, 24-hour test, or 72-hour test was performed. No Ring 3 execution or SMP execution is claimed.

## 2. Bounded architecture

### 2.1 Fixed-size contracts — PASS

[`phase7_reliability.h`](../../kernel/debug/phase7_reliability.h:1) defines compile-time bounds for eight CPU slots, 43 syscall IDs, eight latency buckets, 24 driver-health entries, eight held-lock ranks, 32 crash events, 256 stress iterations, and 24-byte driver names. The implementation in [`phase7_reliability.c`](../../kernel/debug/phase7_reliability.c:1) uses only static arrays and performs no dynamic allocation.

Runtime policy defaults to watchdog diagnostics only. Verbose and self-test policy bits are explicit. Watchdog periods, stall thresholds, report limits, spin limits, and stress iteration limits are clamped in [`atoms_p7_init()`](../../kernel/debug/phase7_reliability.c:58).

### 2.2 Crash, assert, and panic metadata — PASS for source foundation

The bounded crash ring records sequence, timer tick, sampled RIP/RSP, Task and process identity, CPU ID, event code/type, source location, function, and reason in [`atoms_p7_capture_event()`](../../kernel/debug/phase7_reliability.c:172). Assertion failure captures metadata before interrupts are disabled and appends the Phase 7 snapshot to existing crash-log and Vizier diagnostics in [`kernel_panic_assert()`](../../kernel/core/interrupt/src/exception.c:346).

The generic panic-capture API [`atoms_p7_capture_panic()`](../../kernel/debug/phase7_reliability.c:205) is available for bounded crash hooks. Full exception-path conversion and coordinated AP stop snapshots are **NOT VERIFIED** because Phase 6 does not start APs or provide stop-IPI execution.

### 2.3 Per-CPU/BSP-safe profiling and watchdog — PASS for source foundation

[`ATOMS_P7CPUProfile`](../../kernel/debug/phase7_reliability.h:60) tracks heartbeat, timer/IRQ/syscall entries, scheduler progress, context switches, sampled RIP, errors, stall state, and fixed logarithmic latency histograms. [`atoms_p7_tick()`](../../kernel/debug/phase7_reliability.c:92) updates the current bounded CPU slot and invokes periodic watchdog checks. [`atoms_p7_watchdog_check()`](../../kernel/debug/phase7_reliability.c:147) is diagnostic/fail-safe: it records bounded stale-heartbeat events and does not reset or panic the machine.

The PIT handler records IRQ duration and scheduler progress through [`timer_tick_handler()`](../../kernel/core/timer/src/timer.c:14). The active Phase 5 dispatcher records per-ID counts, errors, total/max cycles, histograms, and sampled user RIP through [`syscall_handler()`](../../kernel/core/syscall/src/syscall.c:412). These use `RDTSC` deltas as low-overhead cycle measurements; conversion to calibrated time is not claimed.

AP-local runtime behavior is **NOT VERIFIED**. CPU slots are compatible with the Phase 6 eight-slot bound, but only BSP execution is presently enabled.

### 2.4 Lock diagnostics — PASS for common Phase 6 spinlock integration

The common spinlock now reports recursion, rank inversion, contention, bounded spin-threshold events, and held-rank stack transitions through [`atoms_spin_lock()`](../../kernel/core/sync/spinlock.c:36) and [`atoms_spin_unlock()`](../../kernel/core/sync/spinlock.c:68). The diagnostic rank stack is fixed at eight entries.

This is not a kernel-wide lockdep claim. Existing subsystem-private locks and local-IRQ critical sections have not all been converted; cross-CPU order checking remains **NOT VERIFIED** until real AP execution and CPU-local held-lock stacks exist.

### 2.5 Memory, heap, and stack diagnostics — bounded foundation

[`atoms_p7_check_heap()`](../../kernel/debug/phase7_reliability.c:306) invokes the existing heap metadata/canary validation and counts checks. [`atoms_p7_check_current_stack()`](../../kernel/debug/phase7_reliability.c:310) provides a conservative current-Task presence check and accounting hook.

- Existing heap front/rear canary validation: **PASS for existing source integration**.
- Phase 7 heap diagnostic hook: **PASS for source**.
- Kernel-stack canary validation on every context switch: **FAIL / not implemented**. Existing Task stacks do not expose a universal canary contract.
- Guard-page enforcement for every kernel stack: **NOT VERIFIED**.
- Compiler stack protector: **FAIL / intentionally still disabled by the canonical freestanding build flags**.

### 2.6 Driver health registry — PASS for bounded generic foundation

[`atoms_p7_driver_register()`](../../kernel/debug/phase7_reliability.c:260) provides a fixed registry with unique IDs, bounded names, required/optional status, lifecycle state, heartbeat, events, errors, and stale counters. State, heartbeat, and event hooks are O(24) bounded scans with no allocation.

Broad conversion of every ATA, network, USB, input, display, and audio path to these hooks is **NOT VERIFIED**. Existing Vizier contracts remain intact and are not replaced.

### 2.7 Deterministic bounded self-test — PASS for source/build; runtime semantics bounded

[`atoms_p7_run_bounded_self_test()`](../../kernel/debug/phase7_reliability.c:315) clamps requests to 256 iterations, computes a deterministic digest, and repeatedly checks scheduler consistency and Phase 5 syscall validation. It records process/thread/scheduler/synchronization/syscall-validation categories without creating or claiming real Ring 3 processes. Boot requests 32 iterations after execution managers, scheduler, user-mode substrate, and timer initialization in [`kernel_main()`](../../kernel/kernel.c:1004).

The framework status is explicit:

- bounded deterministic metadata/invariant exercise: **PASS for source and build**;
- real process/thread lifecycle race execution: **NOT VERIFIED**;
- live Ring 3 syscall execution: **NOT VERIFIED**;
- SMP synchronization race execution: **NOT VERIFIED**;
- VMM/TLB shootdown race testing: **FAIL / unavailable because Phase 6 has no operational shootdown protocol**.

## 3. Conservative integration

[`kernel_main()`](../../kernel/kernel.c:907) initializes the reliability layer only after the heap exists and does not alter PIC/PIT, scheduler ownership, Task state values, Process/Thread Manager ownership, Ring 3 selectors, syscall ABI, AP policy, or address-space switching. The watchdog defaults to report-only behavior. No AP is started and no local APIC routing is enabled.

[`build.ps1`](../../build.ps1:1027) explicitly compiles the Phase 7 translation unit, and the canonical link includes it in [`build.ps1`](../../build.ps1:1195).

## 4. Verification evidence

### 4.1 Focused compile — PASS

The Phase 7 implementation, common spinlock integration, and timer integration were compiled directly with the repository freestanding Clang target and flags. Command result: exit code 0.

### 4.2 Canonical Windows build — PASS

Command executed from repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Observed result: exit code 0. Evidence from the build output:

- Phase 7 object compiled and linked;
- kernel payload: 1,690,500 bytes;
- required kernel sectors: 3,302 of the configured 4,091-sector limit;
- userspace ELFs and Doom built;
- [`OS.img`](../../build/OS.img) generated;
- image size alignment, boot signature, kernel offset, and active sector count checks passed;
- VDI, VMDK, and VMX artifacts were generated;
- final build marker: `BUILD SUCCESSFUL`.

Warnings observed were pre-existing Doom pointer/empty-loop warnings and Windows CRT `fopen` deprecation warnings from the image builder. No Phase 7 compile or link failure remained.

### 4.3 Runtime evidence — NOT VERIFIED

No emulator or hardware execution was performed. Therefore the following are not certified:

- Phase 7 boot marker observation;
- runtime execution and terminal status of the 32-iteration source self-test;
- actual watchdog stall detection;
- histogram values under live IRQ/syscall load;
- assert/panic snapshot output;
- driver health hook behavior under device failure;
- heap/stack corruption injection;
- hardware-derived CPL3 execution or syscall return;
- AP startup, IPI, concurrent scheduling, or SMP race behavior;
- long-duration stability.

## 5. Certification matrix

| Property | Status | Evidence boundary |
|---|---|---|
| Fixed-size/no-unbounded-allocation Phase 7 core | **PASS** | Static bounded arrays and clamped loops |
| Canonical build and image validation | **PASS** | Observed exit code 0 and build markers |
| Assert metadata and bounded snapshot hook | **PASS for source** | Integrated assertion path |
| Generic panic snapshot API | **PASS for source** | API present; full exception conversion incomplete |
| BSP timer/IRQ profiling hook | **PASS for source/build** | PIT integration compiled and linked |
| Per-ID syscall profiling | **PASS for source/build** | Active Phase 5 dispatcher integration |
| Report-only heartbeat watchdog | **PASS for source/build** | Bounded diagnostic policy |
| Common spinlock contention/order diagnostics | **PASS for source/build** | Phase 6 spinlock integration |
| Kernel-wide lockdep | **FAIL / incomplete** | Not all locks use common primitive |
| Heap/canary diagnostic reuse | **PASS for source foundation** | Existing heap validator hook |
| Universal kernel-stack canaries | **FAIL** | No universal Task stack canary contract |
| Bounded driver-health registry | **PASS for source/build** | Fixed 24-entry registry |
| All production drivers registered | **NOT VERIFIED** | Generic hooks not universally adopted |
| Deterministic bounded invariant self-test | **PASS for source/build** | 32 requested, 256 hard maximum |
| Real Ring 3 stress | **NOT VERIFIED** | Not run or claimed |
| SMP stress | **NOT VERIFIED** | AP execution absent |
| 24/72-hour testing | **NOT VERIFIED** | Explicitly not run |

## 6. Changed files

- [`kernel/debug/phase7_reliability.h`](../../kernel/debug/phase7_reliability.h:1): bounded contracts and public hooks.
- [`kernel/debug/phase7_reliability.c`](../../kernel/debug/phase7_reliability.c:1): crash ring, profiler, watchdog, lock diagnostics, driver health, memory hooks, snapshots, and bounded self-test.
- [`kernel/core/interrupt/src/exception.c`](../../kernel/core/interrupt/src/exception.c:346): assertion metadata/snapshot integration.
- [`kernel/core/timer/src/timer.c`](../../kernel/core/timer/src/timer.c:14): heartbeat, scheduler progress, and IRQ-cycle profiling.
- [`kernel/core/syscall/src/syscall.c`](../../kernel/core/syscall/src/syscall.c:412): per-ID syscall cycle/error profiling.
- [`kernel/core/sync/spinlock.c`](../../kernel/core/sync/spinlock.c:36): contention, recursion, rank, timeout, and held-lock diagnostics.
- [`kernel/kernel.c`](../../kernel/kernel.c:907): conservative boot initialization and bounded source self-test request.
- [`build.ps1`](../../build.ps1:1027): explicit compilation/link integration.

## 7. Decision

**Phase 7 bounded reliability/observability source foundation: PASS.**

**Canonical Windows build: PASS.**

**Runtime Phase 7 certification: NOT VERIFIED.** Ring 3, AP/SMP execution, fault injection, and long-duration stability are not claimed. Kernel-wide lock conversion and universal kernel-stack canaries remain incomplete and are explicitly recorded as gaps.
