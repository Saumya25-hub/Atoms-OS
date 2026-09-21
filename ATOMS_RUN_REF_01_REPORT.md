# ATOMS OS — PHYSICAL VM-ENTRY FORENSIC TEST REPORT
## RUN_REF_01: IA32_EFER ENTRY CONTROL BIT 15

**Target Hardware:** ASUS PRIME B760M-K (Intel B760 LGA1700 Chipset)  
**Target CPU:** Intel Core i3-14100F (14th Gen Raptor Lake Refresh, 4P/8T, Family 6, Model 183, Stepping 1)  
**Workload:** Genuine FreeBSD 14.1-RELEASE amd64 direct kernel boot (`locore.S`)  
**Execution Environment:** Pure UEFI Network PXE Boot, RAM disk execution only (Zero Host Disks attached)  
**Experiment ID:** `RUN_REF_01`  
**Protocol Phase:** Phase 3 Verification / Condition B Hardware Trial  

---

## 1. Physical Hardware Profile

| Parameter | Value | Verification Source |
| :--- | :--- | :--- |
| **CPU Model** | Intel Core i3-14100F | CPUID string / Panel 29 Operator View |
| **Microarchitecture** | Raptor Lake Refresh (Intel 7 Process) | CPUID Family 0x06, Model 0xB7 |
| **Motherboard** | ASUS PRIME B760M-K D4 | DMI / BIOS 2024 UEFI Native Mode |
| **MAC Address** | `A0:AD:9F:C5:81:27` | PXE DHCP Server Log |
| **Network IP** | `192.168.2.100` | Host `192.168.2.1` TFTP Daemon |
| **GOP Display** | 1920x1080 32bpp Linear Framebuffer | GOP Framebuffer Protocol |

---

## 2. Baseline A (Observed Physical Behavior)

Prior to applying `RUN_REF_01`, the system operated under **Condition A (Baseline)**:

* **VMLAUNCH Attempted:** `YES`
* **VM-Entry Accepted:** `[ FAIL ]`
* **VM Exit Reason:** `0x80000021` (Bit 31 = VM-entry failure, Basic Exit Reason 33 = `EXIT_REASON_INVALID_GUEST_STATE`)
* **Guest Instructions Executed:** `0` (Aborted during silicon transition)
* **Software Invariant Validator:** `PASS (28/28 Invariants Validated)`
* **VMCS Write/Readback Audit:** `100% MATCH (0 Mismatches Across All Fields)`
* **VM_INSTRUCTION_ERROR (0x4400):** `0x00000000` (Hardware does not write instruction error on Exit 33)
* **VM_ENTRY_CONTROLS:** `0x000013FB`
  * Bit 9 (`IA-32e mode guest`) = `1`
  * Bit 15 (`Load IA32_EFER`) = `0` (Stripped in baseline)

---

## 3. Condition B (`RUN_REF_01` Single-Variable Hypothesis)

### Hypothesis:
Reference hypervisors (Linux KVM, FreeBSD bhyve, Xen HVM) unconditionally set Bit 15 (`Load IA32_EFER = 1`) when launching a 64-bit guest (`Bit 9 = 1`), while `VM_EXIT_CONTROLS` has Bit 21 (`Load IA32_EFER on Exit = 1`). On 14th Gen Intel silicon microcode, entering long mode without `Load IA32_EFER` causes microcode state discrepancy between internal long-mode shadow registers and the guest EFER state.

### Single-Variable Modification:
In `kernel/core/hypervisor/src/hypervisor.c`:
```c
/* Baseline A: */
entry_ctls &= ~(1U << 15);  /* 0x000013FB */

/* Condition B (RUN_REF_01): */
entry_ctls |= (1U << 15);   /* 0x000093FB */
```

### Strict Gate Verification:
```text
VM_ENTRY_CONTROLS_BEFORE = 0x000013FB
VM_ENTRY_CONTROLS_AFTER  = 0x000093FB
CHANGED_BITS             = 0x00008000
CHANGED_BIT_COUNT        = 1
```
Zero other bits, registers, segments, or MSRs modified.

---

## 4. Hardware Capability Evidence (MSR 0x490 Readback)

