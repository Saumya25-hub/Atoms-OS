# ATOMS OS Phase 4 — Ring 3 / User Mode Architecture and Certification

## Executive status

**Build status: PASS.**

**Desktop boot-smoke status: PASS.**

**Phase 4 implementation status: foundation integrated.**

**Final production certification: CONDITIONAL.**

The implementation extends the Phase 3 `Task`, Process Manager, Thread Manager, VMM, loader, exception, and syscall boundaries. It does not replace the completed scheduler architecture.

## Ring model

| Privilege | Selector model | Responsibilities |
|---|---|---|
| Ring 0 | Kernel code/data, DPL 0 | Kernel, drivers, interrupt handlers, page-table management |
| Ring 3 | User code/data, DPL 3 | User ELF execution, user stacks, application memory |

The existing GDT exposes user selectors [`USER_CODE_SEGMENT`](../../arch/x86_64/gdt/gdt.h:63) and [`USER_DATA_SEGMENT`](../../arch/x86_64/gdt/gdt.h:64). The IDT exposes vector `0x80` to user mode while hardware exceptions remain kernel-controlled.

## Execution flow

```text
ELF image
  -> vmm_create_address_space()
  -> loader maps PT_LOAD pages
  -> process_build_user_stack() maps user stack + guard page
  -> process_spawn() creates PCB/TCB/Task binding
  -> scheduler selects Task and CR3
  -> enter_usermode() performs IRETQ to CPL 3
  -> user executes and invokes SYSCALL/INT 0x80
  -> TSS.RSP0 supplies kernel stack
  -> kernel validates user pointers
  -> syscall returns to user execution
```

The transition assembly is [`enter_usermode`](../../kernel/core/scheduler/src/enter_usermode.asm:6). The SYSCALL entry path is [`syscall_entry`](../../kernel/core/syscall/src/syscall_entry.asm:77).

## Address-space contract

User addresses are restricted to:

- minimum: `0x0000000001000000`;
- maximum: `0x00007FFFFFFFFFFF`;
- null and low pages: rejected;
- non-canonical addresses: rejected;
- integer-overflowing ranges: rejected.

The public VMM contract is in [`vmm.h`](../../kernel/core/memory/vmm/include/vmm.h:1). User pages use the `PAGE_USER` bit. Write access requires `PAGE_WRITABLE`; execute access rejects `PAGE_NX`. Kernel mappings are not accepted by user-range validation.

### Stack layout

The user stack is built at [`USER_STACK_TOP`](../../kernel/core/process/include/process_builder.h:8), with an unmapped guard page below the committed stack. Stack pages are user-readable and user-writable but non-executable by default.

### Process isolation

[`vmm_create_address_space()`](../../kernel/core/memory/vmm/src/vmm.c:307) creates a process page-table root and copies only the currently established kernel mapping structure. User mappings are added to each process root independently. [`vmm_query_page()`](../../kernel/core/memory/vmm/src/vmm.c:210) and [`vmm_validate_user_range()`](../../kernel/core/memory/vmm/src/vmm.c:227) provide inspection and access validation.

The address-space implementation is prepared for a stronger kernel/user split. A remaining hardening item is removal of user-visible permission from any shared upper-level kernel mapping before declaring full kernel-memory isolation certified.

## Memory protection API

- [`vmm_map_user_page()`](../../kernel/core/memory/vmm/src/vmm.c:246) allocates a user page with requested read/write/execute policy.
- [`vmm_map_guard_page()`](../../kernel/core/memory/vmm/src/vmm.c:258) ensures a page remains unmapped.
- [`vmm_validate_user_range()`](../../kernel/core/memory/vmm/src/vmm.c:227) checks present, user, writable, and executable properties page by page.
- [`ATOMS_UserMode_CopyFromUser()`](../../kernel/core/usermode/user_mode.c:62) and [`ATOMS_UserMode_CopyToUser()`](../../kernel/core/usermode/user_mode.c:78) reject invalid user pointers before copying.

Syscall handlers should migrate pointer-bearing paths to these copy helpers. Existing legacy syscall implementations remain compatible but are not all converted to validated copies in this phase.

## Process and thread integration

The PCB now records user image bounds, entry point, stack guard, address space, and last user exception context. The binding API is [`ATOMS_Process_SetUserImage()`](../../kernel/core/process/process_manager.c:445).

User process creation is integrated through [`process_spawn()`](../../kernel/core/process/src/process.c:12):

1. validate image, entry point, and stack;
2. create/find PCB;
3. allocate kernel stack;
4. create and bind a TCB;
5. record image/address-space metadata;
6. submit the Task to the Phase 3 scheduler.

The TCB remains the owner of thread identity and statistics; the scheduler Task remains the owner of executable CPU context.

## Ring transition and syscall compatibility

The existing MSR setup uses STAR/LSTAR/FMASK in [`syscall_init_asm()`](../../kernel/core/syscall/src/syscall_entry.asm:18). The syscall entry path switches from user RSP to TSS.RSP0, preserves user RIP/RFLAGS/RSP, calls the existing C syscall dispatcher, and uses SYSRET.

Security hardening required before final certification:

- validate every pointer argument in every syscall;
- validate syscall IDs and argument lengths at one gateway;
- prevent user-controlled kernel stack reuse;
- add canonical RIP/RSP and user-segment checks before SYSRET;
- add a dedicated per-thread syscall frame rather than the current scratch global.

