# ATOMS OS — BOFS Phase 12: Final Forensic Debug Dashboard
**Document ID:** ATOMS-BOFS-PHASE12-DOC-001  
**Phase:** 12 of BOFS Certification Series  
**Status:** CERTIFIED PASS  
**Commit Baseline:** `0414dee`  
**Target Hardware Profile:** ASUS PRIME B750M-K (Intel Haswell / QEMU x86_64, 8GB RAM, Pure UEFI Mode)  
**Date:** 2026-09-05  

---

## 1. Objective & Architectural Purpose

Phase 12 is the final forensic observability and cross-layer verification phase before Phase 13 dedicated bare-metal physical storage certification.

Its mission:
> **Build one authoritative forensic dashboard capable of observing, validating, correlating, and proving the complete BOFS stack layer-by-layer before ATOMS is allowed to operate on a dedicated real BOFS volume with real persistent files.**

### Core Evidentiary Principles
The dashboard distinguishes every metric by evidentiary weight:
- **`OBSERVED`**: Directly measured from hardware registers, memory structures, or raw disk sectors.
- **`DERIVED`**: Formally calculated from primary evidence.
- **`PROVEN`**: Verified through rigorous runtime assertions and test cycles.
- **`INFERRED`**: Correlated behavior deduced across layers.
- **`UNKNOWN`**: Unprobed or indeterminable state.
- **`NOT TESTED`**: Deliberately excluded subsystems (e.g. Physical BOFS Storage reserved for Phase 13).

---

## 2. Master Stack Cross-Layer Architecture

```
                  PHYSICAL HARDWARE
                         │
                         ▼
              PCI / STORAGE CONTROLLER
                         │
                         ▼
                    BLOCK DEVICE
                         │
                         ▼
                     PARTITION
                         │
                         ▼
                  BOFS SUPERBLOCK
                         │
                         ▼
                     ALLOCATION
                         │
                         ▼
                       INODE
                         │
                         ▼
                     FILE DATA
                         │
                         ▼
                 DIRECTORY / B+TREE
                         │
                         ▼
                      SECURITY
                         │
                         ▼
                        WAL
                         │
                         ▼
                        VFS
                         │
                         ▼
                      SYSCALL
                         │
                         ▼
                      RING 3
                         │
                         ▼
                       BOSX
                         │
                         ▼
                   FILE MANAGER
```

---

## 3. High-Density 4-Panel Dashboard Layout (2560x1600)

1. **Panel 1: Hardware & Storage Layer**
   - Live CPU brand & core count via CPUID.
   - Total RAM calculation from UEFI `boot_info` memory map.
   - Block device enumeration (`block_device_count()`, `block_device_get()`).
   - Partition table mapping (GPT / MBR).
   - Superblock magic (`0x53464F42`) & IEEE 802.3 CRC32 verification.
   - Backup Superblock comparison.
   - Bitmap block allocation & OOB validation.

2. **Panel 2: Filesystem Core & Reliability**
   - Inode table integrity (`0x4F4E4942`).
   - Direct and indirect extent data mapping.
   - Directory entry lookup & B+Tree topology.
   - Phase 7 DAC security enforcement (0644 vs 0600 mode decision trace).
   - Write-Ahead Log (WAL) transaction journal states.
   - Crash recovery and transaction replay simulation.
   - 1,000-cycle stress test verification with zero drift.

3. **Panel 3: Userspace Runtime & File Manager**
   - VFS dynamic mount manager (`/` root mount).
   - Syscall gateway and ABI parameter validation.
   - Ring 3 userspace execution isolation.
   - BOSX binary loader and W^X security enforcement.
   - File Manager UI ↔ VFS correlation.
   - Read-only write-lock enforcement for storage safety.

4. **Panel 4: Diagnostics, Timeline & Safety**
   - First-Failure Root Cause Detection.
   - Resource drift counters (FD: 0, Inode: 0, Block: 0, Frame: 0, Process: 0).
   - Physical BOFS storage status: `NOT AVAILABLE (PHASE 13 SCOPE)`.
   - Foreign disk writes: strictly `0 BYTES`.
   - Circular Event Timeline (last 3-8 timestamped events).

---

## 4. First-Failure Detection Engine

When an anomaly or fault is detected, the dashboard evaluates layers bottom-up:
1. `Physical Hardware`
2. `Block Device`
3. `Superblock`
4. `Allocation`
5. `Inode`
6. `File Data`
7. `Directory / B+Tree`
8. `Security`
9. `WAL`
10. `VFS`
11. `Syscall`
12. `Ring 3`
13. `BOSX`
14. `File Manager`

The first failing layer is highlighted as `FIRST FAILURE ROOT CAUSE`, preventing misleading secondary blame.

---

## 5. Physical Storage Safety Protocol

Until Phase 13:
- **Foreign Physical Storage:** Strictly Read-Only (`dev->read_only = true`).
- **Protected Partitions:** Windows NTFS, EFI System Partition, MSR, Recovery.
- **Foreign Storage Writes:** Strictly **0 BYTES**.
- **Physical BOFS Status:** `PHYSICAL BOFS STORAGE: NOT TESTED — NO DEDICATED BOFS VOLUME AVAILABLE.`
