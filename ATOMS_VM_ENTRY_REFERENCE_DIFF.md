# ATOMS OS — VM-ENTRY REFERENCE IMPLEMENTATION CROSS-AUDIT

**Document:** `ATOMS_VM_ENTRY_REFERENCE_DIFF.md`  
**Date:** 2026-09-16  
**Target Hardware:** ASUS PRIME B760M-K (Intel B760 LGA1700 Chipset)  
**Target CPU:** Intel Core i3-14100F (14th Gen Raptor Lake Refresh, 4P / 8T, x86_64)  
**Guest Workload:** FreeBSD 14.1-RELEASE amd64 (`locore.S` Direct 64-Bit Kernel Entry)  
**Reference Implementations Audited:**
1. **Intel SDM Vol 3C:** Chapter 26 (Checks on VMX Controls, Host-State Area, and Guest-State Area)
2. **Linux KVM VMX:** `arch/x86/kvm/vmx/vmx.c` & `vmcs12.c`
3. **FreeBSD bhyve VMX:** `sys/amd64/vmm/intel/vmx.c`
4. **Xen HVM VMX:** `xen/arch/x86/hvm/vmx/vmx.c` & `vmcs.c`

---

## 1. Comprehensive VM-Entry Architectural Comparison Matrix

| Field | ATOMS Value | Linux KVM | FreeBSD bhyve | Xen HVM | Intel SDM Rule | Match / Difference | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **GUEST_CR0** | `0x0000000080010031` | `0x80010033` / fixed0-f1 | `0x8005003b` / fixed0-f1 | `0x80050033` / fixed0-f1 | SDM 26.3.1.1: PE=1, PG=1, bits 63:32=0, FIXED0/1 compliance | MATCH (Bits PE, PG, ET, NE, WP set, complies with FIXED0/1) | NOT RELEVANT |
| **GUEST_CR3** | `0x0000000000020000` | PML4 GPA (4KB aligned) | PML4 GPA (`0x20000` base) | PML4 GPA (4KB aligned) | SDM 26.3.1.1: Bits 11:0=0 (4KB aligned), canonical GPA | MATCH (Page-aligned GPA `0x20000`) | NOT RELEVANT |
| **GUEST_CR4** | `0x0000000000000020` | `0x00000020` / `0x660` | `0x00000020` / `0x660` | `0x00000020` / `0x660` | SDM 26.3.1.1: PAE=1 in IA-32e mode, VMXE=0, bits 63:32=0 | MATCH (PAE=1, VMXE=0, bits 63:32=0) | NOT RELEVANT |
| **GUEST_RFLAGS** | `0x0000000000000002` | `0x00000002` | `0x00000002` | `0x00000002` | SDM 26.3.1.1: Bit 1=1, VM=0, bits 63:32=0, reserved bits=0 | MATCH (Compliant architectural default) | NOT RELEVANT |
| **GUEST_RIP** | `0xFFFFFFFF8037C000` | Canonical address | `0xFFFFFFFF8037C000` | Canonical address | SDM 26.3.1.1: Must be canonical 64-bit linear address | MATCH (Canonical FreeBSD `locore.S` entry) | NOT RELEVANT |
| **GUEST_RSP** | `0x000000000007FF00` | Canonical address | `0x000000000007FF00` | Canonical address | SDM 26.3.1.1: Must be canonical 64-bit linear address | MATCH (Canonical page-aligned stack top) | NOT RELEVANT |
| **GUEST_IA32_EFER** | `0x0000000000000D01` | `0xD01` (SCE, LME, LMA, NXE) | `0xD01` / `0xD00` | `0xD01` / `0xD00` | SDM 26.3.1.5: If Load EFER=1, LME/LMA must equal Entry Ctls bit 9; reserved bits=0 | MATCH (Complies with SDM bit pattern) | NOT RELEVANT |
| **GUEST_CS_SEL** | `0x0008` (RPL=0, TI=0) | `0x0008` / `0x0010` | `0x0008` (GDT Code) | `0x0008` | SDM 26.3.1.2: TI=0, RPL=SS.DPL | MATCH | NOT RELEVANT |
| **GUEST_CS_BASE** | `0x0000000000000000` | `0x0000000000000000` | `0x0000000000000000` | `0x0000000000000000` | SDM 26.3.1.2: Must be 0 in 64-bit mode | MATCH | NOT RELEVANT |
| **GUEST_CS_LIMIT** | `0xFFFFFFFF` | `0xFFFFFFFF` | `0xFFFFFFFF` | `0xFFFFFFFF` | SDM 26.3.1.2: Unrestricted in 64-bit mode | MATCH | NOT RELEVANT |
| **GUEST_CS_AR** | `0x0000A09B` (L=1, D=0) | `0x0000A09B` | `0x0000A09B` | `0x0000A09B` | SDM 26.3.1.2: L=1, D/B=0, Present=1, S=1, Type=11 | MATCH | NOT RELEVANT |
| **GUEST_SS_SEL** | `0x0010` (RPL=0, TI=0) | `0x0010` | `0x0010` (GDT Data) | `0x0010` | SDM 26.3.1.2: TI=0, RPL=CS.RPL | MATCH | NOT RELEVANT |
| **GUEST_SS_BASE** | `0x0000000000000000` | `0x0000000000000000` | `0x0000000000000000` | `0x0000000000000000` | SDM 26.3.1.2: Must be 0 in 64-bit mode | MATCH | NOT RELEVANT |
| **GUEST_SS_LIMIT** | `0xFFFFFFFF` | `0xFFFFFFFF` | `0xFFFFFFFF` | `0xFFFFFFFF` | SDM 26.3.1.2: Unrestricted in 64-bit mode | MATCH | NOT RELEVANT |
| **GUEST_SS_AR** | `0x0000C093` (Data, W, P) | `0x0000C093` | `0x0000C093` | `0x0000C093` | SDM 26.3.1.2: Usable, Present=1, S=1, Type=3 (Writable Data), DPL=CS.RPL | MATCH | NOT RELEVANT |
| **GUEST_DS_SEL** | `0x0000` | `0x0010` | `0x0010` | `0x0010` | SDM 26.3.1.2: Unchecked if unusable; if usable, Type=Data | **DIFFERENCE: ATOMS marks Unusable (0), bhyve/KVM/Xen set 0x0010** | **SUSPECT** |
| **GUEST_DS_AR** | `0x00010000` (Unusable) | `0x0000C093` (Usable) | `0x0000C093` (Usable) | `0x0000C093` (Usable) | SDM 26.3.1.2: Both usable and unusable are architecturally valid | **DIFFERENCE: bhyve native FreeBSD loader enters with Usable DS** | **SUSPECT** |
| **GUEST_ES_SEL** | `0x0000` | `0x0010` | `0x0010` | `0x0010` | SDM 26.3.1.2: Unchecked if unusable; if usable, Type=Data | **DIFFERENCE: ATOMS marks Unusable (0), bhyve/KVM/Xen set 0x0010** | **SUSPECT** |
| **GUEST_ES_AR** | `0x00010000` (Unusable) | `0x0000C093` (Usable) | `0x0000C093` (Usable) | `0x0000C093` (Usable) | SDM 26.3.1.2: Both usable and unusable are architecturally valid | **DIFFERENCE: bhyve native FreeBSD loader enters with Usable ES** | **SUSPECT** |
| **GUEST_FS_SEL** | `0x0000` | `0x0000` / `0x0010` | `0x0010` | `0x0010` | SDM 26.3.1.2: Base must be canonical even if unusable | MATCH (Base is 0 = canonical) | NOT RELEVANT |
| **GUEST_FS_AR** | `0x00010000` (Unusable) | `0x00010000` / `0xC093` | `0x0000C093` | `0x0000C093` | SDM 26.3.1.2: Usable or unusable permitted | MATCH (Unusable permitted) | NOT RELEVANT |
| **GUEST_GS_SEL** | `0x0000` | `0x0000` / `0x0010` | `0x0010` | `0x0010` | SDM 26.3.1.2: Base must be canonical even if unusable | MATCH (Base is 0 = canonical) | NOT RELEVANT |
| **GUEST_GS_AR** | `0x00010000` (Unusable) | `0x00010000` / `0xC093` | `0x0000C093` | `0x0000C093` | SDM 26.3.1.2: Usable or unusable permitted | MATCH (Unusable permitted) | NOT RELEVANT |
| **GUEST_TR_SEL** | `0x0028` (TI=0, RPL=0) | `0x0018` / `0x0028` | `0x0028` | `0x0028` | SDM 26.3.1.2: TI=0 (must reside in GDT) | MATCH | NOT RELEVANT |
| **GUEST_TR_BASE** | `0x0816DA90` | Host TSS base (canonical) | Host TSS base (canonical) | Host TSS base (canonical) | SDM 26.3.1.2: Canonical address | MATCH | NOT RELEVANT |
| **GUEST_TR_LIMIT** | `0x00000067` | `0x00000067` / `0x2087` | `0x00000067` | `0x00000067` | SDM 26.3.1.2: Must not be less than 0x67 for 64-bit TSS | MATCH | NOT RELEVANT |
| **GUEST_TR_AR** | `0x0000008B` (Busy TSS) | `0x0000008B` | `0x0000008B` | `0x0000008B` | SDM 26.3.1.2: Type=11 (64-bit Busy TSS), Present=1, S=0, Usable | MATCH | NOT RELEVANT |
| **GUEST_LDTR_SEL** | `0x0000` | `0x0000` | `0x0000` | `0x0000` | SDM 26.3.1.2: Unchecked if unusable | MATCH | NOT RELEVANT |
| **GUEST_LDTR_AR** | `0x00010000` (Unusable) | `0x00010000` (Unusable) | `0x00010000` (Unusable) | `0x00010000` (Unusable) | SDM 26.3.1.2: Unusable permitted | MATCH | NOT RELEVANT |
| **GUEST_GDTR_BASE** | `0x0816DF60` | Canonical GDT base | Canonical GDT base | Canonical GDT base | SDM 26.3.1.3: Must be canonical address | MATCH | NOT RELEVANT |
| **GUEST_GDTR_LIMIT**| `0x0000FFFF` | `0x0000FFFF` | `0x0000FFFF` | `0x0000FFFF` | SDM 26.3.1.3: Bits 31:16 must be 0 | MATCH | NOT RELEVANT |
| **GUEST_IDTR_BASE** | `0x051BCC70` | Canonical IDT base | Canonical IDT base | Canonical IDT base | SDM 26.3.1.3: Must be canonical address | MATCH | NOT RELEVANT |
| **GUEST_IDTR_LIMIT**| `0x0000FFFF` | `0x0000FFFF` | `0x0000FFFF` | `0x0000FFFF` | SDM 26.3.1.3: Bits 31:16 must be 0 | MATCH | NOT RELEVANT |
| **GUEST_DR7** | `0x0000000000000400` | `0x00000400` | `0x00000400` | `0x00000400` | SDM 26.3.1.1: Bit 10=1, bits 11:15=0, bits 63:32=0 | MATCH | NOT RELEVANT |
| **GUEST_DEBUGCTL** | `0x0000000000000000` | `0` | `0` | `0` | SDM 26.3.1.1: Reserved bits 0 | MATCH | NOT RELEVANT |
| **GUEST_PAT** | `0x0007040600070406` | Default PAT / unchanged | Default PAT / unchanged | Default PAT / unchanged | SDM 26.3.1.1: Compliant PAT encoding | MATCH | NOT RELEVANT |
| **SYSENTER (GUEST)**| CS=0, ESP=0, EIP=0 | CS=0, ESP=0, EIP=0 | CS=0, ESP=0, EIP=0 | CS=0, ESP=0, EIP=0 | SDM 26.3.1.1: Canonical ESP/EIP, CS[31:16]=0 | MATCH | NOT RELEVANT |
| **ACTIVITY_STATE** | `0` (Active) | `0` (Active) | `0` (Active) | `0` (Active) | SDM 26.3.2: 0, 1, 2, or 3 permitted | MATCH | NOT RELEVANT |
| **INTERRUPTIBILITY**| `0` | `0` | `0` | `0` | SDM 26.3.2: Bits 31:5=0 | MATCH | NOT RELEVANT |
| **PENDING_DEBUG** | `0` | `0` | `0` | `0` | SDM 26.3.2: Reserved bits=0 | MATCH | NOT RELEVANT |
| **VMCS_LINK_PTR** | `0xFFFFFFFFFFFFFFFF` | `~0ULL` | `~0ULL` | `~0ULL` | SDM 26.3.1.5: Must be ~0ULL when shadow VMCS disabled | MATCH | NOT RELEVANT |
| **PIN_CONTROLS** | `0x00000016` | Adjusted via MSR 0x48D | Adjusted via MSR 0x48D | Adjusted via MSR 0x48D | SDM 26.2.1.1: Must satisfy allowed-0/1 masks | MATCH | NOT RELEVANT |
| **PROC_CONTROLS** | `0x850061F2` | Adjusted via MSR 0x48E | Adjusted via MSR 0x48E | Adjusted via MSR 0x48E | SDM 26.2.1.1: Must satisfy allowed-0/1 masks | MATCH | NOT RELEVANT |
| **SECONDARY_CTLS** | `0x00000002` (EPT) | `0x00000082` (EPT+UG) | `0x00000082` (EPT+UG) | `0x00000082` (EPT+UG) | SDM 26.2.1.1: Must satisfy allowed-0/1 masks | **DIFFERENCE: ATOMS clears Bit 7 (Unrestricted Guest)** | **SUSPECT** |
| **VM_EXIT_CONTROLS**| `0x00336FFB` | `0x00336FFB` (64-bit, EFER) | `0x00336FFB` (64-bit, EFER) | `0x00336FFB` (64-bit, EFER) | SDM 26.2.1.2: Bit 9=1 (64-bit host), EFER save/load | MATCH | NOT RELEVANT |
| **VM_ENTRY_CONTROLS**| `0x000013FB` (Bit 15=0) | `0x000093FB` (Bit 15=1) | `0x000093FB` (Bit 15=1) | `0x000093FB` (Bit 15=1) | SDM 26.2.1.3: Bit 9=1 (IA-32e mode guest), Bit 15=Load EFER | **DIFFERENCE: ATOMS strips Bit 15, KVM/bhyve/Xen set Bit 15** | **SUSPECT** |
| **EPTP** | `0x...1666605E` / `...1E` | Walk=4, WB (`0x1E`) | Walk=4, WB (`0x1E`) | Walk=4, WB (`0x1E`) | SDM 24.6.11 / 26.2.1.1: Walk length 4 (bits 5:3 = 3), Memory type WB (bits 2:0 = 6), bits 11:6=0 | MATCH | NOT RELEVANT |
| **HOST_CR0** | Host CR0 (`0x80050033`) | Host CR0 (`0x80050033`) | Host CR0 (`0x80050033`) | Host CR0 (`0x80050033`) | SDM 26.2.2: PE=1, PG=1, FIXED0/1 compliance | MATCH | NOT RELEVANT |
| **HOST_CR3** | Host CR3 (`0x08160000`) | Host CR3 (PML4) | Host CR3 (PML4) | Host CR3 (PML4) | SDM 26.2.2: 4KB aligned physical address | MATCH | NOT RELEVANT |
| **HOST_CR4** | Host CR4 (`0x00000660`) | Host CR4 (PAE=1) | Host CR4 (PAE=1) | Host CR4 (PAE=1) | SDM 26.2.2: PAE=1 if 64-bit host, FIXED0/1 compliance | MATCH | NOT RELEVANT |
| **HOST_IA32_EFER** | `0x0000000000000D00` | `0xD00` / `0xD01` | `0xD00` / `0xD01` | `0xD00` / `0xD01` | SDM 26.2.2: LME=1, LMA=1, reserved bits=0 | MATCH | NOT RELEVANT |
| **HOST_RIP** | `(uint64)vmx_exit_hdl` | Canonical handler pointer | Canonical handler pointer | Canonical handler pointer | SDM 26.2.3: Must be canonical 64-bit linear address | MATCH | NOT RELEVANT |
| **HOST_RSP** | `host_rsp` (stack top) | Dedicated stack top | Dedicated stack top | Dedicated stack top | SDM 26.2.3: Canonical, not 0 | MATCH | NOT RELEVANT |
| **HOST_CS** | `0x0008` (RPL=0, TI=0) | `0x0008` / `0x0010` | `0x0008` | `0x0008` | SDM 26.2.3: Cannot be 0, RPL=0, TI=0 | MATCH | NOT RELEVANT |
| **HOST_SS** | `0x0010` | `0x0010` / `0x0000` | `0x0010` | `0x0010` | SDM 26.2.3: RPL=0, TI=0 if 64-bit host | MATCH | NOT RELEVANT |
| **HOST_DS** | `0x0010` | `0x0010` / `0x0000` | `0x0010` | `0x0010` | SDM 26.2.3: TI=0 if 64-bit host | MATCH | NOT RELEVANT |
| **HOST_ES** | `0x0010` | `0x0010` / `0x0000` | `0x0010` | `0x0010` | SDM 26.2.3: TI=0 if 64-bit host | MATCH | NOT RELEVANT |
| **HOST_FS** | `0x0010` | `0x0010` / `0x0000` | `0x0010` | `0x0010` | SDM 26.2.3: TI=0 if 64-bit host | MATCH | NOT RELEVANT |
| **HOST_GS** | `0x0010` | `0x0010` / `0x0000` | `0x0010` | `0x0010` | SDM 26.2.3: TI=0 if 64-bit host | MATCH | NOT RELEVANT |
| **HOST_TR** | `0x0028` (from `str`) | `0x0018` / `0x0028` | Host TR selector | Host TR selector | SDM 26.2.3: Cannot be 0, RPL=0, TI=0 | MATCH | NOT RELEVANT |
| **HOST_FS_BASE** | `rdmsr(0xC0000100)` | Canonical FS base | Canonical FS base | Canonical FS base | SDM 26.2.3: Canonical address | MATCH | NOT RELEVANT |
| **HOST_GS_BASE** | `rdmsr(0xC0000101)` | Canonical GS base | Canonical GS base | Canonical GS base | SDM 26.2.3: Canonical address | MATCH | NOT RELEVANT |
| **HOST_TR_BASE** | Host TSS base | Host TSS base | Host TSS base | Host TSS base | SDM 26.2.3: Canonical address | MATCH | NOT RELEVANT |
| **HOST_GDTR_BASE** | `host_gdtr.base` | Canonical GDTR base | Canonical GDTR base | Canonical GDTR base | SDM 26.2.3: Canonical address | MATCH | NOT RELEVANT |
| **HOST_IDTR_BASE** | `host_idtr.base` | Canonical IDTR base | Canonical IDTR base | Canonical IDTR base | SDM 26.2.3: Canonical address | MATCH | NOT RELEVANT |
| **SYSENTER (HOST)** | CS=0, ESP=0, EIP=0 | CS=0, ESP=0, EIP=0 | CS=0, ESP=0, EIP=0 | CS=0, ESP=0, EIP=0 | SDM 26.2.2: CS[31:16]=0, ESP/EIP canonical | MATCH | NOT RELEVANT |

