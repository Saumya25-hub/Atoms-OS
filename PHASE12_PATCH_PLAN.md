# ATOMS OS — ARCHITECTURE PATCH PLAN (TASK 2)
## BOFS Phase 12: Final Forensic Debug Dashboard & Cross-Layer Observability

**Document ID:** ATOMS-BOFS-PHASE12-PLAN-001  
**Input:** `PHASE12_FORENSIC_REPORT.md`  
**Protocol:** ATOMS OS Engineering Protocol V1 — RULE 0 (TASK 2 ARCHITECT TEAM)  
**Date:** 2026-09-05  
**Baseline Git Commit:** `0414dee` (`0414deeb663806f36ee35a7206b02a5c531d041c`)  
**Status:** ARCHITECTURE PLAN COMPLETE — NO SOURCE MODIFIED  

---

## 1. Scope of Modifications (Files to Modify / Create)

Per Rule 0, only the files explicitly listed below are permitted to be modified or created:

| Action | Path | Responsibility |
|---|---|---|
| **CREATE** | `kernel/debug/bofs/bofs_forensic_dashboard.h` | Dashboard function declarations, timeline structures, snapshot limits, and evidentiary classifications. |
| **CREATE** | `kernel/debug/bofs/bofs_forensic_dashboard.c` | In-kernel 2560x1600 ABDE forensic dashboard, hardware queries, stack layer verifications, first-failure engine, heartbeat loop. |
| **MODIFY** | `kernel/kernel.c` | Define `ATOMS_DEBUG_MODE_BOFS_PHASE12`, set as active debug mode, and add dispatcher invocation branch. |
| **MODIFY** | `build.ps1` | Add `bofs_forensic_dashboard.c` compilation rule and add `build/bofs_forensic_dashboard.o` to linker script. |
| **CREATE** | `tools/bofs/test_phase12_forensic.py` | Complete host test runner implementing the 48-test matrix (T01–T48) with 1,000-cycle stress test. |
| **CREATE** | `tools/bofs/test_phase12_qemu.py` | Pure UEFI QEMU runner with telnet screendump, serial telemetry monitor, and PNG artifact generation. |
| **CREATE** | `docs/BOFS/PHASE12_FORENSIC_DEBUG_DASHBOARD.md` | Formal architecture specification and forensic operations reference. |
| **CREATE** | `docs/BOFS/PHASE12_MASTER_CERTIFICATION_REPORT.md` | Master forensic certification report matching Section 64 template. |
| **CREATE** | `PHASE12_MASTER_CERTIFICATION_REPORT.md` | Root workspace mirror of master certification report. |

---

## 2. Technical Architecture & Layer Breakdown

### A. Evidential Classifications
All dashboard elements and metrics must be color-coded and tagged by evidential weight:
1. `[OBSERVED]` (Cyan, `0x0038BDF8`): Directly read hardware register, CPUID result, memory buffer, or raw disk sector.
2. `[DERIVED]` (Purple, `0x00A78BFA`): Mathematically computed from observed values (e.g. allocation percentage, fragmentation ratio).
3. `[PROVEN]` (Green, `0x0022C55E`): Verified through active assertion testing (e.g. CRC32 match, W^X enforcement, 0 drift).
4. `[INFERRED]` (Amber, `0x00FBBF24`): Plausible causal deduction across layers.
5. `[UNKNOWN]` (Slate, `0x0064748B`): State not yet probed or indeterminable.
6. `[NOT TESTED]` (Muted, `0x0094A3B8`): Subsystem intentionally skipped or unavailable on current target.

### B. Master Stack Hierarchy
The dashboard must visualize and test 16 distinct vertical layers:
```
1.  Physical Hardware (CPU brand, cores, RAM MB)
2.  PCI Storage Controllers (AHCI / NVMe detection)
3.  Block Devices (dev name, capacity, sector size, read/write/flush)
4.  Partitions (GPT/MBR, partition index, start LBA, sector count)
5.  BOFS Superblock (magic 0x53464F42, version, geometry, CRC)
6.  Backup Superblock (comparison, match/difference/not implemented)
7.  Allocation (free/used block bitmap, OOB checks, double-free rejection)
8.  Fragmentation (extents, contiguous vs fragmented, gap analysis)
9.  Inodes & Metadata (magic 0x4F4E4942, type, size, mode, UID/GID, CRC)
10. File Data (direct/indirect extents, multi-block read/write, integrity)
11. Directory & B+Tree (root/internal/leaf nodes, depth, splits, canonical UTF-8)
12. Security & DAC (Phase 7 permissions: 0644/0600 evaluation, decision trace)
13. Write-Ahead Log (journal start/size, head/tail, transaction commit states)
14. VFS Layer (mount points, path resolution, FD table allocation/closing)
15. Syscall & Ring 3 (sys_service_exec, pointer bounds checks, usermode context)
16. BOSX & File Manager (executable format validation, dynamic UI correlation)
```

### C. First-Failure Detection Algorithm
When an error occurs during end-to-end traversal:
- Scan layers bottom-up: Storage -> BlockDev -> Partition -> BOFS -> VFS -> Syscall -> Ring 3 -> App.
- Mark the lowest failing component with an unambiguous alert: `FIRST FAILURE ROOT CAUSE`.
- Suppress misleading false-blame downstream alerts.

### D. Cross-Layer Event Timeline Ring Buffer
Maintain an in-memory 16-entry circular event buffer logging timestamped operations:
`HH:MM:SS.mmm [LAYER] [ACTION] [RESULT]`
Display the last 8 events in the dashboard timeline card.

### E. Absolute Storage Safety
- Mock block device abstraction (`mock_p11_bofs_vfs` / `mock_p12_bofs_vfs`) used for in-kernel filesystem mutation tests.
- Physical device writes locked (`dev->read_only = true`).
- Foreign storage writes strictly enforced to **0 BYTES**.
- Physical BOFS volume status prominently displayed as:
  `PHYSICAL BOFS STORAGE: NOT TESTED — NO DEDICATED BOFS VOLUME AVAILABLE.`

---

## 3. Expected Results & Success Criteria

1. **Compilation:** Kernel and bootloader compile cleanly with 0 errors.
2. **Host Test Matrix:** `tools/bofs/test_phase12_forensic.py` passes 48 / 48 tests.
3. **QEMU Pre-Flight:** Pure UEFI QEMU boots into GOP `2560x1600` dashboard; all 16 stack layers verified; heartbeat spinner actively rotating (`| / - \`); COM1 logs `MASTER CERTIFICATION PASS`.
4. **Drift:** 0 FD drift, 0 Inode drift, 0 Block drift, 0 Frame drift, 0 Process drift after 1,000 cycles.
5. **Safety:** Foreign storage writes remain strictly 0 bytes.

---

## 4. Rollback Plan

If any regression occurs:
1. Revert `kernel/kernel.c` back to `#define ATOMS_ACTIVE_DEBUG_MODE ATOMS_DEBUG_MODE_BOFS_PHASE11`.
2. Remove `build/bofs_forensic_dashboard.o` from `build.ps1`.
3. `git checkout 0414dee -- .` to restore pristine Phase 11 baseline.

---
*Task 2 Architecture Plan Complete. Ready for Implementation Plan Artifact & Task 3.*
