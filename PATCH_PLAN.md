# PATCH_PLAN.md — Hardware TSC Real-Time 60.00 FPS Frame Pacing Plan

## Executive Summary
This document specifies the exact plan to implement x86_64 Hardware TSC (`rdtsc`) cycle delta frame pacing in `rook_splash_spin` to guarantee true 60.00 FPS liquid "butter spin" motion on any CPU core.

---

## 1. What to Modify

### Modification A: Hardware TSC Cycle Delta Pacing in rook_core.c
- **File**: [rook_core.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c)
- **Plan**:
  1. Measure CPU TSC frequency or calibrate 16.666ms cycle count at start of `rook_splash_spin`.
  2. For each frame, record starting TSC timestamp `start_tsc = rdtsc()`.
  3. Execute `rook_update(16)` and `rook_render()`.
  4. Wait in a `pause` loop until `rdtsc() - start_tsc >= target_cycles_per_frame`.
  5. Guarantees 100% uniform 16.666ms frame presentation on 4.7 GHz i3-14100F, Haswell, and VMware!

### Modification B: Anti-Aliased Soft Edge Rendering in ame_spinner.c
- **File**: [ame_spinner.c](file:///d:/Signatures_OS/kernel/ame/src/ame_spinner.c)
- **Plan**:
  1. Add soft radial alpha falloff to circle rendering for silky anti-aliased arc edges.

---

## 2. Expected Result
- **Frame Presentation**: 100% Hard-Real-Time 60.00 FPS ("Butter Spin" Liquid Smoothness).
- **CPU Portability**: Perfect uniform animation speed on any bare-metal CPU or hypervisor VM.

---

## 3. Rollback Plan
- Revert pacing to static loop if TSC reading encounters any unexpected virtualizer constraint.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