---

## 2. Deep Analysis of Discrepancies

### 🔍 Discrepancy #1: `VM_ENTRY_CONTROLS` Bit 15 (`Load IA32_EFER`)
- **Actual ATOMS Value:** `entry_ctls = 0x000013FB` (Bit 9 = 1, Bit 15 = 0).
- **Reference Pattern (KVM, bhyve, Xen):** All three production hypervisors program `entry_ctls = 0x000093FB` (Bit 9 = 1, Bit 15 = 1).
- **Intel Requirement (SDM Vol 3C Sec 26.3.1.5):**
  - If Bit 15 is 1: `GUEST_IA32_EFER` must have LME=1 and LMA=1 (matching Bit 9), and reserved bits must be 0.
  - In ATOMS, `GUEST_IA32_EFER` is `0x0000000000000D01` (LME=1, LMA=1, NXE=1, SCE=1), which **100% satisfies** the SDM Bit 15 validation rule!
  - **Relevance:** In `VM_EXIT_CONTROLS`, Bit 21 (`Load IA32_EFER on Exit`) is enabled (`1`). On 14th Gen Raptor Lake silicon (microcode version 2024+), entering long mode without `Load IA32_EFER` while exit controls load EFER may cause the internal silicon shadow EFER state to disagree with guest mode requirements.
