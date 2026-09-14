# ATOMS OS — PATCH PLAN (TASK 2)
## Mission: BOFS Phase 13 — Real-Hardware Native BOFS Certification (Full Physical Storage + Real File + Persistence + Recovery + Stress)

**Input:** `PHASE13_FORENSIC_REPORT.md`  
**Protocol:** ATOMS OS Engineering Protocol V1 — RULE 0 (TASK 2 ARCHITECT TEAM)  
**Date:** 2026-09-05  
**Baseline Checkpoint:** `PHASE13_PRECHECKPOINT = 1b472fdcb9a39a6bf129e1695d88a95c510cf959` (`1b472fd`)  
**Status:** ARCHITECTURAL PLAN READY — NO SOURCE CODE MODIFIED  

---

## 1. Scope of Proposed Changes

In strict compliance with the **Hard Rules** and **Rule 0**, ONLY the following files are approved for creation or modification:

| Action | Component | File Path | Rationale |
|---|---|---|---|
| **NEW** | **Phase 13 Engine Header** | `kernel/debug/bofs/bofs_phase13_certified_runner.h` | Defines Phase 13 safety gate structures, test identifiers (T01–T53), evidential classifications, write/read ledgers, and entrypoints. |
| **NEW** | **Phase 13 Physical Runner & UI** | `kernel/debug/bofs/bofs_phase13_certified_runner.c` | Implements physical device discovery, foreign storage write-locking, human safety confirmation gate, physical BOFS format engine, automated lifecycle runner, persistence verifier, first-failure detection, and 4-panel ABDE UI with live heartbeat spinner. |
| **MODIFY** | **Kernel Boot Router** | `kernel/kernel.c` | Adds `#define ATOMS_DEBUG_MODE_BOFS_PHASE13 17`, sets active debug mode to 17, and routes boot storage execution to `bofs_phase13_certified_runner_run()`. |
| **MODIFY** | **Build Configuration** | `build.ps1` | Adds clang rule compiling `bofs_phase13_certified_runner.c` to `bofs_phase13_certified_runner.o` and registers object in `$lldRsp` linker list. |
| **NEW** | **Host Automated Test Engine** | `tools/bofs/test_phase13_physical.py` | Standalone Python host test suite executing 53 tests including 1,000-cycle stress test and zero resource drift verification. |
| **NEW** | **Pure UEFI QEMU Dual-Disk Runner** | `tools/bofs/test_phase13_qemu.py` | Launches QEMU in pure UEFI mode with a secondary dedicated physical disk image (`build/bofs_dedicated_drive.img`), verifies COM1 telemetry, and captures screenshot. |
| **NEW** | **Documentation** | `docs/BOFS/PHASE13_REAL_HARDWARE_CERTIFICATION.md` | Formal architecture and certification documentation. |
| **NEW** | **Master Report** | `docs/BOFS/PHASE13_MASTER_CERTIFICATION_REPORT.md` | Authoritative forensic report complying with Section 60. |

---

## 2. Detailed Component Architecture

### A. Safety Gate & Foreign Storage Write-Lock Guard
```
                         DISCOVER STORAGE
                                ↓
        ┌───────────────────────┴───────────────────────┐
        ↓                                               ↓
FOREIGN PARTITIONS (1..5)                      DEDICATED TARGET
[ESP, MSR, NTFS, Recovery]                     [Dedicated Partition/Drive]
        ↓                                               ↓
HARDWARE WRITE-LOCK                            SAFETY CHECKS:
read_only = true                               Not ESP / Not MSR / Not NTFS /
FOREIGN WRITES = 0 BYTES                       Not Overlapping / Size >= 32MB
                                                        ↓
                                               HUMAN SAFETY CONFIRMATION
                                                        ↓
                                               PHYSICAL BOFS FORMAT
```

### B. Automated End-to-End Test Engine (T01–T53)
The in-kernel runner `bofs_phase13_certified_runner_run()` will execute:
1. `p13_probe_hardware()`: Queries CPUID, RAM, PCI controllers, NVMe, SATA, and block devices.
2. `p13_evaluate_safety_gate()`:
   - Scans partition tables.
   - Enforces `read_only = true` on `nvme0n1p1`, `p2`, `p3`, `p4`, `p5`.
   - Confirms `FOREIGN STORAGE WRITES = 0 BYTES`.
   - Identifies candidate dedicated test block device. If none found, displays `PHYSICAL BOFS STORAGE: NOT AVAILABLE (SAFETY GATE LOCKED)` and halts before format.
3. `p13_format_bofs()`: Writes Superblock, Backup Superblock, Journal Header, Bitmaps, Root Inode, and Root Directory Leaf Node. Verifies readback and CRC32.
4. `p13_mount_vfs()`: Mounts physical BOFS at `/phase13/` (or `/`) via `vfs_mount_fs()`.
5. `p13_run_lifecycle_tests()`: Executes real file operations (Create, Write, Read, Reopen, Rename, Stat, Mkdir, Nested, Unicode, Permissions, Zero-Mutation Denial, Delete, Rmdir, Remount).
6. `p13_stress_and_drift()`: Runs 1,000 filesystem cycles and measures resource deltas (`FD=0, Inode=0, Block=0, Process=0, Frame=0, Journal=0`).
7. `p13_wal_and_crash()`: Verifies WAL atomic commit and crash replay boundaries.
8. `p13_reboot_persistence()`: Verifies persistent file content matching pre-reboot SHA-256 / CRC32 hash.
9. `p13_first_failure_detection()`: Pinpoints earliest broken layer if any test fails.
10. `p13_draw_dashboard()`: Renders 4-panel ABDE UI with live heartbeat spinner and streams live screenshot over UDP 9998.

---

## 3. Expected Results & Verification Gates

1. **Compiler & Linker:** Zero errors, zero warnings.
2. **Host Test Matrix:** 53 / 53 tests PASS (100%).
3. **1,000-Cycle Stress:** Zero FD, Inode, Block, Process, or Journal drift.
4. **QEMU Pure UEFI Pre-Flight:** Boot with dedicated disk image, verify serial telemetry, and capture framebuffer screenshot.
5. **Real Bare-Metal ASUS B750M-K:** Safety gate enforces read-only protection on Windows 11 NTFS, detects physical controllers, displays truth dashboard, and streams live telemetry + screenshot.
6. **Regressions:** Zero regressions on Phases 3–12.

---

## 4. Rollback Plan

If any critical failure or regression occurs:
1. Revert working tree to `PHASE13_PRECHECKPOINT`:
   ```powershell
   git reset --hard 1b472fdcb9a39a6bf129e1695d88a95c510cf959
   ```
2. Recompile and restore Phase 12 certified state via `build.ps1`.
