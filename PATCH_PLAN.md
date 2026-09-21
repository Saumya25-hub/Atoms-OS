# ATOMS OS — PATCH PLAN (Task 2: Dynamic VM-Exit Diagnosis & Dashboard Polish)

**Date:** 2026-09-21  
**Architect:** ATOMS Systems Architecture Team  
**Input:** `FORENSIC_REPORT.md` (Date 2026-09-21)  
**Target Milestone:** Intel VT-x Hardware VM-Entry Certification  
**Target Hardware:** ASUS PRIME B760M-K / Intel Core i3-14100F  

---

## 1. Scope of Modifications

Modify strictly one file:
- `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`

---

## 2. Detailed Component Plan

### `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`

1. **Panel 1 (`VM_EXIT_REASON` display, approx. lines 587-594):**
   - **What**: Conditionally format the exit reason string:
     - If `g_vmentry_screen_forensics.is_entry_failure` is true, display failure status in red (`c_fail`).
     - If `g_vmentry_screen_forensics.is_entry_failure` is false, display `(Clean VM-Exit / Hardware Success) [Bit 31: NO]` in green (`c_pass`).
   - **Why**: Accurately reflects hardware success when `VMLAUNCH` succeeds.

2. **Panel 1 (`EXIT QUAL` display, approx. lines 595-599):**
   - **What**: If `is_entry_failure` is true, display `| Guest Inst Executed: 0 (Aborted in Silicon Transition)`. If false, display `| Guest Execution: Active / Clean Transition PASS`.
   - **Why**: Correctly reports that the CPU entered guest execution mode.

3. **Panel 5 (`DIAGNOSIS` display, approx. lines 685-692):**
   - **What**:
     - If `is_entry_failure` is true, render red failure text `EXIT_REASON_INVALID_GUEST_STATE (33) -> CPU Rejected Guest VMCS`.
     - If `is_entry_failure` is false, render green success text `DIAGNOSIS : HARDWARE SUCCESS -> Intel VT-x vCPU Launched & Executed Cleanly`.
   - **Why**: Aligns Panel 5 with the 28/28 all-green pipeline stages.

---

## 3. Expected Result

Both the left pipeline stage list (Stages 1-28) and the right deep forensics panel will uniformly indicate **SUCCESS / PASS**, with zero hardcoded failure strings shown on physical silicon boots.