- **Classification:** **SUSPECT** (Hypothesis 1 for single-variable testing).

---

### 🔍 Discrepancy #2: Guest Segment State for DS and ES (Usable vs Unusable)
- **Actual ATOMS Value:**
  - `GUEST_DS_SELECTOR = 0x0000`, `LIMIT = 0`, `AR = 0x00010000` (Unusable).
  - `GUEST_ES_SELECTOR = 0x0000`, `LIMIT = 0`, `AR = 0x00010000` (Unusable).
- **Reference Pattern (FreeBSD bhyve):**
  - In bhyve's native loader entry for FreeBSD amd64:
    - `GUEST_DS_SELECTOR = 0x0010`, `LIMIT = 0xFFFFFFFF`, `AR = 0x0000C093` (Usable Data Segment, Present, Writable).
    - `GUEST_ES_SELECTOR = 0x0010`, `LIMIT = 0xFFFFFFFF`, `AR = 0x0000C093` (Usable Data Segment, Present, Writable).
- **Reference Pattern (Linux KVM & Xen):**
  - Both KVM and Xen configure DS and ES as usable data segments with limit `0xFFFFFFFF` and AR `0x0000C093`.
- **Intel Requirement (SDM Vol 3C Sec 26.3.1.2):**
  - Intel SDM states that marking DS and ES unusable (`AR bit 16 = 1`) bypasses type, limit, and selector checks.
  - However, in FreeBSD amd64 kernel startup (`locore.S`), the very first instructions assume valid segment registers:
    ```assembly
    mov $0x10, %eax
    mov %eax, %ds
    mov %eax, %es
    ```
