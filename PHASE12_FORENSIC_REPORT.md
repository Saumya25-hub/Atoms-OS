# ATOMS OS — FORENSIC REPORT (TASK 1)
## BOFS Phase 12: Final Forensic Debug Dashboard & Cross-Layer Observability

**Document ID:** ATOMS-BOFS-PHASE12-FORENSIC-001  
**Protocol:** ATOMS OS Engineering Protocol V1 — RULE 0 (TASK 1 FORENSIC TEAM)  
**Date:** 2026-09-05  
**Baseline Git Commit:** `0414dee` (`0414deeb663806f36ee35a7206b02a5c531d041c`)  
**Status:** FORENSIC AUDIT COMPLETE — NO SOURCE MODIFIED  

---

## 1. Objective & Forensic Mission

Phase 12 constitutes the final pre-physical-storage observability and verification gate before Phase 13 dedicated bare-metal physical BOFS certification.

Its primary objectives are:
1. Construct an authoritative in-kernel forensic debug dashboard exposing the entire vertical ATOMS filesystem stack:
   ```
   Physical Hardware -> PCI/Storage -> Block Device -> Partition ->
   BOFS Superblock -> Allocation -> Inode -> File Data -> Directory / B+Tree ->
   Security (DAC) -> WAL -> VFS -> Syscall -> Ring 3 -> BOSX -> File Manager
   ```
2. Enforce strict evidential classification across all diagnostic fields:
   - `OBSERVED`: Directly measured hardware/memory values (e.g. CPU brand, RAM, PCI devices, Superblock bytes).
   - `DERIVED`: Computed metrics from primary evidence (e.g. allocation percentage, free blocks, B+Tree depth).
   - `PROVEN`: Verified by rigorous diagnostic tests (e.g. CRC matches, W^X rejection, 1,000-cycle stress).
   - `INFERRED`: Plausible correlation without direct hardware proof.
   - `UNKNOWN`: Unverified or unprobed state.
   - `NOT TESTED`: Subsystem or feature deliberately excluded (e.g. Physical BOFS Storage).
3. Implement First-Failure Detection: Rather than merely reporting downstream faults, pinpoint the exact layer where failure originates.
4. Absolute Storage Safety: Enforce strictly `FOREIGN STORAGE WRITES = 0 BYTES` against existing NVMe/SATA partitions (Windows NTFS, EFI, MSR, Recovery).

---

## 2. Existing Debug Infrastructure Audit Map (Section 3)

| File | Function / Symbol | Line | Layer | Capability | Reusable | Protected Subsystem |
|---|---|---|---|---|---|---|
| `kernel/debug/abde/abde.c` | `abde_render_string()`, `abde_fill_rect()` | 45-120 | Display / Diagnostics | 8x16 font rendering, colored panels, screen clears | Yes | GOP Framebuffer |
| `kernel/debug/abde/abde.h` | `g_abde` engine structure | 40-146 | Global Diagnostics | Telemetry state, screen dimensions, CPU status | Yes | ABDE State |
| `kernel/debug/storage_forensic_debug.c` | `forensic_emit()` | 44-63 | Telemetry Gateway | Dual COM1 serial & UDP broadcast (port 9999) | Yes | Serial & Net Telemetry |
| `kernel/debug/storage_forensic_debug.c` | Hardware detection helpers | 120-250 | Hardware / PCI | PCI bus enumeration, AHCI / NVMe detection | Yes | PCI / Storage Drivers |
| `kernel/vfs/vfs_legacy/storage/include/block_device.h` | `block_device_count()`, `block_device_get()` | 35-43 | Storage Layer | Block device abstraction & capability queries | Yes | Block Device Registry |
| `kernel/vfs/bofs/include/bofs_validator.h` | `bofs_crc32()` | 27 | Integrity Layer | IEEE 802.3 CRC32 verification | Yes | BOFS Validator |
| `kernel/vfs/bofs/include/bofs_format.h` | `bofs_superblock_t`, `bofs_inode_t` | 50-320 | On-Disk Format | Layout structures, geometry formulas | Yes | BOFS Format Specification |
| `kernel/vfs/bofs/include/bofs_dir.h` | `bofs_dir_node_t` | 20-80 | Directory Layer | B+Tree node topology and keys | Yes | B+Tree Engine |
| `kernel/vfs/bofs/include/bofs_wal.h` | `bofs_wal_t` | 30-70 | Reliability Layer | Write-Ahead Log state machine | Yes | WAL Engine |
| `kernel/core/syscall/include/syscall.h` | `sys_service_exec()` | 80-95 | Syscall Layer | Ring 3 gateway, pointer sanitization | Yes | Syscall Gate |
| `kernel/debug/screenshot/atoms_screenshot.h` | `atoms_screenshot_capture_cooperative()` | 15 | Diagnostic Capture | Non-blocking cooperative screenshot | Yes | XHCI / Input Pipeline |

