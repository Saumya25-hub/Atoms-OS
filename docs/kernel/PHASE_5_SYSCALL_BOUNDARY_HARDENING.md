# Phase 5 Syscall Boundary Hardening — Implementation Evidence

## Scope

This record covers only the Phase 5 syscall boundary work guided by [`PHASES_5_7_FINAL_KERNEL_ARCHITECTURE_AUDIT.md`](PHASES_5_7_FINAL_KERNEL_ARCHITECTURE_AUDIT.md:95). Process Manager, Thread Manager, Task, scheduler, and the existing Ring 3 foundation were preserved. SMP/APIC/Phase 6 and broad Phase 7 systems were not implemented.

Status vocabulary follows the audit: **PASS** means directly evidenced source/build property; **FAIL** means the property remains absent; **NOT VERIFIED** means no direct runtime evidence was observed.

## Exact changes

### PASS — ABI and dispatch ownership

- [`kernel/core/syscall/include/syscall.h`](../../kernel/core/syscall/include/syscall.h:1) now owns ABI version 1, stable syscall IDs 0–42, return codes, bounded copy limits, a single six-register argument model, a concrete [`ATOMS_SyscallFrame`](../../kernel/core/syscall/include/syscall.h:61), diagnostics, and a compile-time frame-size assertion.
- The unreachable ID 300 branch was removed. Invalid IDs are rejected before dispatch.
- Legacy [`INT 0x80`](../../kernel/core/syscall/src/syscall.c:466) is explicitly kernel-only compatibility with its old four-operation namespace; it is not a second Ring 3 ABI and does not remap production IDs.
- [`syscall_gateway.c`](../../kernel/core/syscall/syscall_gateway.c:14) is explicitly named [`ATOMS_KernelService_Dispatch()`](../../kernel/core/syscall/syscall_gateway.c:14), documenting that it is a separate kernel-internal application-service interface. The production Ring 3 authority remains [`syscall_handler()`](../../kernel/core/syscall/src/syscall.c:429).

### PASS — bounded user-copy policy in active dispatcher

- [`syscall.c`](../../kernel/core/syscall/src/syscall.c:33) copies strings byte-by-byte through [`ATOMS_UserMode_CopyFromUser()`](../../kernel/core/usermode/user_mode.c:60), with a 256-byte limit.
- Read and write use bounded 4 KiB kernel chunks through [`copy_read_to_user()`](../../kernel/core/syscall/src/syscall.c:61) and [`copy_write_from_user()`](../../kernel/core/syscall/src/syscall.c:96), with a 1 MiB total limit.
- Directory, keyboard, input, GUI event, and extended GUI structures are copied through [`ATOMS_UserMode_CopyToUser()`](../../kernel/core/usermode/user_mode.c:76) or [`ATOMS_UserMode_CopyFromUser()`](../../kernel/core/usermode/user_mode.c:60) before subsystem use.
- Surface dimensions are overflow-checked and pixel data is copied into a bounded kernel allocation before presentation.
- No active production Ring 3 dispatcher call passes the original user pointer to VFS, input, GUI, or surface subsystems.

### PASS — entry and return-state hardening

- [`syscall_entry.asm`](../../kernel/core/syscall/src/syscall_entry.asm:45) no longer uses the former global `syscall_scratch_rsp` slot. User RSP and active frame state are stored in software-owned fields adjacent to the BSP TSS; this is a CPU-local substrate in the current uniprocessor architecture and is extensible for Phase 6 without an ABI redesign.
- The assembly/C frame contract captures user RIP, RSP, RFLAGS, number, six arguments, result, task identity, PID/TID, nesting, termination, and return mode.
- [`syscall_prepare_return()`](../../kernel/core/syscall/src/syscall.c:446) checks task identity, termination state, canonical/user RIP and RSP range, and executable/readable and writable page permissions before return.
- Unsafe RFLAGS bits are masked and a safe baseline is restored. Validated state uses SYSRET; sanitized-but-non-SYSRET state uses an IRET frame; rejected state enters a failure path and terminates/does not return to the invalid user frame.
- IA32_FMASK now clears control bits including IF, TF, DF, IOPL, NT, RF, and AC on entry; return validation remains explicit.

### PASS — bounded diagnostics and source self-test

- [`ATOMS_SyscallDiagnostics`](../../kernel/core/syscall/include/syscall.h:85) tracks entries, exits, invalid IDs, not-implemented calls, pointer/size rejections, return rejections, nesting, legacy entries, and per-ID totals/errors.
- [`syscall_phase5_self_test()`](../../kernel/core/syscall/src/syscall.c:481) checks invalid-ID rejection, canonical-address policy, user-range bounds, and C/assembly frame size. It is a source-level boundary self-test only and does not claim CPL3 execution.

## Verification evidence

### PASS — canonical build

Command executed from repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Result: exit code 0. The canonical pipeline assembled, compiled, linked the kernel, built userspace ELFs, created `OS.img`, validated image alignment/signature/offset checks, and produced VDI/VMDK/VMX artifacts. Kernel payload was 1,674,116 bytes and within the configured reserved capacity. Existing unrelated compiler warnings were emitted by Doom and the Windows image-builder CRT deprecation diagnostics; no Phase 5 compile/link error remained.

### NOT VERIFIED — runtime boundary execution

No QEMU or direct runtime test was executed in this implementation session. In particular, the following remain unverified: actual CPL3 entry, successful SYSCALL/SYSRET or IRET return, fault containment under malformed user mappings, live copy-fault behavior, syscall counters observed in serial output, and 100/1,000 real user-process lifecycle tests.

## Remaining gaps / explicit non-claims

- **FAIL / outside this scope:** broad user/kernel page-table isolation and shared mapping ownership remain as documented by the architecture audit.
- **NOT VERIFIED:** runtime SYSRET/IRET behavior and hardware-derived CPL3 evidence.
- **NOT VERIFIED:** the source self-test has not been observed running in a booted kernel and must not be interpreted as Ring 3 certification.
- **NOT IMPLEMENTED:** SMP, APIC/IOAPIC/IPI, per-CPU scheduling, TLB shootdown, integrated watchdogs, or broad Phase 7 certification.
- GUI textbox, bounds, and destroy operations remain explicitly not implemented rather than being silently given unsafe behavior.

## Decision

**Phase 5 source/build hardening: PASS for the changes and canonical build evidence listed above. Full production/runtime Phase 5 certification: NOT VERIFIED.** No Ring 3 or SMP claim is made.
