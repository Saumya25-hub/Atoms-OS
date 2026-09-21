# ATOMS OS — VM-ENTRY FIX REPORT

**Target CPU:** Intel Core i3-14100F (Raptor Lake Refresh, LGA1700)  
**Host Motherboard:** ASUS PRIME B760M-K (Pure Native UEFI)  
**Milestone:** Stage 22 `VM_ENTRY` Hardware Bring-Up  
**Guest Kernel:** FreeBSD 14.1-RELEASE amd64  

---

## 1. Minimal Proven Fix Applied

In accordance with RULE 0 ("Do NOT guess, apply only the minimum proven fix"), the changes address the exact validation gaps identified in `VM_ENTRY_DEEP_FORENSIC_REPORT.md` and `VM_ENTRY_VALIDATION_MATRIX.md`:

### Fix 1: Guest Data Segment (DS, ES, FS, GS) 64-Bit Usability Alignment
* **File:** `kernel/core/hypervisor/src/hypervisor.c`
* **Lines:** 1324–1349
* **Before:**
  ```c
  vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, GDT_KERNEL_DATA); /* 0x10 */
  vmx_vmwrite(VMCS_GUEST_DS_LIMIT, 0xFFFFFFFF);
  vmx_vmwrite(VMCS_GUEST_DS_AR_BYTES, 0x0000C093); /* Usable 32-bit data */
  ```
* **After:**
  ```c
  vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, 0x0000);
  vmx_vmwrite(VMCS_GUEST_DS_LIMIT, 0x00000000);
  vmx_vmwrite(VMCS_GUEST_DS_AR_BYTES, 0x00010000); /* Unusable per SDM 26.3.1.2 */
  ```
* **Reason:** In IA-32e 64-bit mode (`VM_ENTRY_CONTROLS[9] == 1`), unused data segments must not maintain stale 32-bit segmentation caches that fail CPU consistency checks. Marking them Unusable (`0x00010000`) complies directly with standard FreeBSD long-mode initialization.
* **Hardware Rule Addressed:** Intel SDM Vol 3C Section 26.3.1.2 ("Checks on Guest Segment Registers").

### Fix 2: CR0.WP (Write-Protect Bit 16) Alignment
* **File:** `kernel/core/hypervisor/src/hypervisor.c`
* **Line:** 1404
* **Before:**
  ```c
  g_cr0 |= (1ULL << 0) | (1ULL << 31) | (1ULL << 4) | (1ULL << 5); /* PE, PG, ET, NE */
  ```
* **After:**
  ```c
  g_cr0 |= (1ULL << 0) | (1ULL << 31) | (1ULL << 4) | (1ULL << 5) | (1ULL << 16); /* PE, PG, ET, NE, WP */
  ```
* **Reason:** Aligns guest CR0 to `0x80010031`, fulfilling FreeBSD `locore.S` mandatory requirement that paging operates under write protection.
* **Hardware Rule Addressed:** Intel SDM Vol 3C Section 26.3.1.1 & FreeBSD 14.1-RELEASE amd64 `sys/amd64/amd64/locore.S`.

### Fix 3: Hardware CPU Flag Preservation & Failure Classification
* **File:** `kernel/core/hypervisor/src/vmx_entry.asm`
* **Lines:** 63–85
* **Before:**
  ```asm
  vmlaunch
  jmp .launch_failed
  ```
* **After:**
  ```asm
  vmlaunch
  pushfq
  pop rax
  mov [g_vmlaunch_rflags], rax
  jc .set_vmfail_invalid
  jz .set_vmfail_valid
  mov qword [g_vmlaunch_class], 3
  jmp .launch_failed
  ```
* **Reason:** Eliminates flag loss upon failure, providing software with exact CF, ZF, and RFLAGS values directly from hardware silicon.
* **Hardware Rule Addressed:** Intel SDM Vol 3C Section 30.1 ("VMX Instructions: VMLAUNCH / VMRESUME").