---

## 3. Forensic Identification of Deficiencies

1. **Lack of Unified Multi-Layer Dashboard:** Current diagnostic tests (Phases 3–11) test individual layers independently (`bofs_format_test`, `bofs_allocation_test`, `bofs_wal_test`, etc.), but do not provide a single authoritative visual and programmatic console correlating hardware, block device, filesystem, VFS, syscall, process, and file manager state.
2. **Missing First-Failure Root Cause Analysis:** When a higher-layer component fails (e.g. File Manager cannot open a file), existing tests log downstream errors without tracing whether the root cause was storage I/O, superblock corruption, B+Tree traversal failure, permission denial, or syscall pointer rejection.
3. **Absence of Unified Cross-Layer Event Timeline:** No mechanism currently logs timestamped cross-layer sequences (`UI -> Syscall -> VFS -> BOFS -> WAL -> Storage`) in an in-memory ring buffer for post-mortem analysis.
4. **Physical Storage Boundary Isolation:** The system must clearly delineate that physical BOFS volume testing is reserved for Phase 13, explicitly reporting `PHYSICAL BOFS STORAGE: NOT TESTED — NO DEDICATED BOFS VOLUME AVAILABLE` and guaranteeing zero foreign writes.

---

## 4. Risk Analysis

- **Storage Write Risk:** Absolute prohibition on writing to physical NVMe or SATA devices containing Windows NTFS, EFI, or system partitions. Any write to an uncertified volume represents critical data loss risk.
- **Display Stability Risk:** Visual rendering must use proven ABDE direct framebuffer primitives without introducing nested loops, large synchronous memory allocations, or blocking network calls.
- **Drift Risk:** The dashboard and its test runner must achieve zero leak across 1,000 test cycles for file descriptors, memory frames, page mappings, and inodes.

---

## 5. Proposed Architectural Resolution (Non-Code)

1. Design `bofs_forensic_dashboard.h` and `bofs_forensic_dashboard.c` under `kernel/debug/bofs/` to render a 2560x1600 high-density 4-panel diagnostic matrix:
   - Panel 1: Hardware, Storage Controllers, Block Devices & Partitions.
   - Panel 2: BOFS Core (Superblock, Allocation, Inodes, B+Tree, Security, WAL).
   - Panel 3: Runtime Stack (VFS, FDs, Syscalls, Ring 3, BOSX, File Manager).
   - Panel 4: Invariants, Cross-Layer Timeline, First-Failure Detection & Drift Counters.
2. Develop `tools/bofs/test_phase12_forensic.py` executing the comprehensive 48-test matrix (T01–T48).
3. Create `tools/bofs/test_phase12_qemu.py` validating UEFI boot, ABDE dashboard rendering, and COM1 serial telemetry.
4. Produce exhaustive documentation in `docs/BOFS/PHASE12_FORENSIC_DEBUG_DASHBOARD.md` and `docs/BOFS/PHASE12_MASTER_CERTIFICATION_REPORT.md`.

---
*Task 1 Forensic Investigation Complete. Awaiting Architecture Plan (Task 2).*
