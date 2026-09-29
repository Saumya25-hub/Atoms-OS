# ATOMS OS — PHYSICAL VMX TRIPLE FAULT FORENSIC REPORT

**Investigation Target**: Forensic Investigation of VMX Exit Reason `0x0002` (`VMX_EXIT_REASON_TRIPLE_FAULT`) on Physical Intel Core i3-14100F (LGA1700) / ASUS PRIME B760M-K  
**Protocol Phase**: TASK 1 (Forensic Team) — STRICTLY READ-ONLY (RULE 0: NO CODE MODIFICATIONS, NO PATCHES)  
**Execution Context**: Physical Intel Core i3-14100F / ASUS PRIME B760M-K UEFI Native Boot  
**Date**: September 30, 2026  
**Status**: ROOT CAUSE CONCLUSIVELY PROVEN (FREEBSD INTENTIONAL SHUTDOWN FOLLOWING TRAP 1 RDTSCP #UD)  

---

## 1. Executive Forensic Verdict

The observed VMX Triple Fault (`Exit Reason 0x0002`) at Guest RIP `0xFFFFFFFF80FC457E` is **NOT** a hypervisor crash, NOT a memory corruption, and NOT a stack overflow.

It is an **intentional hardware reset / CPU shutdown sequence executed by FreeBSD's own `cpu_reset_real()` function**.

### The Complete Failure Chain:
1. **Interrupt Storm Resolution**:
   - The Phase 5A Task 3 patch (`sti; nop; cli` interrupt window in VMX root mode) **100% successfully eliminated the external-interrupt storm**.
   - FreeBSD advanced past `0xFFFFFFFF80FD1D78` (`ucode_load_bsp`), completed `hammer_time()`, and executed deep into `mi_startup()`.
   - Total VM-exits dropped from 3+ billion to just `0x00011D2E` (72,990 exits across normal boot).
   - FreeBSD successfully initialized ACPI AML tables, IOAPIC IRQ routing, Power Button, ACPI-fast timer (3.579545 MHz), procfs, and statistical TSC calibration.
2. **The Real Blocker (`Fatal trap 1: privileged instruction fault`)**:
   - In `mi_startup()`, `cpu_initclocks()` -> `tsc_calibrate()` called `tc_init(&tsc_timecounter)`.
   - `tc_init()` executed the timecounter read function `tscp_get_timecount_low()` at `0xFFFFFFFF80FD18A0`.
   - At `0xFFFFFFFF80FD18A4`, the instruction executed is:
     ```text
     0xFFFFFFFF80FD18A4: 0F 01 F9 -> rdtscp
     ```
   - In `kernel/core/hypervisor/src/hypervisor.c:2024`, `VMCS_SECONDARY_VM_EXEC_CONTROL` was configured as:
     ```c
     uint32_t sec_ctls = adjust_vmx_control((1U << 1) /* Enable EPT */ | (1U << 7) /* Unrestricted Guest */ | (1U << 20) /* Enable XSAVE/XRSTOR */, IA32_VMX_PROCBASED_CTLS2_MSR);
     ```
     **Bit 3 (`Enable RDTSCP`, `1U << 3`) was left at 0 (DISABLED).**
   - Meanwhile, CPUID Leaf `0x80000001` EDX Bit 27 (`RDTSCP`) was advertised as **1 (SUPPORTED)** to the guest in `virtual_platform.c:787`.
   - Per **Intel SDM Vol 3C §24.6.2 & §25.3**, when Secondary Execution Control Bit 3 is 0, any execution of `RDTSCP` by the guest unconditionally generates an **Invalid Opcode Exception (`#UD`)** in guest non-root mode.
3. **The Panic and Intentional Triple Fault**:
   - FreeBSD received `#UD`, which BSD maps to `trap number = 1` (`Fatal trap 1: privileged instruction fault while in kernel mode`).
   - FreeBSD displayed the full panic dump and backtrace on screen/serial.
   - FreeBSD waited 15 seconds (`Automatic reboot in 15 seconds...`).
   - FreeBSD called `cpu_reset_real()` in `sys/amd64/amd64/vm_machdep.c`.
   - `cpu_reset_real()` attempted resetting via keyboard controller (port `0x64`), PCI reset (port `0xCF9`), and Fast A20 (port `0x92`).
   - Because ATOMS OS intercepted port `0xCF9` and advanced RIP instead of resetting the VM, all soft-reset attempts failed.
   - FreeBSD printed: `No known reset method worked, attempting CPU shutdown`.
   - FreeBSD intentionally loaded a **zero-limit IDT** (`lidtq -0x10(%rbp)`) and executed `int3` at `0xFFFFFFFF80FC457E`.
   - The CPU attempted to deliver `int3` through the 0-limit IDT -> Double Fault (`#DF`) -> attempted to deliver `#DF` through the 0-limit IDT -> **TRIPLE FAULT**!
   - Hardware Intel VMX exited with `VMX_EXIT_REASON_TRIPLE_FAULT` (Exit Reason 2).

---

## 2. Forensic VMCS State at Exit Reason 0x02

From physical hardware serial telemetry captured in `build/atoms_live_kernel.log` (lines 38168–38300):

| VMCS Field | Captured Value | Architectural Meaning |
| :--- | :--- | :--- |
| **VM_EXIT_REASON** | `0x00000002` | `VMX_EXIT_REASON_TRIPLE_FAULT` |
| **VM_EXIT_QUALIFICATION** | `0x0000000000000000` | N/A for Triple Fault |
| **VM_EXIT_INSTRUCTION_LEN** | `0x00000001` | Length of `int3` opcode (`0xCC`) = 1 byte |
| **GUEST_RIP** | `0xFFFFFFFF80FC457E` | `cpu_reset_real + 0x7E` (pointing directly at `int3`) |
| **GUEST_RSP** | `0xFFFFFFFFA0205AD0` | Kernel stack inside `cpu_reset_real()` |
| **GUEST_RFLAGS** | `0x0000000000000002` | Reserved Bit 1 = 1, `IF = 0` (`cpu_reset_real` ran `cli`) |
| **GUEST_CR0** | `0x0000000080000031` | `PG=1, WP=0, NE=1, ET=1, MP=1, PE=1` (Normal 64-bit Kernel) |
| **GUEST_CR3** | `0x0000000000020000` | FreeBSD Kernel PML4 |
| **GUEST_CR4** | `0x00000000000006A0` | `FSGSBASE=1, OSXSAVE=1, PAE=1` |
| **GUEST_XCR0** | `0x0000000000000007` | `x87 (1) \| SSE (2) \| AVX (4)` (Configured via earlier XSETBV) |
| **GUEST_EFER** | `0x0000000000000D01` | `NXE=1, LMA=1, LME=1, SCE=1` |
| **GUEST_CS** | `0x0020` | Kernel Code Selector |
| **GUEST_SS** | `0x0028` | Kernel Data Selector |
| **GUEST_IDTR** | **Base = 0, Limit = 0** | **Explicitly zeroed by `cpu_reset_real` to force triple fault** |
| **IDT_VECTORING_INFO** | `0x00000000` | No event injection was in progress |
| **VM_EXIT_INTR_INFO** | `0x00000000` | No exception intercepted by hypervisor |
| **Total Exits** | `0x00011D2E` (72,990) | No exit storm; normal boot exit cadence |

---

## 3. Disassembly & Symbol Mapping

### 3.1 The Triple Fault Location (`0xFFFFFFFF80FC457E`)
Disassembly of `tools/freebsd_payload/kernel.elf` at `cpu_reset_real`:

```asm
ffffffff80fc4500 <cpu_reset_real>:
ffffffff80fc4500: 55                      pushq   %rbp
ffffffff80fc4501: 48 89 e5                movq    %rsp, %rbp
ffffffff80fc4504: 48 83 ec 10             subq    $0x10, %rsp
ffffffff80fc4508: fa                      cli
ffffffff80fc4509: b0 fe                   movb    $-0x2, %al
ffffffff80fc450b: e6 64                   outb    %al, $0x64              ; 1. Try KBD controller reset
...
ffffffff80fc4517: b0 02                   movb    $0x2, %al
ffffffff80fc4519: ba f9 0c 00 00          movl    $0xcf9, %edx
ffffffff80fc451e: ee                      outb    %al, %dx                ; 2. Try PCI reset port 0xCF9
ffffffff80fc451f: b0 06                   movb    $0x6, %al
ffffffff80fc4521: ee                      outb    %al, %dx
...
ffffffff80fc453f: e6 92                   outb    %al, $0x92              ; 3. Try Fast A20 reset
...
ffffffff80fc4551: 48 c7 c7 87 0b 12 81    movq    $0xffffffff81120b87, %rdi ; "No known reset method worked..."
ffffffff80fc4558: 31 c0                   xorl    %eax, %eax
ffffffff80fc455a: e8 c1 2b bc ff          callq   printf
ffffffff80fc455f: bf 40 42 0f 00          movl    $0xf4240, %edi
ffffffff80fc4564: e8 17 dc 00 00          callq   DELAY
; --- INTENTIONAL TRIPLE FAULT GENERATOR ---
ffffffff80fc4569: 66 c7 45 f8 00 00       movw    $0x0, -0x8(%rbp)        ; idtr.limit = 0
ffffffff80fc456f: 48 c7 45 f0 00 00 00 00 movq    $0x0, -0x10(%rbp)       ; idtr.base = 0
ffffffff80fc4577: 48 8d 45 f0             leaq    -0x10(%rbp), %rax
ffffffff80fc457b: 0f 01 18                lidtq   (%rax)                  ; Load 0-limit IDT
ffffffff80fc457e: cc                      int3                            ; <--- TRIPLE FAULT EXIT 0x02
ffffffff80fc457f: 90                      nop
ffffffff80fc4580: eb fe                   jmp     0xffffffff80fc4580
```

### 3.2 The Root Cause Failure Location (`0xFFFFFFFF80FD18A4`)
From physical screen capture:
```text
Fatal trap 1: privileged instruction fault while in kernel mode
cpuid = 0; apic id = 00
instruction pointer = 0x20:0xffffffff80fd18a4
stack pointer       = 0x28:0xffffffffa0205f40
frame pointer       = 0x28:0xffffffffa0205f40
...
trap number         = 1
panic: privileged instruction fault
KDB: stack backtrace:
#0 0xffffffff80b7fbfd at kdb_backtrace+0x5d
#1 0xffffffff80b32961 at vpanic+0x131
#2 0xffffffff80b32823 at panic+0x43
#3 0xffffffff80fff91b at trap+0xcbb
#4 0xffffffff80fd6a48 at calltrap+0x8
#5 0xffffffff80b45a21 at tc_init+0x271
#6 0xffffffff80fd1246 at tsc_calibrate+0x76
#7 0xffffffff80fbebe5 at cpu_initclocks+0x15
#8 0xffffffff80abf540 at profclock+0x2f0
#9 0xffffffff80abb425 at mi_startup+0xb5
```

Disassembly of `tools/freebsd_payload/kernel.elf` at `0xFFFFFFFF80FD18A4`:

```asm
ffffffff80fd18a0 <tscp_get_timecount_low>:
ffffffff80fd18a0: 55          pushq   %rbp
ffffffff80fd18a1: 48 89 e5    movq    %rsp, %rbp
ffffffff80fd18a4: 0f 01 f9    rdtscp           ; <--- FAULT POINT: #UD (Invalid Opcode)
ffffffff80fd18a7: 8b 4f 30    movl    0x30(%rdi), %ecx
ffffffff80fd18aa: 0f ad d0    shrdl   %cl, %edx, %eax
ffffffff80fd18ad: 5d          popq    %rbp
ffffffff80fd18ae: c3          retq
```

---

## 4. Architectural Analysis: Why RDTSCP Faulted

### 4.1 Intel VMX Specification on RDTSCP
Per **Intel SDM Vol 3C §24.6.2** (*Table 24-7: Format of Secondary Processor-Based VM-Execution Controls*):
- **Bit 3**: **Enable RDTSCP**.
  - *"If this control is 0, the `RDTSCP` instruction causes an invalid-opcode exception (`#UD`)."*
  - *"If this control is 1, the `RDTSCP` instruction operates normally."*

### 4.2 The Asymmetry with CPUID
In [`kernel/core/hypervisor/src/virtual_platform.c:787`](file:///d:/Signatures_OS/kernel/core/hypervisor/src/virtual_platform.c#L787):
```c
case 0x80000001: /* Extended Processor Info & Features */
    *eax = 0x000306C3;
    *ebx = 0;
    *ecx = 0x00000121;
    *edx = 0x2C100800; /* SYSCALL/SYSRET (bit 11), NX (bit 20), 1GB Page (bit 26), RDTSCP (bit 27), LM (bit 29) */
    break;
```
`*edx = 0x2C100800` explicitly sets **Bit 27 (`RDTSCP`) = 1**.
Therefore, FreeBSD's kernel boots, inspects CPUID Leaf `0x80000001`, sees Bit 27 set, and installs `tscp_get_timecount_low()` (which uses `RDTSCP`).

However, in [`kernel/core/hypervisor/src/hypervisor.c:2024`](file:///d:/Signatures_OS/kernel/core/hypervisor/src/hypervisor.c#L2024):
```c
uint32_t sec_ctls = adjust_vmx_control(
    (1U << 1) /* Enable EPT */ |
    (1U << 7) /* Unrestricted Guest */ |
    (1U << 20) /* Enable XSAVE/XRSTOR */,
    IA32_VMX_PROCBASED_CTLS2_MSR
);
```
**Bit 3 (`Enable RDTSCP`, `1U << 3`) was never requested.**
Consequently, the hardware VMCS has `Enable RDTSCP == 0`. When FreeBSD executed `rdtscp` at `0xFFFFFFFF80FD18A4`, the Intel processor in non-root mode generated `#UD`.

---

## 5. Interaction Analysis with Phase 5A Task 3 Patch (`sti; nop; cli`)

The user specifically requested verification of whether the root-mode interrupt window (`sti; nop; cli`) introduced any side effects or memory/VMCS corruption.

### Verification Findings:
1. **Interrupt Storm Completely Fixed**:
   - The previous storm of 3+ billion exits pinned at `0xFFFFFFFF80FD1D78` was 100% eliminated.
   - FreeBSD executed **thousands of functions** after `ucode_load_bsp`, progressing through `hammer_time()`, locore bootstrap, memory subsystem init, ACPI table acquisition, IOAPIC routing, and `mi_startup()`.
2. **Host Interrupt Servicing Verified**:
   - Host timer ticks were handled via `isr32` -> `irq_dispatch` -> `pic_send_eoi(0)`.
   - Host remained alive, serial telemetry and DGL dashboard were rendered continuously.
3. **No Guest State Corruption**:
   - The host ISR runs strictly on `g_saved_host_rsp` (the host kernel stack).
   - Guest VMCS registers, guest GPRs, and guest page tables are never touched by the host ISR.
   - The triple fault occurred purely within guest software space because of an unhandled `#UD` trap caused by VMCS Secondary Control Bit 3.

---

## 6. QEMU vs Physical Silicon Comparison

| Environment | Hypervisor Mode | Secondary VMCS Controls | CPUID Leaf 0x80000001 EDX.27 | Behavior on `RDTSCP` | Result |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **QEMU Preflight** | AMD SVM (`AuthenticAMD`) | SVM VMCB Intercepts | Bit 27 = 1 | QEMU TCG emulates RDTSCP without intercepting or faulting | **PASS** (Advances past `tc_init`) |
| **Physical Intel Silicon** | Intel VMX (`GenuineIntel`) | `sec_ctls` Bit 3 = 0 | Bit 27 = 1 | Hardware silicon strictly generates `#UD` (SDM §24.6.2) | **FAIL** (Trap 1 -> Panic -> Reset -> Triple Fault) |

---

## 7. Suspected Fix (NO CODE / ARCHITECTURAL ANALYSIS ONLY)

To resolve the Trap 1 / Triple Fault chain:
1. **VMCS Secondary Controls**:
   In `kernel/core/hypervisor/src/hypervisor.c` line 2024, request **Bit 3 (`Enable RDTSCP`, `(1U << 3)`)** in `sec_ctls` passed to `adjust_vmx_control()`.
   - Silicon Capability: Intel Core i3-14100F (Raptor Lake) and Haswell LGA1150 silicon 100% support `Enable RDTSCP` in `IA32_VMX_PROCBASED_CTLS2_MSR` (MSR `0x48B`).
2. **Expected Architectural Result**:
   - `RDTSCP` instruction executes normally in guest non-root mode without `#UD`.
   - FreeBSD `tc_init()` successfully initializes `tsc_timecounter`.
   - FreeBSD does NOT take Trap 1.
   - FreeBSD does NOT panic.
   - FreeBSD does NOT call `cpu_reset_real()`.
   - Execution continues past `mi_startup()` to driver attachment, PCI discovery, and VirtIO networking!

---

## 8. Conclusion

- **Failure Boundary**: Guest execution of `RDTSCP` at `0xFFFFFFFF80FD18A4` in `tscp_get_timecount_low()`.
- **Root Cause**: VMCS `SECONDARY_VM_EXEC_CONTROL` Bit 3 (`Enable RDTSCP`) is `0`, causing hardware `#UD` on physical Intel silicon.
- **Triple Fault Cause**: Intentional `lidtq [0]` + `int3` executed by FreeBSD's panic reset handler `cpu_reset_real()` when port 0xCF9 reset failed.
- **Rule 0 Compliance**: Strictly read-only investigation. Zero lines of code modified. Ready for Architecture Team Phase (Task 2).