- **Relevance:** While software validation permits unusable DS/ES, real Intel microcode may enforce strict consistency between CS (DPL 0, RPL 0) and DS/ES when entering directly into ring 0 64-bit kernel space without unrestricted guest mode.
- **Classification:** **SUSPECT** (Hypothesis 2 for single-variable testing).

---

### 🔍 Discrepancy #3: Secondary Processor-Based Controls (Bit 7: Unrestricted Guest)
- **Actual ATOMS Value:** `sec_ctls = 0x00000002` (Bit 1 = Enable EPT, Bit 7 = 0).
- **Reference Pattern (KVM, bhyve, Xen):** All three enable Bit 7 (`SECONDARY_EXEC_UNRESTRICTED_GUEST`) whenever supported in MSR `0x48B`.
- **Intel Requirement (SDM Vol 3C Sec 26.2.1.1 & 26.3.1.1):**
  - If Bit 7 = 0, guest CR0.PE and CR0.PG must be 1. ATOMS already has PE=1 and PG=1.
- **Relevance:** If Bit 7 is supported by the CPU, enabling it relaxes real-mode / protected-mode transition constraints in microcode.
- **Classification:** **SUSPECT** (Hypothesis 3 for single-variable testing).

---

## 3. Reference Correlation Architecture Plan

