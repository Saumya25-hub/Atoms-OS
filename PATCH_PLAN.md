# 📐 ARCHITECTURE PATCH PLAN: SAFE HARDWARE SHUTDOWN & PCH 5VSB POWER RAIL PROTECTION
**Subsystem:** ATOMS OS Power Management Subsystem (`system_power.c`, Intel PCH LPC ICH, ACPI PMBASE)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-15  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Eliminate all blind, unverified I/O port writes in `system_power.c` that cause Intel Haswell H81 PCH power latch lockout.
- Implement standard Linux `lpc_ich` PCI 0:31:0 dynamic PMBASE discovery.
- Ensure safe hypervisor shutdown in QEMU/VirtualBox while protecting physical bare-metal hardware.
- Provide clean, safe fallback CPU halt with zero PCH register corruption.

---

## 2. Target Files for Modification
1. `kernel/core/power/system_power.c`:
   - Replace blind port writes with dynamic Intel LPC Bridge (PCI 0:31:0) PMBASE query (`pci_read_config_32(0, 31, 0, 0x40)`).
   - Only write to verified `PMBASE + 0x04` when Intel LPC is confirmed active.
   - For hypervisors (QEMU/Bochs), write to standard QEMU ACPI port `0x604` only when running under QEMU or if verified.
   - Remove destructive blind writes to `0xB004`, `0x600`, `0x1804`, `0xB2`.

---

## 3. Expected Engineering Results
- **Physical H81 Hardware:** Clean, safe shutdown with zero 5VSB power rail lockouts. Front-panel power button functions 100% normally without requiring AC power cord disconnects.
- **Hypervisors (QEMU/VMware/VirtualBox):** Clean instant power-off.

---

## 4. Rollback Plan
Revert changes to Git commit `cc0e201`.
