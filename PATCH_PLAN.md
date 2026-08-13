# PATCH_PLAN.md — ATOMS OS Stage 1 Isolation Architecture Plan

## Executive Summary
This document specifies the exact architecture modifications planned to achieve commercial-grade liquid-smooth boot splash animation by enforcing Stage 1 CPU isolation.

---

## 1. What to Modify

### Modification A: Defer Background System Tasks Until Login Stage
- **File**: [kernel.c](file:///d:/Signatures_OS/kernel/kernel.c)
- **Action**: Move `scheduler_create_kernel_task` calls for `ABDE_Telemetry`, `Heartbeat`, `Diagnostics`, and `Debug_Shell` to execute IMMEDIATELY AFTER `rook_splash_spin(6000)` finishes and transitions to `DGL_STATE_LOGIN`.

### Modification B: Maintain Dedicated Stage 1 Rendering Loop
- **File**: [rook_core.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c)
- **Action**: Keep `rook_splash_spin` running as the sole foreground execution context during `DGL_STATE_BOOT` with zero preemption interference.

---

## 2. Technical Rationale (Why)
- Commercial OS kernels (Windows 11 `winload.efi` / Linux Plymouth) isolate Stage 1 boot rendering from background services.
- By deferring background telemetry and debug shell task registration until `DGL_STATE_LOGIN`, 100% of CPU cycles during the 6.0-second boot splash are dedicated exclusively to AME Spinner rendering.
- Zero context switching overhead = 100% steady frame presentation.

---

## 3. Expected Result
- **Boot Splash (`ROOK_PAGE_BOOT_SPLASH`)**: 100% Liquid Smooth Windows 11 Fluent Dynamic Arc Ring motion at native 60 FPS on real bare-metal hardware and VMware Workstation.
- **Login Transition (`ROOK_PAGE_LOGIN`)**: All background system threads (`ABDE_Telemetry`, `Heartbeat`, `Diagnostics`, `Debug_Shell`) automatically initialize and start running smoothly when the login screen appears.

---

## 4. Risk & Mitigation
- **Risk**: A background service needed by early boot might be delayed.
- **Mitigation**: Inspection confirms `ABDE_Telemetry`, `Heartbeat`, `Diagnostics`, and `Debug_Shell` are non-blocking background monitoring threads that are only required during interactive shell / login runtime.

---

## 5. Rollback Plan
- If any regression occurs, move task creation back prior to `rook_splash_spin` in `kernel.c`.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
