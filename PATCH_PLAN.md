# 📐 ARCHITECTURE PATCH PLAN: UNIVERSAL ACPI RSDP/FADT/DSDT SHUTDOWN ENGINE
**Subsystem:** ATOMS OS Universal Power Management Subsystem (`system_power.c`, ACPI RSDP/XSDT/FADT/DSDT S5 Parser)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Implement a complete, self-contained ACPI S5 Parser in `system_power.c` matching the Linux kernel (`drivers/acpi/acpica/hwxfsleep.c`) standard.
- Add memory scanner for `"RSD PTR "` across BIOS / EBDA / ACPI Reclaimable ranges.
- Locate FADT (`"FACP"`) and DSDT.
- Parse `\_S5_` AML package to extract exact `SLP_TYPa` and `SLP_TYPb` sleep types.
- Switch chipset into ACPI mode via `SMI_CMD` if necessary.
- Trigger native physical power-off via verified `PM1a_CNT_BLK` and `PM1b_CNT_BLK`.
- Add native VMware hypervisor backdoor power-off (`Port 0x5658`).

---

## 2. Target Files for Modification
1. `kernel/core/power/system_power.c`:
   - Implement `find_rsdp()`, `find_fadt()`, `parse_s5_from_dsdt()`, `acpi_power_off()`, and `vmware_power_off()`.
   - Update `system_shutdown()` to execute VMware power-off, ACPI S5, and fallback mechanisms.

---

## 3. Expected Engineering Results
- **Physical LGA1150 H81 Motherboard:** Cuts ATX power cleanly via dynamic ACPI FADT/DSDT S5 sequence.
- **VMware Workstation:** Cuts power instantly via backdoor + ACPI S5.
- **QEMU / VirtualBox:** Cuts power instantly.

---

## 4. Rollback Plan
Revert changes to Git commit `e941a85`.
