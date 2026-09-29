# ATOMS OS — ARCHITECTURE VERIFICATION REPORT: XSETBV (VM-EXIT 55)

**Audit Target**: Pre-Patch Architecture Verification of Proposed XSETBV (VM-Exit 55) Handling  
**Protocol Phase**: TASK 3 PRE-PATCH VERIFICATION (RULE 0 Mandatory Phase Isolation)  
**Input Reference**: [`FORENSIC_REPORT.md`](file:///d:/Signatures_OS/FORENSIC_REPORT.md), [`PATCH_PLAN.md`](file:///d:/Signatures_OS/PATCH_PLAN.md)  
**Target Hardware**: ASUS PRIME B760M-K (Intel Core i3-14100F, Haswell-Raptor Lake x86_64) & Haswell H81  
**Target Guest**: FreeBSD 14.1-RELEASE amd64 ELF Kernel (`locore.S` -> `hammer_time` -> `fpuinit()`)  
**Audit Mode**: STRICT 100% READ-ONLY (Zero Code Modified, Zero Builds, Zero Reboots)  
**Date**: September 30, 2026  

---

## 1. Executive Summary & Verdict on Proposed Design

### The Core Architectural Question:
> *Is the naive proposed approach: `"execute physical XSETBV using guest EDX:EAX, then advance Guest RIP by 3"` architecturally safe in this hypervisor?*

### Formal Architectural Verdict:
**NO. The unvalidated naive execution of physical `xsetbv` with guest registers is NOT architecturally safe.**

### Rationale:
1. **Host `#GP(0)` Panic Vulnerability**: If guest software passes an invalid register index (`ECX != 0`) or an invalid feature mask (e.g., Bit 0 cleared, or unsupported bits set), executing `xsetbv` in host CPL 0 triggers a **Host CPU General Protection Fault (`#GP(0)`)**, immediately crashing the ATOMS OS micro-hypervisor.
2. **Host Task Extended State Degradation**: If guest software were to execute `xsetbv` with a reduced mask (such as `0x3`, disabling AVX), modifying physical `XCR0` would cause the host kernel's `g_cpu_ext_engine` (which saves task extended context using mask `0x7`) to take a fatal `#GP(0)` on subsequent host task switches.
3. **Physical Host XCR0 is Already 0x7**: The ATOMS OS host kernel already configures physical `XCR0` to `0x7` (`x87 | SSE | AVX`) during early initialization in `arch/x86_64/cpu/cpu_features.c`.

### The Certified Architecturally Safe Approach:
The hypervisor must:
1. **Validate guest operands against Intel SDM rules and CPUID Leaf 0xD specifications** before touching hardware or state.
2. **Track guest XCR0 in the `vCPU` structure (`vcpu->xcr0`)**.
3. **Assert physical `XCR0` synchronization only when the validated mask is compatible with host capabilities (`0x7`)**.
4. **Advance `Guest RIP` by instruction length (3 bytes: `0F 01 D1`) and resume guest execution cleanly**.

---

## 2. Detailed Technical Audit of the 8 Verification Questions

### 1. Does ATOMS currently maintain a guest XCR0 value per vCPU?
- **Finding**: **NO**.
- **Evidence**:
  - In `kernel/core/hypervisor/include/hypervisor.h` (lines 96–126), `struct vcpu` maintains:
    - Guest General Purpose Registers: `GuestCpuRegisters guest_regs;`
    - Guest Control Registers & MSRs: `cr0`, `cr3`, `cr4`, `efer`, `msr_star`, `msr_lstar`, `msr_fmask`, `msr_fs_base`, `msr_gs_base`, `msr_kernel_gs_base`.
  - There is currently **no `xcr0` field** in `struct vcpu`.
  - In `kernel/core/hypervisor/src/virtual_platform.c`, `xcr0` is only referenced as static constants in CPUID Leaf 0xD emulation.
- **Architectural Recommendation**: Add `uint64_t xcr0;` to `struct vcpu` in `hypervisor.h`. Initialize it to `0x00000001` (architectural reset value: x87 only) during `atoms_vcpu_create()`.

---

### 2. Is host XCR0 currently modified anywhere during VMX guest execution?
- **Finding**: **NO**.
- **Evidence**:
  - A workspace-wide grep reveals that `xsetbv` is executed in exactly **one** location:
    `arch/x86_64/cpu/cpu_features.c:143`:
    ```c
    uint64_t xcr0 = (1ULL << 0) | (1ULL << 1); // Enable x87 and SSE
    if (g_cpu_features.has_avx && (g_cpu_features.xfeature_supported_mask & (1ULL << 2))) {
        xcr0 |= (1ULL << 2); // Enable AVX
    }
    uint32_t low = (uint32_t)(xcr0 & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(xcr0 >> 32);
    __asm__ volatile ("xsetbv" :: "a"(low), "d"(high), "c"(0));
    ```
  - This executes once during early bootstrap of the host OS.
  - Throughout VMX initialization, VMCS setup, guest vCPU launch, and VM-exit dispatching, host XCR0 is **never modified**.

---

### 3. Is VMX/XCR0 state saved or restored around VM entry/exit?
- **Finding**: **NO**.
- **Evidence**:
  - In `kernel/core/hypervisor/src/vmx_entry.asm`:
    - `vmx_run_vcpu_raw`: Saves host callee-saved GPRs (`RBX`, `RBP`, `R12`–`R15`, `RDI`) and host `RSP`. Loads guest GPRs (`RAX`–`R15`). Executes `vmlaunch`/`vmresume`.
    - `vmx_vmexit_handler`: Saves guest GPRs into `g_active_guest_regs`. Restores host `RSP` and callee-saved GPRs. Returns to C.
  - The CPU hardware automatically restores Host CR0, CR3, CR4, RSP, and RIP from VMCS host-state area.
  - **Neither XCR0 nor extended FPU/SSE/AVX state (`XSAVE`/`XRSTOR`) is saved or restored across VM boundaries.**

---

### 4. Does Intel VMX provide any existing mechanism/configuration in this implementation for guest XCR0 handling?
- **Finding**: **NO hardware mechanism exists in Intel VMX**.
- **Evidence**:
  - Per Intel SDM (Vol 3C, Chapters 24–27):
    - The VMCS control structure **has neither a `GUEST_XCR0` nor a `HOST_XCR0` field**.
    - VM-entry and VM-exit controls do not support automated hardware loading/saving of XCR0.
    - Secondary Processor-Based Execution Control Bit 20 is "Enable XSAVES/XRSTORS" (enabling supervisor extended state instructions), not automated XCR0 swapping.
    - Intel architecture explicitly defines `XSETBV` in VMX non-root operation as an **unconditional VM-exit** (Exit Reason 55: `VMX_EXIT_REASON_XSETBV`).
  - Therefore, the hypervisor software must handle Exit 55 explicitly.

---

### 5. Would simply executing host XSETBV with the guest value corrupt or alter host CPU execution state?
- **Finding**: **YES, it is potentially catastrophic if unvalidated, but safe if verified against host mask `0x7`**.
- **Evidence & Breakdown**:
  - **Scenario A (Guest requests invalid mask or index)**:
    If FreeBSD or malicious guest code executes `xsetbv` with `ECX != 0`, Bit 0 = 0, or reserved bits set, executing `xsetbv` in host mode triggers an unhandled **Host `#GP(0)` exception**, crashing the entire host kernel immediately.
  - **Scenario B (Guest requests downgraded mask, e.g., `0x3`)**:
    If the guest disables AVX (`XCR0 = 0x3`), and the host sets physical XCR0 to `0x3`, the host kernel's `g_cpu_ext_engine` (in `kernel/core/cpu/cpu_state.c:204`, which uses `xfeature_mask = 0x7`) will execute `xsave_raw` with Bit 2 set while physical XCR0 has Bit 2 cleared. Per Intel SDM, executing `XSAVE` with an instruction mask bit not enabled in XCR0 generates `#GP(0)`.
  - **Scenario C (FreeBSD's actual request: `0x7`)**:
    FreeBSD is requesting `XCR0 = 0x0000000000000007ULL` (`x87 | SSE | AVX`). The host physical XCR0 is **already `0x7`**. Therefore, validating the request and keeping physical XCR0 at `0x7` creates zero divergence between host and guest.

---

### 6. What validation is required for guest XCR0 according to the currently implemented CPUID XSAVE feature set?
- **Finding**:
  In `kernel/core/hypervisor/src/virtual_platform.c` (lines 730–765), ATOMS emulates CPUID Leaf `0x0D`:
  - Subleaf 0: Valid lower 32-bit XCR0 mask = `0x00000007` (`x87 (1) | SSE (2) | AVX (4)`). Upper 32-bit mask = `0x00000000`.
- **Mandatory Architectural Validation Rules (Intel SDM Vol 2D, `XSETBV`)**:
  1. **Register Index**: `vcpu->guest_regs.rcx == 0`. (If `RCX != 0` -> `#GP(0)`).
  2. **Reserved / Unsupported Bits**: `((val & ~0x00000007ULL) == 0)`. High 32 bits (`RDX`) must be `0`; low 32 bits (`RAX`) must have no bits set outside `0x7`.
  3. **Mandatory x87 State**: `(val & (1ULL << 0)) != 0`. Bit 0 must always be 1. (Clearing Bit 0 -> `#GP(0)`).
  4. **AVX/SSE State Dependency**: If Bit 2 (AVX) is set, Bit 1 (SSE) must also be set: `((val & (1ULL << 2)) != 0) ==> ((val & (1ULL << 1)) != 0)`. (AVX without SSE -> `#GP(0)`).

---

### 7. Can FreeBSD's requested XCR0=0x7 safely be represented as guest state without modifying the host's XCR0?
- **Finding**: **YES, 100% SAFELY**.
- **Evidence**:
  - The physical processor's XCR0 is already set to `0x7` by `arch/x86_64/cpu/cpu_features.c` during host boot.
  - When FreeBSD runs in VMX non-root mode:
    - `xgetbv(0)` executed by the guest reads physical XCR0, which is `0x7`.
    - `xsave` / `xrstor` executed by the guest with mask `0x7` matches physical XCR0 (`0x7`), executing cleanly without faults.
    - AVX/SSE instructions executed by the guest check physical XCR0 bits 1 and 2, which are active (`0x7`), executing natively without `#UD`.
  - Storing `vcpu->xcr0 = 0x7` satisfies the guest's architectural state requirement without mutating host configuration.
  - Re-asserting `xsetbv(0, 0x7)` in the hypervisor is also completely idempotent and safe because it writes the identical value already present in hardware.

---

### 8. Does the existing hypervisor already have XSAVE/XRSTOR support that can be reused?
- **Finding**: **PARTIALLY**.
- **Evidence**:
  - `kernel/core/cpu/cpu_state.c` and `kernel/core/cpu/cpu_state.h` provide complete, tested, 64-byte aligned XSAVE/XRSTOR implementations:
    - `xsave_raw(void *buf, uint64_t mask)`
    - `xrstor_raw(const void *buf, uint64_t mask)`
    - `cpu_extended_state_save(Task *task)`
    - `cpu_extended_state_restore(Task *task)`
  - However, the hypervisor subsystem (`kernel/core/hypervisor`) does not currently invoke these functions across VM-entry/exit. The hypervisor host kernel is built with `-msoft-float -mno-sse` (as verified in `build.ps1`), meaning the host kernel does not use or corrupt FPU/SSE/AVX registers during VM-exit handling.
  - Therefore, full extended state swapping across VM boundaries is **not required** for single-vCPU guest execution, but the low-level routines are fully available if needed in the future.

---

## 3. Architecture Conclusion & Approved Design for TASK 3

The architecture verification confirms that the fix for Exit 55 (`VMX_EXIT_REASON_XSETBV`) must not simply execute a naked `xsetbv` with raw guest registers.

Instead, the patch must implement:
1. Define `#define VMX_EXIT_REASON_XSETBV 55` in `kernel/core/hypervisor/include/vmx.h`.
2. Add `uint64_t xcr0;` to `struct vcpu` in `kernel/core/hypervisor/include/hypervisor.h`.
3. In `kernel/core/hypervisor/src/hypervisor.c:atoms_vmexit_dispatch()`, implement `case VMX_EXIT_REASON_XSETBV:`:
   - Extract `reg_idx = vcpu->guest_regs.rcx` and `val = ((uint64_t)vcpu->guest_regs.rdx << 32) | (uint32_t)vcpu->guest_regs.rax`.
   - Validate `reg_idx == 0`, `(val & 1) != 0`, `((val & 4) == 0 || (val & 2) != 0)`, and `(val & ~0x7ULL) == 0`.
   - If invalid: log error and halt safely (or inject `#GP(0)`).
   - If valid: record `vcpu->xcr0 = val;`, synchronize physical XCR0 via `xsetbv(0, val)` (which is `0x7`), advance `vcpu->guest_regs.rip += vcpu->last_exit.instruction_length` (3 bytes), set `handled = true; disposition = VMEXIT_HANDLED_AND_RESUME;`, and return `true`.