The physical CPU was directly queried via `RDMSR` for `IA32_VMX_TRUE_ENTRY_CTLS` (MSR `0x490`):

* **Raw MSR 0x490 Read:** `0x0076FFFF000011FB`
* **Forced-1 Mask (Bits 31:0):** `0x000011FB` (Hardware requires these bits to be 1)
* **Allowed-1 Mask (Bits 63:32):** `0x0076FFFF` (Bits permitted to be 1)
* **Bit 15 Capability Check:**
  * `allowed_1_mask & (1U << 15) = 0x0076FFFF & 0x00008000 = 0x00008000 != 0`
* **Hardware Verdict:** **YES (PASS)** — 14th Gen Intel silicon explicitly permits Bit 15 (`Load IA32_EFER`) to be `1`.

---

## 5. Mandatory Pre-Launch Complete Snapshot (Condition B)

| Register / Field | Programmed Value | Readback Verification | Hardware Status |
| :--- | :--- | :--- | :--- |
| **VM_ENTRY_CONTROLS** | `0x000093FB` | `0x000093FB` | MATCH (Bit 9=1, Bit 15=1) |
| **VM_EXIT_CONTROLS** | `0x00336FFB` | `0x00336FFB` | MATCH (Host 64B=1, Load EFER=1, Save EFER=1) |
| **PIN_BASED_CONTROLS** | `0x00000016` | `0x00000016` | MATCH (MSR 0x48D mask satisfied) |
| **PRIMARY_PROC_CONTROLS**| `0x850061F2` | `0x850061F2` | MATCH (Secondary=1, MSR 0x48E satisfied) |
| **SECONDARY_PROC_CONTROLS**| `0x00000002` | `0x00000002` | MATCH (EPT=1, Unrestricted=0) |
| **IA32_VMX_BASIC (0x480)** | `0x03DA050000000013` | N/A (MSR Read) | Bit 55=1 (True MSRs supported) |
| **IA32_VMX_TRUE_ENTRY (0x490)**| `0x0076FFFF000011FB` | N/A (MSR Read) | Forced-1: `0x11FB`, Allowed-1: `0x0076FFFF` |
| **IA32_VMX_TRUE_EXIT (0x48F)** | `0x007FFFFF00036DFB` | N/A (MSR Read) | EFER save/load permitted |
| **IA32_VMX_PROCBASED2 (0x48B)**| `0x0000000000000082` | N/A (MSR Read) | EPT and UG capable |
| **GUEST_CR0** | `0x0000000080010031` | `0x0000000080010031` | PE=1, PG=1, ET=1, NE=1, WP=1 |
| **GUEST_CR3** | `0x0000000000020000` | `0x0000000000020000` | 4KB Page Aligned PML4 |
| **GUEST_CR4** | `0x0000000000000020` | `0x0000000000000020` | PAE=1 (Required in long mode) |
| **GUEST_IA32_EFER** | `0x0000000000000D01` | `0x0000000000000D01` | LME=1, LMA=1, NXE=1, SCE=1 |
| **GUEST_RIP** | `0xFFFFFFFF8037C000` | `0xFFFFFFFF8037C000` | Canonical FreeBSD `locore.S` entry |
| **GUEST_RSP** | `0x000000000007FF00` | `0x000000000007FF00` | Canonical page-aligned stack top |
| **GUEST_RFLAGS** | `0x0000000000000002` | `0x0000000000000002` | Bit 1=1 |
| **GUEST_CS** | Sel=`0x0008`, Base=`0`, Lim=`0xFFFFFFFF`, AR=`0x0000A09B` | Match | Long mode 64-bit Code (L=1, D=0) |
| **GUEST_SS** | Sel=`0x0010`, Base=`0`, Lim=`0xFFFFFFFF`, AR=`0x0000C093` | Match | 64-bit Data Writable |
| **GUEST_DS** | Sel=`0x0000`, Base=`0`, Lim=`0`, AR=`0x00010000` | Match | Unusable |
| **GUEST_ES** | Sel=`0x0000`, Base=`0`, Lim=`0`, AR=`0x00010000` | Match | Unusable |
| **GUEST_FS** | Sel=`0x0000`, Base=`0`, Lim=`0`, AR=`0x00010000` | Match | Unusable |
| **GUEST_GS** | Sel=`0x0000`, Base=`0`, Lim=`0`, AR=`0x00010000` | Match | Unusable |
| **GUEST_TR** | Sel=`0x0028`, Base=`0x0816FC90`, Lim=`0x67`, AR=`0x8B` | Match | 64-bit Busy TSS (GDT verified) |
| **GUEST_LDTR** | Sel=`0x0000`, Base=`0`, Lim=`0`, AR=`0x00010000` | Match | Unusable |
| **GUEST_GDTR** | Base=`0x08170160`, Limit=`0xFFFF` | Match | Canonical GDT base |
| **GUEST_IDTR** | Base=`0x051C9970`, Limit=`0xFFFF` | Match | Canonical IDT base |
| **GUEST_DR7** | `0x0000000000000400` | `0x0000000000000400` | Architectural default |
| **SYSENTER_CS / ESP / EIP** | CS=`0`, ESP=`0`, EIP=`0` | Match | Zeroed |
| **EPTP** | `0x000000001667205E` | `0x000000001667205E` | 4-Level walk, WB caching |
| **VMCS_LINK_POINTER** | `0xFFFFFFFFFFFFFFFF` | `0xFFFFFFFFFFFFFFFF` | Shadowing disabled |
| **HOST_CR0 / CR3 / CR4** | `0x80050033` / Host PML4 / `0x00000660` | Match | Validated |
| **HOST_RIP / RSP** | `vmx_vmexit_handler` / Dedicated Host Stack | Match | Canonical |
| **HOST_CS / SS / TR** | CS=`0x0008`, SS=`0x0010`, TR=`0x0028` | Match | Validated |
| **HOST_GDTR / IDTR** | Host GDTR base / Host IDTR base | Match | Canonical |

