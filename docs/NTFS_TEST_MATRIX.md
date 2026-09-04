# ATOMS OS — Real Hardware Progressive NTFS Test Matrix
**Document ID:** `NTFS-TEST-MATRIX-V1.0`  
**Target Platform:** ASUS PRIME B750M-K (Intel Core i3-14100F, Haswell/RaptorLake LGA1700)  
**Storage Target:** WD Blue SN5000 500GB NVMe SSD (`nvme0n1`, Serial: `25211F806396`)  
**Volume Target:** Partition 3 (Start LBA: `239,616`, Size: `243.30 GB` NTFS)  
**Authors:** Developer A (Architecture) & Developer B (Forensics)  
**Date:** 2026-09-04  

---

## 1. Test Methodology & Evidence Gates

Testing progresses strictly from low-risk baseline operations to high-order structural mutations.
A test level may **only** be marked `PASS` when:
1. Operation completes in ATOMS OS without faults.
2. Read-back in ATOMS confirms byte-for-byte data integrity.
3. System reboot into ATOMS confirms persistence on physical media.
4. Cross-boot into physical Windows 11 mounts volume cleanly with zero `BugCheck 0x24`, zero `chkdsk` prompts, and full file visibility in Windows Explorer / Notepad.

---

## 2. Progressive 24-Stage Real Hardware Test Register

| Stage | Test Name | Target Objective | Expected Physical Result | Validation Method | Hardware Status |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **01** | **Empty File Creation** | Create `/ATOMS_EMPTY.txt` (0 bytes) | Free MFT record allocated, bit set in `$MFT::$BITMAP`, resident $DATA (0B), linked in parent index. | Hex inspection of MFT record + bitmap bit. | ⏳ PENDING |
| **02** | **Resident File Creation** | Create `/ATOMS_RESIDENT.txt` ($\le 256$ B) | Formatted MFT record with resident payload; zero external clusters allocated. | Telemetry read-back + USA verification. | ⏳ PENDING |
| **03** | **Deterministic Data Write** | Write known pattern `0x55AA55AA...` | Exact payload resident in `$DATA` attribute stream. | Byte-for-byte SHA256 / CRC32 comparison. | ⏳ PENDING |
| **04** | **Immediate Read-Back** | Read file via `ntfs_file_read()` | Returns exact 100% matching payload. | In-kernel buffer assertion. | ⏳ PENDING |
| **05** | **Handle Close & Reopen** | Close file handle, flush cache, reopen | Re-traverses B-tree, parses MFT record, reads data. | Cache hit/miss telemetry verification. | ⏳ PENDING |
| **06** | **Warm Reboot into ATOMS** | Issue hardware reset, PXE boot ATOMS | File record persists on physical NVMe flash. | Bare-metal automated autopsy runner. | ⏳ PENDING |
| **07** | **Post-Reboot Read-Back** | Read `/ATOMS_RESIDENT.txt` in fresh boot | Validates cold NVMe flash persistence. | COM1 / UDP 9999 telemetry confirmation. | ⏳ PENDING |
| **08** | **Cross-Boot into Windows 11**| Reboot into native Windows 11 installation | `ntfs.sys` mounts volume cleanly; zero BugChecks. | Windows Desktop loaded without `chkdsk`. | ⏳ PENDING |
| **09** | **Windows Notepad Verification**| Open `C:\ATOMS_RESIDENT.txt` in Windows | File content displays cleanly in Notepad. | Manual user visual confirmation. | ⏳ PENDING |
| **10** | **Multiple Resident Files** | Create 10 files in root directory | All 10 records allocated, bits set in bitmap, sorted collation maintained. | MFT scan + index dump. | ⏳ PENDING |
| **11** | **Subdirectory Creation** | Create `/ATOMS_DIR/` | New MFT record with `flags = 0x0003`, resident `$INDEX_ROOT`. | Directory enumeration via `ntfs_dir_enum()`.| ⏳ PENDING |
| **12** | **File Rename** | Rename `/ATOMS_RESIDENT.txt` to `/ATOMS_RENAMED.txt` | Removes old index entry, inserts new entry in sorted position, updates `$FILE_NAME`. | Directory B-tree traversal. | ⏳ PENDING |
| **13** | **File Deletion** | Delete `/ATOMS_RENAMED.txt` | Removes entry from index, clears record flags to `0x0000`, clears bitmap bit, increments sequence. | Bitmap bit confirmed 0, record flagged free. | ⏳ PENDING |
| **14** | **File Re-Creation** | Re-create `/ATOMS_RENAMED.txt` in same slot | Reuses freed MFT record, verifies sequence number incremented ($S+1$). | On-disk sequence number verified $S+1$. | ⏳ PENDING |
| **15** | **Small Non-Resident File** | Create 4096-byte file (1 cluster) | Allocates 1 cluster via Volume `$Bitmap`, writes cluster, encodes single-run data runlist. | Volume `$Bitmap` bit verified; runlist decoded.| ⏳ PENDING |
| **16** | **Large Non-Resident File** | Create 1 MB file (256 clusters) | Multi-cluster contiguous or fragmented runlist in `$DATA`. | Multi-cluster sector read-back. | ⏳ PENDING |
| **17** | **Cluster Boundary Cross** | Write payload spanning cluster boundary (e.g. 8192 bytes) | Verifies multi-cluster read/write translation across LCNs. | End-to-end CRC32 check across cluster edge. | ⏳ PENDING |
| **18** | **MFT Extent Boundary Cross**| Allocate record across MFT extent boundary | Canonical extent mapper maps record across Extent 0 and Extent 1. | MFT Extent 1 physical LBA verification. | ⏳ PENDING |
| **19** | **Directory Expansion** | Add files until `$INDEX_ROOT` is full | Reaches maximum resident index capacity without corruption. | Index slack space telemetry monitoring. | ⏳ PENDING |
| **20** | **Directory B-Tree Split** | Add 1 more file triggering node split | Converts to Two-Tier B-Tree; allocates 4096B `"INDX"` block; sets `$INDEX_ALLOCATION`. | Two-tier B-tree verification in ATOMS & Win11.| ⏳ PENDING |
| **21** | **Controlled Power Interruption**| Simulate power drop during Stage 2 vs Stage 3 | Proves rollback restores volume consistency. | Post-power-loss consistency autopsy. | ⏳ PENDING |
| **22** | **Multi-Tier Read in Windows** | Boot Windows 11 with two-tier directory | Windows Explorer enumerates expanded directory with zero errors. | Windows Explorer directory view. | ⏳ PENDING |
| **23** | **Cross-Modification Test** | Create file in Windows, read in ATOMS; create in ATOMS, read in Windows | 100% Bidirectional read/write interoperability verified. | Dual-OS cross-read validation. | ⏳ PENDING |
| **24** | **Long-Run Stress Test** | 1,000 rapid file creations and deletions | Zero memory leaks, zero cluster leaks, zero orphaned records. | Final forensic ledger + volume verification. | ⏳ PENDING |