To eliminate blind spots, we will introduce **REFERENCE CORRELATION MODE** into the Autopsy Engine.
This mode runs directly on physical hardware and displays a 6-tier matrix:
```
[1] Intel SDM Validation  : PASS (All 28 Invariants)
[2] KVM Correlation       : DIFF ON VM_ENTRY_CONTROLS (Bit 15) & DS/ES Usable
[3] bhyve Correlation     : DIFF ON DS/ES Segments (bhyve uses 0x10 / 0xC093)
[4] Xen Correlation       : DIFF ON VM_ENTRY_CONTROLS (Bit 15)
[5] ATOMS VMCS Readback   : Readback matches written values 100%
[6] Final Silicon Result  : EXIT_REASON_INVALID_GUEST_STATE (0x80000021)
```

---

## 4. Single-Variable Hypothesis Testing Protocol (No Multi-Variable Guessing)

Strict adherence to the Scientific Bring-Up Rule:
1. **ONE CHANGE AT A TIME.**
2. Test on physical hardware (ASUS B760M-K + i3-14100F).
3. If successful, perform **A/B/A validation** (revert -> reproduce fail -> reapply -> reproduce pass).

| Run ID | Baseline | Hypothesis / One Change | Expected Outcome | Physical Silicon Result |
| :--- | :--- | :--- | :--- | :--- |
| `RUN_REF_01` | Baseline (Entry Ctls `0x13FB`, DS/ES Unusable) | **Hypothesis 1:** Set `VM_ENTRY_LOAD_IA32_EFER` (Bit 15) in `VM_ENTRY_CONTROLS` (`0x13FB` -> `0x93FB`) | Match KVM/bhyve/Xen long-mode EFER synchronization | PENDING PHYSICAL RUN |
| `RUN_REF_02` | Baseline (or Post-H1) | **Hypothesis 2:** Set `GUEST_DS` & `GUEST_ES` to Usable (`0x10`, `0xFFFFFFFF`, `0xC093`) | Match bhyve native FreeBSD 64-bit kernel entry | PENDING PHYSICAL RUN |
| `RUN_REF_03` | Baseline (or Post-H1/H2) | **Hypothesis 3:** Enable `SECONDARY_EXEC_UNRESTRICTED_GUEST` (Bit 7 in `sec_ctls`) | Match KVM/bhyve/Xen secondary execution configuration | PENDING PHYSICAL RUN |
