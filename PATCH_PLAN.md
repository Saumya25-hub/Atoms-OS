# PATCH PLAN — Real Hardware Boot Presentation Timing Calibration

**Input**: `FORENSIC_REPORT.md`  
**Git Safety Checkpoint**: `10772dad35d8e636690451e0df7b8ab66e1e0e7d`

---

## 1. Scope of Modification

Modify ONLY the following files:
1. [`kernel/shell/rook/src/rook_core.c`](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c)
   - Add hardware PIT-calibrated `rook_get_tsc_per_ms()` function using Port 0x61 and Port 0x42/0x43.
   - Update `rook_splash_spin(uint32_t total_ms)` to pace frames using exact 16.666 ms cycles derived from `rook_get_tsc_per_ms()`.
   - Update `rook_login_spin()` to use `rook_get_tsc_per_ms()` instead of hardcoded 50M cycles.
2. [`kernel/shell/rook/debug/dashboard.c`](file:///d:/Signatures_OS/kernel/shell/rook/debug/dashboard.c)
   - Update `rook_dashboard_spin(uint32_t total_ms)` to pace frames using `rook_get_tsc_per_ms()`.
3. [`kernel/kernel.c`](file:///d:/Signatures_OS/kernel/kernel.c)
   - Adjust `rook_splash_spin()` duration to 3000ms (3.0s) and `rook_dashboard_spin()` to 1500ms (1.5s) to fulfill the 4.5–5.0 second boot presentation requirement.

---

## 2. Hard Constraints & Subsystems NOT Touched

❌ Do NOT touch bootloader architecture (`boot\uefi\bootx64.c`, `kernel_payload.asm`).  
❌ Do NOT touch Memory/PMM/VMM/Heap.  
❌ Do NOT touch Process/Scheduler/Syscalls.  
❌ Do NOT touch Networking/E1000/R8168/TLS/DNS.  
❌ Do NOT touch Browser/Blink/V8/Skia/BWE/Mojo/Sandbox.  
❌ Do NOT touch Login authentication or user accounts.  

---

## 3. Rollback Plan

If any regression occurs, execute `git reset --hard 10772dad35d8e636690451e0df7b8ab66e1e0e7d`.