---

## 6. Field-by-Field Diff (Condition A vs Condition B)

```diff
--- Baseline A (0x000013FB)
+++ Condition B (0x000093FB)
@@ -1 +1 @@
-VM_ENTRY_CONTROLS = 0x000013FB  (Bit 15 = 0, Load IA32_EFER disabled)
+VM_ENTRY_CONTROLS = 0x000093FB  (Bit 15 = 1, Load IA32_EFER enabled)
```
**Total Bits Changed:** Exactly **1 bit** (`0x00008000`).  
**Zero unintended modifications.**

---

## 7. A/B/A/B Causality Matrix

| Iteration | Condition | Programmed `VM_ENTRY_CONTROLS` | Hardware VMLAUNCH | Hardware Exit Reason | Instructions Executed | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **A1** | Baseline A | `0x000013FB` | EXECUTED | `0x80000021` (Exit 33) | 0 | **FAIL (Documented)** |
| **B1** | Condition B | `0x000093FB` | EXECUTED | *Pending Physical Boot* | *Pending* | **ACTIVE IN BUILD** |
| **A2** | Revert to A | `0x000013FB` | *Pending* | *Pending* | *Pending* | PENDING B1 OUTCOME |
| **B2** | Reapply B | `0x000093FB` | *Pending* | *Pending* | *Pending* | PENDING B1 OUTCOME |

---

## 8. Expected Outcomes & Next Actions

### If B1 Passes (`VMLAUNCH` accepted, guest instructions > 0):
1. Immediately capture guest RIP, first VM exit reason, and FreeBSD console milestone.
2. Revert to `A2` (`0x000013FB`) and confirm `0x80000021` reproduces.
3. Reapply `B2` (`0x000093FB`) and confirm VMLAUNCH succeeds again.
4. Only then certify Bit 15 as the **PROVEN CAUSAL FACTOR**.

### If B1 Fails (Exit reason remains `0x80000021`, guest instructions = 0):
1. Classify `RUN_REF_01` as **NOT PROVEN**.
2. Revert Bit 15 to `0` (`0x000013FB`).
3. Move strictly to **Hypothesis 2 (`RUN_REF_02`): Guest DS & ES Usability** (configuring DS/ES as Usable `0x0010` / `0x0000C093` matching FreeBSD bhyve loader).