## Exception model

User-origin exceptions are detected by CPL bits in the saved CS. [`ATOMS_UserMode_HandleException()`](../../kernel/core/usermode/user_mode.c:103) records:

- vector;
- RIP/RSP/RFLAGS;
- CR2/fault address;
- error code;
- PID/TID;
- address space;
- user/kernel exception counters.

For a faulting user Task, the current implementation records PCB fault state, terminates the scheduler Task, and terminates the owning process when possible. Kernel-origin faults retain the existing panic path.

Dedicated handlers remain registered for page fault and general protection fault in [`exception_init()`](../../kernel/core/interrupt/src/exception.c:300). This preserves kernel diagnostics while making user faults recoverable at the process boundary.

## Diagnostics

[`ATOMS_UserModeDiagnostics`](../../kernel/core/usermode/user_mode.h:46) exposes:

- ring entries and returns;
- user and kernel exception counts;
- page and protection fault counts;
- privilege violations and invalid pointer rejections;
- address-space lifecycle counters;
- current address space, PID, TID, and ring;
- last exception context;
- terminated user-process count.

Debug utilities include:

- [`ATOMS_UserMode_DumpPrivilege()`](../../kernel/core/usermode/user_mode.c:158);
- [`ATOMS_UserMode_DumpException()`](../../kernel/core/usermode/user_mode.c:177);
- [`ATOMS_UserMode_DumpMappings()`](../../kernel/core/usermode/user_mode.c:194);
- [`vmm_dump_address_space()`](../../kernel/core/memory/vmm/src/vmm.c:267).

## Certification tests

The Phase 4 test contract is declared by [`ATOMS_UserModeCertificationReport`](../../kernel/debug/user_mode_certification.h:7) and implemented by [`ATOMS_UserMode_RunCertification()`](../../kernel/debug/user_mode_certification.c:16).

Covered checks:

- canonical-address validation;
- null-page rejection;
- kernel-range rejection;
- arithmetic-overflow rejection;
- address-space creation;
- per-address-space mapping isolation;
- user/read/write/NX permission behavior;
- read-only write rejection;
- executable mapping validation;
- guard-page absence;
- ring selector values;
- kernel idle privilege;
- diagnostics coherence.

The test object is compiled and linked by [`build.ps1`](../../build.ps1:423). The test suite is available for explicit invocation by the kernel debug harness; it should not be interpreted as proof that a user ELF has executed successfully unless its report is emitted in the runtime log.

## Verification evidence

### Full build

The complete [`build.ps1`](../../build.ps1:1) pipeline completed successfully after the Phase 4 changes:

- kernel compiled and linked;
- user-mode and certification objects compiled and linked;
- userspace ELFs built;
- raw image validation passed;
- VDI, VMDK, and VMX artifacts generated.

Observed warnings were pre-existing DOOM pointer-cast, empty-loop, and CRT `fopen` deprecation warnings. No Phase 4 compile or link failure was reported.

### QEMU desktop smoke test

The bounded QEMU run reached:

- `[SCHED] Production scheduler initialized`;
- `TMR & AME OK`;
- boot task registration;
- context save and restore pass markers;
- desktop main loop;
- stable scheduler telemetry with two runnable tasks;
- no captured page fault, GPF, scheduler panic, or kernel halt.

This validates that the Phase 4 changes did not regress the existing desktop boot path.

## Known limitations and certification boundaries

The following are **not** claimed as fully certified from the available evidence:

1. A real user ELF was not proven by a captured runtime line showing CPL 3 execution.
2. No explicit successful Ring 3 syscall/return trace was captured.
3. The current VMM address-space constructor retains broad copied kernel mappings; a complete user/kernel upper-half permission audit is required before claiming strong kernel-memory isolation.
4. Not every existing syscall pointer argument is routed through `ATOMS_UserMode_CopyFromUser()` or `ATOMS_UserMode_CopyToUser()`.
5. The syscall scratch stack variable is global rather than per-CPU/per-thread.
6. Address-space destruction is a basic reclamation implementation and requires deeper page-table ownership auditing for shared kernel structures.
7. 100/1,000 user-process stress, long runtime, leak detection, SMP, and race audits were not executed in this session.
8. The certification test object is compiled and linked, but its full runtime report was not emitted by the boot smoke run.

## Phase 5 handoff

Before beginning the Phase 5 system-call milestone, complete these gates:

- convert every pointer-bearing syscall to validated user copies;
- add a dedicated syscall-frame structure and per-CPU entry storage;
- add a user ELF launch test that proves CPL 3, syscall entry, syscall return, and process exit;
- harden upper-half page-table permissions and audit shared mappings;
- add process/address-space teardown tests;
- run 100 and 1,000 user-process stress tests;
- capture explicit certification reports in QEMU logs.

## Certification decision

**Phase 4 implementation: build-integrated and desktop boot-smoke validated.**

**Phase 4 production certification: CONDITIONAL / NOT FINAL.**

The architecture is prepared for Phase 5, but the strict completion criteria requiring proven Ring 3 execution, user application launch, enforced kernel-memory isolation, validated syscall pointers, and certification reports are not all demonstrated by the available runtime evidence. Marking Phase 4 fully production-certified would overstate the verification results.
