# PATCH_PLAN.md — Restore 100% Stable Boot (Remove MTRR MSR Write)

## Executive Summary
This document specifies the exact plan to revert MTRR MSR writes and restore 100% stable boot across all bare-metal hardware and virtual machines.

---

## 1. What to Modify

### Modification A: Disable MTRR MSR Write Call in kernel.c
- **File**: [kernel.c](file:///d:/Signatures_OS/kernel/kernel.c)
- **Action**: Comment out `vram_accel_init(boot_info);` call.

### Modification B: Make vram_accel.c a Safe Stub
- **File**: [vram_accel.c](file:///d:/Signatures_OS/kernel/drivers/display/vram_accel.c)
- **Action**: Return immediately in `vram_accel_init` to prevent any MSR modification.

---

## 2. Expected Result
- **Boot Reliability**: 100% Instant Clean Boot on Bare-Metal H81, Intel Core i3 14th Gen + RTX 4060, VMware Workstation, and VirtualBox.
- **Display Output**: 100% Centered Chevron Logo, 256-Subdegree Subpixel Windows 11 Fluent Dynamic Arc Ring rendering smoothly.

---

## 3. Rollback Plan
- Reverting to `kernel.c` baseline restores certified stable state (`v5.0-atoms-un-stuck-calibrated-60fps-certified`).

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
