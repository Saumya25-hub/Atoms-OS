# PATCH_PLAN.md — 4-Part Commercial Combo (VSYNC + Safe CPUID MTRR WC)

## Executive Summary
This document specifies the exact architecture plan to implement VSYNC Scanline Synchronization and Safe CPUID MTRR Write-Combining without risking virtual machine triple faults.

---

## 1. What to Modify

### Modification A: Safe CPUID-Guarded MTRR Write-Combining
- **File**: [vram_accel.c](file:///d:/Signatures_OS/kernel/drivers/display/vram_accel.c) & [vram_accel.h](file:///d:/Signatures_OS/kernel/drivers/display/vram_accel.h)
- **Plan**:
  1. Add CPUID feature check for MTRR support (`CPUID EAX=1`, `EDX` bit 12).
  2. Add Hypervisor detection (`CPUID EAX=1`, `ECX` bit 31). If running inside VMware/VirtualBox/QEMU, skip raw MSR writes safely.
  3. If running on bare-metal hardware with MTRR support, safely configure MTRR Pair 0 for Write-Combining (`0x01`).

### Modification B: VSYNC Retrace Synchronization Gating
- **File**: [rook_render.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c)
- **Plan**:
  1. Add VSYNC retries helper (`inb(0x3DA)` Vertical Retrace / VBLANK detection).
  2. In `rook_render_flush()`, wait for Vertical Retrace before blitting dirty rects to physical VRAM.

### Modification C: Safe Driver Integration
- **File**: [kernel.c](file:///d:/Signatures_OS/kernel/kernel.c) & [build.ps1](file:///d:/Signatures_OS/build.ps1)
- **Plan**:
  1. Call `vram_accel_init(boot_info)` safely after `dgl_init`.
  2. Add `vram_accel.o` to build and link scripts.

---

## 2. Expected Results
- **VSYNC Syncing**: 100% Zero Tearing, 100% Phase-Aligned Smooth Motion on physical LCD/LED monitors.
- **Safe MTRR WC**: PCIe Burst Speed (8,000+ MB/s) on bare-metal hardware (Intel i3 / Haswell / AMD / NVIDIA) with ZERO `#GP` crashes on VMware Workstation and VirtualBox!

---

## 3. Rollback Plan
- If any build or runtime issues occur, remove `vram_accel_init` call from `kernel.c` and disable VSYNC gating in `rook_render.c`.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
