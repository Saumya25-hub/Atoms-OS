# 🔬 FORENSIC INVESTIGATION REPORT: UNIVERSAL ACPI RSDP/FADT/DSDT SHUTDOWN ENGINE
**Subsystem:** ATOMS OS Universal Power Management Subsystem (`system_power.c`, ACPI RSDP/XSDT/FADT/DSDT S5 Parser)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary & Root Cause
Testing on physical H81 motherboard and VMware Workstation revealed that the previous shutdown attempt did not cut ATX power.
* **Root Cause 1 (UEFI Runtime Address Space):** Calling UEFI `RuntimeServices->ResetSystem()` POST-`ExitBootServices()` without active UEFI runtime virtual page table mappings causes an unmapped fault or silent abort in firmware.
* **Root Cause 2 (ACPI S5 AML Object Resolution):** Haswell H81 motherboard and VMware Workstation require parsing the ACPI `FADT` (Signature `'FACP'`) and `DSDT` AML bytecode for `\_S5_` to resolve the board-specific `PM1a_CNT_BLK` I/O port and `SLP_TYPa` / `SLP_TYPb` sleep state values.

---

## 2. Universal Real OS Linux ACPI Standard (`drivers/acpi/acpica/hwxfsleep.c`)
1. **RSDP Search:** Scan memory regions (`0xE0000`–`0xFFFFF` and EBDA `0x9FC00`–`0x9FFFF`) for `"RSD PTR "` signature.
2. **XSDT / RSDT Navigation:** Read 64-bit/32-bit physical table pointers and locate FADT (`"FACP"`).
3. **Hardware ACPI Enable:** If `SCI_EN` bit 0 in `PM1a_CNT_BLK` is 0, send `FADT->ACPI_ENABLE` command to `FADT->SMI_CMD` port.
4. **DSDT AML S5 Object Parsing:** Locate `\_S5_` package in DSDT bytecode to extract exact `SLP_TYPa` and `SLP_TYPb`.
5. **Universal Sleep Execution:** Write `(SLP_TYPa << 10) | (1 << 13)` to `PM1a_CNT_BLK` and `(SLP_TYPb << 10) | (1 << 13)` to `PM1b_CNT_BLK`.
6. **VMware Backdoor Power-Off:** Send VMware I/O backdoor command (`Port 0x5658`, Magic `0x564D5868`, Cmd `10`) for instant VMware hypervisor power cut.

---

## 3. Files Involved
* `kernel/core/power/system_power.c`: Implement universal ACPI RSDP/FADT/DSDT S5 parser and VMware backdoor power-off.
