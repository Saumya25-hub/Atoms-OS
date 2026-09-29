# ATOMS OS — ARCHITECTURE PATCH PLAN (PHASE 5A / TASK 2)
### TARGET: ELIMINATION OF PHYSICAL FREEBSD TRAP 1 (RDTSCP #UD) & SUBSEQUENT TRIPLE FAULT AT GUEST RIP 0xFFFFFFFF80FD18A4

**Protocol Phase**: TASK 2 — ARCHITECT TEAM (RULE 0 Mandatory Phase Isolation)  
**Input Reference**: [`TRIPLE_FAULT_FORENSIC_REPORT.md`](file:///d:/Signatures_OS/TRIPLE_FAULT_FORENSIC_REPORT.md)  
**Target Subsystem**: Intel VMX Secondary Execution Controls (`kernel/core/hypervisor/src/hypervisor.c`)  
**Target Hardware**: Physical Intel Core i3-14100F (Raptor Lake LGA1700) / ASUS PRIME B760M-K  
**Date**: September 30, 2026  
**Status**: ARCHITECTURAL PROPOSAL FOR OPERATOR APPROVAL (STRICTLY READ-ONLY — NO CODE MODIFIED)  

---

## 1. Proven Architectural Defect & Root Cause

### 1.1 The Failure Sequence
1. **Interrupt Storm Resolution**:
   - The Phase 5A Task 3 patch (`sti; nop; cli` interrupt window) successfully eliminated the external-interrupt storm.
   - FreeBSD booted cleanly past `0xFFFFFFFF80FD1D78` (`ucode_load_bsp`), finished `hammer_time()`, and entered `mi_startup()`.
   - Normal boot exit count was recorded: `0x00011D2E` (72,990 exits, not billions).
2. **The Primary Fault (`Fatal trap 1: privileged instruction fault`)**:
   - In `mi_startup()`, `cpu_initclocks()` -> `tsc_calibrate()` called `tc_init(&tsc_timecounter)`.
   - `tc_init()` executed `tscp_get_timecount_low()` at `0xFFFFFFFF80FD18A0`.
   - At `0xFFFFFFFF80FD18A4`, the instruction executed is:
     ```text
     0xFFFFFFFF80FD18A4: 0F 01 F9 -> rdtscp
     ```
   - In `kernel/core/hypervisor/src/hypervisor.c:2024`, `VMCS_SECONDARY_VM_EXEC_CONTROL` was configured as:
     ```c
     uint32_t sec_ctls = adjust_vmx_control(
         (1U << 1) /* Enable EPT */ |
         (1U << 7) /* Unrestricted Guest */ |
         (1U << 20) /* Enable XSAVE/XRSTOR */,
         IA32_VMX_PROCBASED_CTLS2_MSR
     );
     ```
     **Bit 3 (`Enable RDTSCP`, `1U << 3`) was left at 0 (DISABLED).**
   - Meanwhile, in `kernel/core/hypervisor/src/virtual_platform.c:787`, CPUID Leaf `0x80000001` EDX Bit 27 advertised RDTSCP support to the guest:
     ```c
     case 0x80000001:
         *edx = 0x2C100800; /* Bit 27 = RDTSCP */
     ```
   - Per **Intel SDM Vol 3C §24.6.2 & §25.3**, when Secondary Execution Control Bit 3 is 0, any execution of `RDTSCP` by the guest unconditionally generates an **Invalid Opcode Exception (`#UD`)** in guest non-root mode.
   - FreeBSD received `#UD`, mapped it to `trap number = 1` (`Fatal trap 1: privileged instruction fault while in kernel mode`), and panicked.
3. **The Subsequent Triple Fault (Exit Reason 0x02)**:
   - After the panic, FreeBSD waited 15 seconds and invoked its reset sequence: `cpu_reset_real()` in `sys/amd64/amd64/vm_machdep.c`.
   - Because soft-reset methods (ports `0x64`, `0xCF9`, `0x92`) were intercepted without resetting the VM, FreeBSD executed its last-resort CPU shutdown:
     - `lidtq -0x10(%rbp)` (loads IDT with limit 0, base 0)
     - `int3` at `0xFFFFFFFF80FC457E`
   - CPU attempted to deliver `int3` through the 0-limit IDT -> Double Fault (`#DF`) -> attempted to deliver `#DF` through 0-limit IDT -> **TRIPLE FAULT**!
   - Hardware Intel VMX exited with `VMX_EXIT_REASON_TRIPLE_FAULT` (0x02).

---

## 2. VMX Capability MSR Analysis for Secondary Control Bit 3

### 2.1 Capability MSR Specification
- **MSR Index**: `IA32_VMX_PROCBASED_CTLS2_MSR` = `0x0000048B`.
- **Bit 3**: `Enable RDTSCP`.
  - Bits 63:32 contain the **Allowed-1** mask. If Bit 35 (Bit 3 of bits 63:32) is 1, the processor allows `Enable RDTSCP` to be set to 1.
  - Bits 31:0 contain the **Allowed-0 / Forced-1** mask. Bit 3 is 0 (it is not forced to 1).

### 2.2 Silicon Capability Verification
- Intel introduced the "Enable RDTSCP" secondary VM-execution control in Westmere (2010).
- Supported across all Haswell (LGA1150), Skylake, Comet Lake, and Raptor Lake (LGA1700 Core i3-14100F) CPUs.
- In `adjust_vmx_control(ctl, msr)`:
  ```c
  uint64_t vmx_msr = vmm_rdmsr(msr);
  ctl &= (uint32_t)(vmx_msr >> 32); /* allowed-1 */
  ctl |= (uint32_t)vmx_msr;         /* forced-1 */
  return ctl;
  ```
  Passing `(1U << 3)` to `adjust_vmx_control()` is 100% compliant with hardware capability MSR clamping and will be safely preserved on the target silicon.

---

## 3. Approved Files for Modification (TASK 3 Scope)

Per RULE 0: Only **ONE** file is approved for modification in Task 3:

```text
[SINGLE APPROVED FILE]
d:/Signatures_OS/kernel/core/hypervisor/src/hypervisor.c
```

**STRICT PROHIBITION**:
- NO modifications to `virtual_platform.c`.
- NO modifications to CPUID emulation or XSETBV handler.
- NO modifications to the external-interrupt handling (`sti; nop; cli`).
- NO modifications to Realtek RTL8125 driver or VirtIO / vtnet.
- NO modifications to DHCP, DNS, TCP, or HTTPS.
- NO modifications to FreeBSD kernel payload.

---

## 4. Detailed Patch Specification (For Task 3)

### File: `kernel/core/hypervisor/src/hypervisor.c`
- **Function**: `vmx_setup_vmcs(vCPU *vcpu)`
- **Line**: 2024
- **Current Code**:
  ```c
  uint32_t sec_ctls = adjust_vmx_control((1U << 1) /* Enable EPT */ | (1U << 7) /* Unrestricted Guest */ | (1U << 20) /* Enable XSAVE/XRSTOR */, IA32_VMX_PROCBASED_CTLS2_MSR);
  ```
- **Target Replacement Code**:
  ```c
  uint32_t sec_ctls = adjust_vmx_control(
      (1U << 1)  /* Enable EPT */ |
      (1U << 3)  /* Enable RDTSCP */ |
      (1U << 7)  /* Unrestricted Guest */ |
      (1U << 20) /* Enable XSAVE/XRSTOR */,
      IA32_VMX_PROCBASED_CTLS2_MSR
  );
  ```

---

## 5. Architectural Verification of Related Subsystems

### 5.1 CPUID Consistency
- `virtual_platform.c:787` advertises RDTSCP via Leaf `0x80000001` EDX Bit 27 (`0x2C100800`).
- Enabling Secondary Control Bit 3 ensures the hardware execution environment strictly matches the advertised feature set. Zero asymmetry remains.

### 5.2 `TSC_AUX` and MSR `0xC0000103` (`IA32_TSC_AUX`)
- `RDTSCP` writes the value of `IA32_TSC_AUX` into `ECX`.
- In FreeBSD `tscp_get_timecount_low` ([`0xFFFFFFFF80FD18A0`](file:///d:/Signatures_OS/tools/freebsd_payload/kernel.elf#L80fd18a0)):
  ```asm
  ffffffff80fd18a4: 0f 01 f9    rdtscp
  ffffffff80fd18a7: 8b 4f 30    movl    0x30(%rdi), %ecx
  ffffffff80fd18aa: 0f ad d0    shrdl   %cl, %edx, %eax
  ffffffff80fd18ad: 5d          popq    %rbp
  ffffffff80fd18ae: c3          retq
  ```
  FreeBSD immediately overwrites `ECX` with `0x30(%rdi)` (`tsc_shift`). The value placed into `ECX` by `RDTSCP` is discarded.
- In `virtual_platform.c`, `virtual_platform_handle_rdmsr()` defaults to `*val = 0; return true;` and `virtual_platform_handle_wrmsr()` defaults to `return true;`. Any explicit read/write of MSR `0xC0000103` succeeds without fault.
- No additional MSR infrastructure or TSC offsetting is required.

### 5.3 Interaction with Root-Mode Interrupt Window (`sti; nop; cli`)
- The external-interrupt fix in `case VMX_EXIT_REASON_EXTERNAL_INTR:` remains completely untouched and active.
- Enabling `RDTSCP` allows FreeBSD to complete `tsc_calibrate()` and `tc_init()`, while the interrupt window continues to service periodic PIT/IRQ0 host timer interrupts safely in the background.

---

## 6. Regression Risk Assessment

| Subsystem | Impact | Risk Level | Mitigation / Proof |
| :--- | :--- | :--- | :--- |
| **VM-Entry Pre-Flight** | Adds Bit 3 to `sec_ctls` | **ZERO** | Bit 3 is fully supported in `IA32_VMX_PROCBASED_CTLS2_MSR` on Raptor Lake / Haswell. Clamped by `adjust_vmx_control`. |
| **CPUID** | None | **ZERO** | CPUID is not modified; already advertises Bit 27. |
| **XSETBV** | None | **ZERO** | Bit 20 (`Enable XSAVE/XRSTOR`) is preserved. |
| **Interrupt Handling** | None | **ZERO** | `pin_ctls` Bit 0 and `sti; nop; cli` are untouched. |
| **FreeBSD Execution** | Eliminates `#UD` Trap 1 | **POSITIVE** | Prevents panic, prevents `cpu_reset_real()`, allows `mi_startup()` to continue. |

---

## 7. Expected Verification Telemetry Following Patch

1. **Trap 1 Elimination**:
   - `Fatal trap 1: privileged instruction fault` at `0xFFFFFFFF80FD18A4` will NOT occur.
2. **Panic & Reboot Elimination**:
   - `panic: privileged instruction fault` will NOT occur.
   - `cpu_reset_real()` will NOT be called.
   - Triple Fault (`Exit Reason 0x0002` at `0xFFFFFFFF80FC457E`) will NOT occur.
3. **FreeBSD Advancement**:
   - FreeBSD will complete `tc_init()` and output:
     `Timecounter "TSC" frequency ... Hz quality 1000`
   - Execution will advance to device configuration, PCI probing, and VirtIO-Net (`vtnet0`) attachment!

---

## 8. Rollback Plan

```bash
git checkout kernel/core/hypervisor/src/hypervisor.c
```
Reverts `hypervisor.c` to pre-patch state.
