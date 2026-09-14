# ATOMS OS — PHASE 2: BOFS V1 REQUIREMENTS & ARCHITECTURAL BLUEPRINT

**Document ID:** `ATOMS-BOFS-PHASE2-ARCH-001`  
**Classification:** ARCHITECTURAL SPECIFICATION & SYSTEM DESIGN BLUEPRINT (DESIGN-ONLY FORENSIC PHASE)  
**Author:** ATOMS OS Core Engineering & Filesystem Architecture Team  
**Date:** 2026-09-04  
**Hardware Baseline:** ASUS PRIME B750M-K (Intel Core i3-14100F, Haswell/Raptor Lake x86_64, WD Blue SN5000 500GB NVMe SSD, 8GB RAM)  
**Git Safety State:** Checkpoint Commit `c8b5841`  
**Rule 0 Compliance:** Strictly Architectural Specification — ZERO SOURCE CODE MODIFICATIONS.

---

## 1. EXECUTIVE SUMMARY

### 1.1 What Exactly is BOFS?
**BOFS (BOS Operating Filesystem)** is the native, first-class filesystem designed specifically for ATOMS OS / BOS Kernel. It is a modern, 64-bit, extent-based, crash-consistent, write-ahead journaled filesystem engineered from first principles for high-speed non-volatile memory (PCIe NVMe SSDs) and native 64-bit x86_64 OS workloads.

**BOFS is NOT NTFS, is NOT Linux ext4, and is NOT a derivative or fork of any Windows or Linux filesystem codebase.** While industry standards (POSIX.1-2017, Microsoft MS-FSCC, Linux VFS, and Unix filesystem principles) have been forensically studied as behavioral references, BOFS introduces a clean, unencumbered, native ATOMS architecture designed from scratch without historical baggage.

### 1.2 What Problem Does BOFS Solve?
Prior to BOFS, ATOMS OS relied on two disparate filesystem drivers:
1. **FAT32** (`kernel/vfs/vfs_legacy/fs/fat32/`): Limited by a 32-bit file size ceiling (4 GB max), vulnerable to catastrophic cluster chain fragmentation, completely devoid of security, permissions, or ownership, and lacking any journaling or crash consistency mechanisms `[CODE OBSERVED]`.
2. **NTFS** (`kernel/vfs/vfs_legacy/fs/ntfs/`): Certified production-ready for read-only forensic evidence extraction across fragmented 465 GB Windows volumes `[REAL HARDWARE PROVEN]`, but proven mathematically and structurally unsafe for general write operations due to extreme proprietary complexity (1024-byte MFT records, USA sector fixups, dual $INDEX_ROOT / $INDEX_ALLOCATION B-trees, uncommitted $Bitmap collisions, and Windows BugCheck 0x24 risks) `[PHASE 1 FORENSIC REPORT]`.

**BOFS V1 solves this fundamental OS storage deficit** by providing:
- High-performance, 4096-byte aligned, direct DMA extent storage on NVMe SSDs without bounce-buffer memory overhead.
- True atomic crash consistency via a dedicated Write-Ahead Logging (WAL) journal ring buffer.
- Native POSIX Discretionary Access Control (DAC) with 32-bit UIDs, 32-bit GIDs, and 12-bit permission mode masks enforced at the ATOMS syscall gate.
- Direct runtime integration with native `.BOSX` binary executables, establishing hardware-enforced execution permission checks.
- End-to-end data integrity protection using IEEE 802.3 CRC32 checksums embedded across all structural metadata blocks.

---

## 2. CURRENT ATOMS REALITY AUDIT

Before establishing the BOFS design, the live ATOMS OS codebase was audited to identify exact boundaries, constraints, and integration hooks.

### 2.1 Epistemological Classification Key
- **`[OBSERVED]`**: Directly verified via static source code inspection (with exact file and line references).
- **`[PROVEN]`**: Empirically measured and certified on physical hardware (ASUS B750M-K) or QEMU.
- **`[INFERRED]`**: Derived logically from architectural evidence.
- **`[UNKNOWN]`**: Explicitly unverified in source code; requires future empirical determination.
- **`[PLANNED]`**: Architecture intended for implementation in later phases.

### 2.2 Subsystem Inspection & BOFS Architectural Implications

| Subsystem | Exact File & Function | Current Observed Behavior | BOFS Architectural Implication |
| :--- | :--- | :--- | :--- |
| **VFS Driver Contract** | [`kernel/vfs/vfs_legacy/include/vfs.h:17-35`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs.h#L17-L35) | `FilesystemDriver` interface defines callbacks: `mount`, `unmount`, `open`, `read`, `write`, `close`, `readdir`, `mkdir`, `create`, `rename`, `delete`. | BOFS must implement this exact struct in `bofs_vfs.c` for seamless registration into `filesystem_registry` `[OBSERVED]`. |
| **VFS Node Representation** | [`kernel/vfs/vfs_legacy/include/vfs_node.h:15-30`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs_node.h#L15-L30) | `VFS_Node` stores `char name[64]`, `uint32_t size`, `void* private_data`. Has no UID, GID, or mode permissions `[OBSERVED]`. | `VFS_Node` represents a 32-bit size limit (4 GB max). In Phase 9, `VFS_Node` must be augmented or bridged to support 64-bit sizes (`uint64_t`) and POSIX credentials. |
| **Open File Descriptors** | [`kernel/vfs/vfs_legacy/src/vfs.c:20`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L20) | Static global array `g_fd_table[32]` with 64-bit `offset` `[OBSERVED]`. | VFS supports 32 concurrent open files system-wide. BOFS file offsets are already 64-bit capable at the file descriptor level. |
| **NVMe Block Device** | [`kernel/drivers/storage/nvme/nvme.c:577-593`](file:///d:/Signatures_OS/kernel/drivers/storage/nvme/nvme.c#L577-L593) | Registers `"nvme0n1"` with `sector_size=512`, `sector_count`, and callbacks `read`, `write`, `flush` `[REAL HARDWARE PROVEN]`. | BOFS must translate 4096-byte logical blocks to 512-byte hardware sectors (8 sectors per block) and invoke `dev->flush()` for transaction sync. |
| **AHCI SATA Controller** | [`kernel/drivers/storage/ahci/ahci.c:36`](file:///d:/Signatures_OS/kernel/drivers/storage/ahci/ahci.c#L36) | Implements SATA AHCI 1.3 `BlockDevice` interface with DMA bounce frames `[OBSERVED]`. | BOFS will operate identically on SATA SSDs/HDDs via the universal `BlockDevice` abstraction. |
| **Disk Partition Manager** | [`kernel/vfs/vfs_legacy/storage/src/disk_manager.c:19-51`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/storage/src/disk_manager.c#L19-L51) | Translates logical partition LBAs: `absolute_lba = logical_data->start_lba + lba` with boundary checking `[OBSERVED]`. | BOFS sees a partition-relative block 0; partition isolation is strictly enforced by the disk manager. |
| **GPT Partition Table** | [`kernel/drivers/storage/partition/gpt.c:7-15`](file:///d:/Signatures_OS/kernel/drivers/storage/partition/gpt.c#L7-L15) | Parses GPT GUIDs (`GUID_BASIC_DATA`, `GUID_EFI_SYSTEM`) `[OBSERVED]`. | BOFS can define a unique native partition type GUID: `GUID_BOFS_SYSTEM` (`0x424F4653-B0F5-4154-4F4D-535F4F530001`). |
| **Physical Memory (PMM)** | [`kernel/core/memory/pmm/include/pmm.h`](file:///d:/Signatures_OS/kernel/core/memory/pmm/include/pmm.h) | Page allocator providing 4096-byte physical frames via `pmm_alloc_page()` `[OBSERVED]`. | Perfectly aligns with the 4096-byte BOFS block size. Buffer allocations can map 1-to-1 with physical pages. |
| **Virtual Memory (VMM)** | [`kernel/core/memory/vmm/include/vmm.h:11-27`](file:///d:/Signatures_OS/kernel/core/memory/vmm/include/vmm.h#L11-L27) | Defines `VMM_PAGE_SIZE 4096ULL`, canonical address checks, and `vmm_validate_user_range()` `[OBSERVED]`. | BOFS read/write buffers can directly map into user virtual address spaces during file I/O and executable loading. |
| **Kernel Heap (Heap)** | [`kernel/core/memory/heap/include/heap.h:9-15`](file:///d:/Signatures_OS/kernel/core/memory/heap/include/heap.h#L9-L15) | **Rule 18**: Heap NEVER owns memory; manages allocations via VMM. Red zone canaries (`0xCAFEBABE8BADF00D`), BMLE threshold 1 MB `[OBSERVED]`. | BOFS in-memory structures must respect Rule 18: use bounded static pools or explicit `kmalloc()`/`kfree()` with zero memory leaks. |
| **Syscall Interface** | [`kernel/core/syscall/include/syscall.h:35-55`](file:///d:/Signatures_OS/kernel/core/syscall/include/syscall.h#L35-L55) | Exposes `SYS_OPEN` (14), `SYS_READ` (15), `SYS_CLOSE` (25), `SYS_SEEK` (26), `SYS_WRITE_FILE` (29). `MAX_SYSCALL` is 32 `[OBSERVED]`. | Missing POSIX syscalls (`readdir`, `mkdir`, `create`, `rename`, `unlink`, `stat`, `sync`, `chmod`, `chown`) must be architected for Phase 9. |
| **User Pointer Validation**| [`kernel/core/syscall/src/services.c:652-673`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c#L652-L673) | Calls `syscall_validate_user_string(path, 256)` and `syscall_validate_user_ptr(buf, count)` before any VFS invocation `[OBSERVED]`. | All future BOFS-facing syscalls must mandate these checks to prevent kernel memory corruption from Ring 3 payloads. |
| **Process Security Context**| [`kernel/core/process/process_manager.h:44-50`](file:///d:/Signatures_OS/kernel/core/process/process_manager.h#L44-L50) | `ATOMS_ProcessSecurityContext` contains `uint32_t user_id`, `uint32_t group_id`, `uint64_t capability_mask` `[OBSERVED]`. | Process credentials ALREADY EXIST in ATOMS! BOFS will directly evaluate `proc->security.user_id` against Inode UID/GID. |
| **Native Executable Loader**| [`kernel/core/loader/bosx_format.h:15-51`](file:///d:/Signatures_OS/kernel/core/loader/bosx_format.h#L15-L51) | `BOSX_Header` magic `'BOSX'` (`0x58534F42`), sections (`BOSX_SEC_EXEC`, `READ`, `WRITE`) `[OBSERVED]`. | BOFS acts strictly as a byte/extent provider to `BOSX_Load()`. Inode `mode` bit enforces execution rights before parsing header. |
| **Desktop Explorer View** | [`kernel/shell/apps/explorer.h:10-25`](file:///d:/Signatures_OS/kernel/shell/apps/explorer.h#L10-L25) | Pure View Layer: owns zero filesystem data. All operations delegate to BSOM (`BSOMObject*`, `BSOM_GetChildren`) `[OBSERVED]`. | Explorer interacts with BOFS via BSOM and VFS. BOFS does not need GUI code; it provides standard directory enumeration. |

---

## 3. BOFS DESIGN PRINCIPLES

1. **First-Principles Simplicity**: Discard legacy workarounds from the 1980s and 1990s. No 8.3 short filename generation, no Update Sequence Array (USA) sector fixups, no complex MFT attribute resident/non-resident polymorphic lists.
2. **NVMe Hardware Alignment**: Enforce a mandatory 4096-byte block quantum aligned to solid-state memory page boundaries, minimizing flash controller write amplification.
3. **Deterministic Predictability**: Fixed 512-byte Inode records, pre-allocated Inode tables, bounded B+Tree directory split algorithms, and circular Write-Ahead Log buffers ensure constant-time $O(1)$ and logarithmic $O(\log N)$ bounds.
4. **Crash Consistency as a Day-1 Primitive**: The filesystem is never left in an unrecoverable state. No write operation touches live metadata in-place without prior serialization in the journal ring.
5. **Native Security Architecture**: Access control is not bolted on as an afterthought. Every file object possesses explicit UID, GID, and POSIX permission mode bits from the exact moment of creation.
6. **Clean-Room Boundary**: Zero line-by-line copying, decompilation, or code sharing from GPL-licensed projects (Linux `ext4`, `ntfs3`, `btrfs`, `ntfs-3g`). Design from published behavioral standards and mathematical specifications only.

---

## 4. REFERENCE STUDY & DESIGN TAXONOMY

Each architectural decision in BOFS is classified according to its epistemological lineage:

```text
[REFERENCE-INSPIRED]: Sourced from published specifications or OS literature.
[ATOMS-NATIVE]:       Designed specifically for the unique architecture of ATOMS OS.
[REUSED CONCEPT]:     Proven classical computer science principle applied to BOFS.
[NEW BOFS DESIGN]:    Original architectural mechanism created for this filesystem.
```

### 4.1 Microsoft NTFS (MS-FSCC / MS-FSA)
- **Lessons Learned**:
  - *Strengths*: Extent-based runlists are compact and effective for large contiguous sequential I/O `[REUSED CONCEPT]`. Free-space bitmaps provide fast multi-cluster scanning.
  - *Weaknesses*: Variable-length attribute chains in 1024-byte MFT records create severe parser vulnerability. Sector fixup bytes (USA) are archaic. Dual directory indexing ($INDEX_ROOT in MFT + $INDEX_ALLOCATION in B-tree) creates fragile split states that corrupt easily during power loss.
- **BOFS Decision**: Discard MFT attribute lists and USA fixups. Adopt uniform fixed 512-byte Inodes with direct extent tuples `[NEW BOFS DESIGN]`. Discard dual directory trees in favor of a single uniform B+Tree `[REUSED CONCEPT]`.

### 4.2 Linux Kernel Filesystem & VFS Architecture
- **Lessons Learned**:
  - Clear separation between VFS operations (`inode_operations`, `file_operations`) and underlying block storage `[REFERENCE-INSPIRED]`.
  - Inode-based reference counting: files exist as long as `link_count > 0` or an open file descriptor references them `[REUSED CONCEPT]`.
  - Directory entries (`dentry`) mapping strings to Inode numbers decoupled from file data.
- **BOFS Decision**: Adopt the classical Inode-Dentry decoupled architecture for BOFS V1 `[REUSED CONCEPT]`.

### 4.3 Linux `ntfs3` & NTFS-3G
- **Lessons Learned**:
  - Strict commit ordering is required to prevent metadata corruption: allocate clusters $\rightarrow$ flush data $\rightarrow$ update Inode/extent $\rightarrow$ update directory $\rightarrow$ commit journal `[REFERENCE-INSPIRED]`.
  - Linux `ntfs3` median-key directory index splitting is necessary to prevent directory insertion deadlocks.
- **BOFS Decision**: Strictly enforce ordered metadata flush sequencing in the BOFS transaction pipeline `[NEW BOFS DESIGN]`.

### 4.4 UNIX / POSIX Standards (IEEE Std 1003.1)
- **Lessons Learned**:
  - Standard permission model: User (Owner), Group, Others with Read (4), Write (2), Execute (1) bits `[REFERENCE-INSPIRED]`.
  - Special bits: SetUID (`04000`), SetGID (`02000`), Sticky (`01000`) for directory access control.
  - Standard file types: Regular (`-`), Directory (`d`), Symlink (`l`), Character Device (`c`), Block Device (`b`), FIFO (`p`), Socket (`s`).
- **BOFS Decision**: Implement native POSIX mode bits and file types directly in the 512-byte Inode header `[ATOMS-NATIVE]`.

### 4.5 Git Bash & Practical Shell Expectations
- **Lessons Learned**:
  - Canonical forward slash `/` path normalization.
  - Special directory entries `.` (self) and `..` (parent) must always be present and enumerable in every directory `[REFERENCE-INSPIRED]`.
  - Atomic rename (`rename(old, new)`) must be crash-safe to support safe file editing patterns.
- **BOFS Decision**: Mandate `.` and `..` directory records and atomic directory rename transactions `[ATOMS-NATIVE]`.

---

## 5. BOFS VOLUME ARCHITECTURE

### 5.1 Volume Identity & Metadata
- **Magic Signature**: `0x53464F42` (`'BOFS'` in little-endian uint32) `[ATOMS-NATIVE]`.
- **Volume UUID**: 128-bit RFC 4122 randomly generated UUID uniquely identifying the partition.
- **Volume Label**: Up to 64 bytes of canonical UTF-8 text (e.g. `"ATOMS_SYSTEM"`, `"NVME_DATA"`).
- **Filesystem Version**: Major Version `1`, Minor Version `0`, ABI Version `1`.

### 5.2 Hybrid Region Layout
BOFS V1 utilizes a **Hybrid Region Layout**:
- Structural metadata zones (Superblock, Journal, Bitmaps, and Inode Table) reside in pre-allocated, fixed-offset regions at the beginning of the partition.
- File data and directory B+Tree index blocks are dynamically allocated from the general Data Block Pool.

**Architectural Justification for Hybrid Layout**:
Dynamic metadata allocation (like NTFS MFT dynamic extent growth) introduces extreme complexity, non-deterministic disk head / controller seeking, and circular allocation deadlocks (allocating a block to record where a block was allocated). A pre-allocated Inode table and fixed journal ring guarantee deterministic seek times, instant mount recovery, and zero metadata fragmentation `[DESIGN INVARIANT]`.

### 5.3 Volume Geometry Map

```text
+---------------------------------------------------------------------------------------------------+
| LBA Range (512B Sectors) | Block Range (4KB Blocks) | Subsystem Description                       |
+--------------------------+--------------------------+---------------------------------------------+
| LBA 0 - 7                | Block 0                  | Primary Superblock (4,096 Bytes)            |
| LBA 8 - 15               | Block 1                  | Backup Superblock 1 (4,096 Bytes)           |
| LBA 16 - 31              | Blocks 2 - 3             | Reserved Bootloader / Partition Anchor      |
| LBA 32 - 65,567          | Blocks 4 - 8,195         | Write-Ahead Journal Ring (8,192 Blks = 32MB)|
| LBA 65,568 - 65,575      | Block 8,196              | Inode Allocation Bitmap ($InodeBitmap, 4KB) |
| LBA 65,576 - 67,623      | Blocks 8,197 - 8,452     | Block Allocation Bitmap ($BlockBitmap, 1MB) |
| LBA 67,624 - 133,159     | Blocks 8,453 - 16,644    | Fixed Inode Table (65,536 Inodes = 32 MB)   |
| LBA 133,160 - End of Dev | Blocks 16,645 - N        | General Data Block Pool (Files & B+Trees)   |
+---------------------------------------------------------------------------------------------------+
```

### 5.4 Volume State Flags
- `BOFS_STATE_CLEAN` (`0x0001`): Clean unmount confirmed; fast-path mount enabled.
- `BOFS_STATE_DIRTY` (`0x0002`): Volume was not cleanly unmounted; mandatory journal replay required before servicing I/O.
- `BOFS_STATE_RECOVERING` (`0x0004`): Currently replaying journal transactions.
- `BOFS_STATE_DEGRADED` (`0x0008`): Unrecoverable checksum error detected; volume mounted in strict read-only mode.

---

## 6. BLOCK MODEL

### 6.1 Fundamental Storage Quantum
- **Logical BOFS Block Size**: Fixed at **4,096 bytes (4 KB)** `[DESIGN INVARIANT]`.
- **Physical Sector Translation**:
  - The driver queries `dev->sector_size` via the `BlockDevice` interface `[OBSERVED]`.
  - On standard 512-byte drives (`512e`), $1 \text{ Block} = 8 \text{ physical sectors}$.
  - On 4096-byte native drives (`4Kn`), $1 \text{ Block} = 1 \text{ physical sector}$.
  - All hardware I/O requests translate block address $B$ to sector $S = B \times (\text{BlockSize} / \text{SectorSize})$.

### 6.2 Volume & File Dimensions
- **Block Addressing Type**: Unsigned 64-bit integer (`uint64_t bofs_block_t`).
- **Maximum Addressable Blocks**: $2^{64} - 1$ blocks.
- **Maximum Volume Capacity**: $2^{64} \times 4096 \text{ bytes} \approx 75.5 \text{ Zettabytes}$ (Practical V1 volume limit: **16 Exabytes**).
- **Maximum File Capacity**: Native 64-bit size field: **16 Exabytes** ($2^{64} - 1$ bytes).

### 6.3 Alignment & Zero-Copy Direct DMA
- All block boundaries are naturally aligned to 4096 bytes.
- This matches the ATOMS x86_64 PMM page frame size `[OBSERVED]`.
- NVMe I/O submissions direct-map physical frame addresses into PRP1/PRP2 descriptors without CPU bounce buffering `[REAL HARDWARE PROVEN]`.

---

## 7. ALLOCATION MODEL

### 7.1 Allocation Strategy Comparison & Selection

| Allocation Mechanism | Memory Footprint | Contiguous Extent Search | Crash Rollback Complexity | Selected for BOFS V1? |
| :--- | :--- | :--- | :--- | :--- |
| **Simple Free List** | Minimal (O(1) in disk) | Terrible ($O(N)$ random fragmentation) | High (Pointers break during torn writes) | ❌ Rejected |
| **Extent-Based B-Tree**| Moderate | Good | Very High (Requires B-tree inside allocator) | ❌ Rejected for V1 |
| **Contiguous Bitmaps** | **1 bit per 4KB block** | **Fast ($O(N/64)$ 64-bit word scanning)** | **Simple (Atomic bit-flip journal records)**| **✅ SELECTED FOR BOFS V1** |

### 7.2 Proving Free Space Before Allocation
To eliminate the catastrophic bug discovered during the Phase 1 NTFS audit—where `ntfs_alloc_clusters` assumed hint clusters were free without verification—BOFS enforces a strict verification protocol:
1. **Allocator Lock**: Acquire `vol->alloc_lock`.
2. **Word Inspection**: The allocator reads 64-bit words (`uint64_t`) from the in-memory `$BlockBitmap` cache.
3. **Proof Verification**: A block $B$ is proven free **if and only if** `(bitmap_word & (1ULL << bit_idx)) == 0`.
4. **Buddy Scan**: If requesting $K$ contiguous blocks, scan forward to verify that bits $[B, B + K - 1]$ are all strictly `0`.
5. **Two-Phase Reservation**:
   - *Phase A (In-Memory Reservation)*: Flip bits to `1` in memory working buffer; attach allocation delta to the active journal transaction.
   - *Phase B (Storage Commit)*: After data is flushed and journal is committed, the updated bitmap block is flushed to physical storage.
6. If the volume runs out of free bits, the allocator immediately rolls back in-memory reservations, releases locks, and returns `-ENOSPC` without corrupting disk state.

---

## 8. FILE METADATA MODEL (INODE)

### 8.1 Inode Geometry & Allocation
- **Inode Size**: Fixed at **512 bytes** `[DESIGN INVARIANT]`.
- **Inodes per Block**: Exactly 8 Inodes fit into a single 4 KB block ($8 \times 512 = 4096$).
- **Total Inodes in V1**: 65,536 Inodes (occupying exactly 8,192 blocks = 32 MB).
- **Reserved Inode Indexing**:
  - `Inode 0`: Reserved / Invalid (`BOFS_NULL_INODE`).
  - `Inode 1`: Root Directory (`/`).
  - `Inode 2`: Write-Ahead Journal Object (`$Journal`).
  - `Inode 3`: Block Bitmap Object (`$BlockBitmap`).
  - `Inode 4`: Inode Bitmap Object (`$InodeBitmap`).
  - `Inode 5`: Bad Blocks Map (`$BadBlocks`).
  - `Inodes 6 - 15`: Reserved for future expansion.
  - `Inodes 16+`: Available for user/system files and directories.

### 8.2 Inode Structure Layout (512 Bytes Exact)

```c
typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BINO' (0x4F4E4942) */
    uint32_t generation;         /* Offset 0x004: Inode lifecycle counter (stale ref check) */
    uint64_t inode_num;          /* Offset 0x008: Absolute Inode number */
    uint16_t mode;               /* Offset 0x010: POSIX mode bits (file type + permissions) */
    uint16_t flags;              /* Offset 0x012: BOFS flags (immutable, system, hidden) */
    uint32_t uid;                /* Offset 0x014: Owner User ID */
    uint32_t gid;                /* Offset 0x018: Owner Group ID */
    uint32_t link_count;         /* Offset 0x01C: Hard link reference counter */
    uint64_t size_bytes;         /* Offset 0x020: Exact file size in bytes */
    uint64_t allocated_blocks;   /* Offset 0x028: Total 4KB blocks allocated */
    
    /* 64-bit High-Resolution Timestamps (Nanoseconds since UNIX Epoch) */
    uint64_t atime_sec;          /* Offset 0x030: Last access time (seconds) */
    uint32_t atime_nsec;         /* Offset 0x038: Last access time (nanoseconds) */
    uint32_t reserved_time1;     /* Offset 0x03C: Alignment padding */
    uint64_t mtime_sec;          /* Offset 0x040: Last modification time (seconds) */
    uint32_t mtime_nsec;         /* Offset 0x048: Last modification time (nanoseconds) */
    uint32_t reserved_time2;     /* Offset 0x04C: Alignment padding */
    uint64_t ctime_sec;          /* Offset 0x050: Last metadata change time (seconds) */
    uint32_t ctime_nsec;         /* Offset 0x058: Last metadata change time (nanoseconds) */
    uint32_t reserved_time3;     /* Offset 0x05C: Alignment padding */
    uint64_t crtime_sec;         /* Offset 0x060: File creation / birth time (seconds) */
    uint32_t crtime_nsec;        /* Offset 0x068: File creation time (nanoseconds) */
    uint32_t reserved_time4;     /* Offset 0x06C: Alignment padding */

    /* Extent Descriptors: 12 Direct Extents (12 * 24 bytes = 288 bytes) */
    bofs_extent_t direct_extents[12]; /* Offset 0x070 to 0x18F */

    /* Indirect Extent Pointers */
    uint64_t indirect_block;     /* Offset 0x190: 1st Tier Indirect Block Pointer */
    uint64_t double_indirect_blk;/* Offset 0x198: 2nd Tier Double Indirect Block Pointer */

    /* Reserved for Extended Attributes / Security Tokens */
    uint8_t  extended_attr[96];  /* Offset 0x1A0 to 0x1FF */

    /* Inode Integrity Checksum */
    uint32_t checksum;           /* Offset 0x1FC: CRC32 of first 508 bytes */
} __attribute__((packed)) bofs_inode_t;
```

---

## 9. FILE DATA & EXTENT MODEL

### 9.1 Extent Descriptor Structure
An extent is defined as a contiguous range of blocks:
```c
typedef struct {
    uint64_t logical_block;      /* Starting logical block offset within the file */
    uint64_t physical_block;     /* Starting physical block offset on the partition */
    uint32_t block_count;        /* Number of contiguous 4KB blocks in this run */
    uint32_t flags;              /* Extent flags: NORMAL, UNWRITTEN, SPARSE */
} __attribute__((packed)) bofs_extent_t;
```

### 9.2 Extent Addressing Capacity
- **12 Direct Extents**: If stored contiguously, 12 extents of up to $2^{32}$ blocks can represent petabyte-scale files without leaving the Inode.
- **Indirect Extent Blocks**: A single 4 KB indirect block contains $4096 / 24 = 170$ additional extent descriptors.
- **Double Indirect Extent Blocks**: Holds 512 block pointers, each pointing to an indirect block, providing $512 \times 170 = 87,040$ extents.

### 9.3 Practical File Representation Examples
- **1 KB Small File**: Occupies 1 allocated 4 KB block. `size_bytes = 1024`. `direct_extents[0] = { logical: 0, physical: 24500, count: 1 }`. Bytes 1024 to 4095 in the block are zero-padded.
- **100 KB File**: Occupies 25 contiguous blocks. `size_bytes = 102400`. `direct_extents[0] = { logical: 0, physical: 30000, count: 25 }`. Represented in a single extent descriptor.
- **10 MB Fragmented File**: Stored across 4 non-contiguous extents. Fits completely in `direct_extents[0..3]` with zero indirect block lookups.
- **Large Sparse File (100 GB with 1 MB data at 50 GB offset)**: Logical block 0 to 13,107,199 has `physical_block = 0` (`BOFS_EXTENT_SPARSE`). Logical block 13,107,200 has 256 physical blocks allocated. Consumes only 1 MB of actual storage.

---

## 10. DIRECTORY MODEL

### 10.1 Uniform Directory Records
In BOFS, directories are regular Inodes (`mode & S_IFDIR`) whose data blocks store a uniform stream of variable-length directory entries `[DESIGN INVARIANT]`:
```c
typedef struct {
    uint64_t inode_num;          /* Target Inode number */
    uint16_t rec_len;            /* Length of this directory entry record */
    uint8_t  name_len;           /* Length of the filename in bytes */
    uint8_t  file_type;          /* Cached file type for fast readdir() */
    char     name[256];          /* Null-terminated canonical UTF-8 filename */
} __attribute__((packed)) bofs_dirent_t;
```

### 10.2 Indexing Strategy: Hybrid Linear / B+Tree
- **Small Directories ($\le 16$ Entries)**: Entries are stored linearly within the directory's first 4 KB data block. Scanning 16 entries in CPU cache takes $< 2$ microseconds.
- **Large Directories ($> 16$ Entries)**: The directory converts to a **Uniform B+Tree**:
  - **Key**: 64-bit hash (`uint64_t name_hash = bofs_hash(name, name_len)`) using MurmurHash3 or SipHash-2-4.
  - **Node Geometry**: Standard 4 KB index blocks.
  - **Leaf Nodes**: Store sorted pairs of `(hash, bofs_dirent_t)`.
  - **Router Nodes**: Store pairs of `(pivot_hash, child_block_ptr)`.
  - **Split Semantics**: When an index block reaches 4096 bytes during file creation, it splits at the median key. The median pivot is promoted to the parent node, guaranteeing an $O(\log N)$ tree depth.
- **Discarding NTFS Architecture**: BOFS completely eliminates the confusing NTFS dual-tier `$INDEX_ROOT` (inside MFT) and `$INDEX_ALLOCATION` (outside MFT). All directory blocks in BOFS are standard 4 KB data blocks.

---

## 11. NAMESPACE & PATH MODEL

- **Root Designator**: `/` represents the volume root (Inode 1).
- **Path Separator**: Canonical forward slash `/`. (Backslash `\` is automatically translated to `/` at the VFS entry gateway).
- **Special Directory References**:
  - `.` maps to the directory's own Inode number.
  - `..` maps to the parent directory's Inode number.
  - In the root directory (`/`), `..` maps to Inode 1.
- **Maximum Path Length**: `4,096 bytes` (`BDE_PATH_MAX`).
- **Maximum Filename Length**: `255 bytes` (UTF-8 encoded).
- **Case Policy**: **Strict Case-Sensitive Byte Comparison** (`strcmp`).
  - *Rationale*: Eliminates the need for 256 KB unicode casing tables (e.g. NTFS `$UpCase`) in kernel memory, eliminates collation ambiguities, and provides deterministic POSIX compatibility.
- **Invalid Characters**: Only two characters are strictly forbidden in filenames:
  - `0x00` (ASCII NUL byte - string terminator).
  - `0x2F` (`/` - path separator).
  - All other UTF-8 byte sequences are valid.

---

## 12. SECURITY & ACCESS CONTROL MODEL

### 12.1 Security Principals
- **UID (User ID)**: 32-bit unsigned integer.
  - `UID = 0`: Superuser (`root` / `admin`).
  - `UID = 1000`: Primary desktop user.
  - `UID = 65534`: `guest` / `nobody`.
- **GID (Group ID)**: 32-bit unsigned integer.
  - `GID = 0`: `root` group.
  - `GID = 100`: `users` group.

### 12.2 Mode Permission Bits (Octal Representation)
Stored in `inode->mode`:
- `04000`: SetUID on execution.
- `02000`: SetGID on execution.
- `01000`: Sticky bit (only file owner or root can delete files in this directory).
- `00700`: Owner Read (`r`), Write (`w`), Execute (`x`).
- `00070`: Group Read (`r`), Write (`w`), Execute (`x`).
- `00007`: Others Read (`r`), Write (`w`), Execute (`x`).

### 12.3 Gateway Enforcement Rules
When a task requests access:
1. Fetch caller's `user_id` and `group_id` from `current_proc->security` `[OBSERVED]`.
2. If `user_id == 0` (Root), bypass all read/write checks. Execute check requires at least one `x` bit to be set anywhere.
3. If `user_id == inode->uid`, evaluate Owner bits.
4. Else if `group_id == inode->gid`, evaluate Group bits.
5. Else, evaluate Others bits.
6. If the requested operation (R/W/X) is not permitted by the evaluated bitmask, return `-EACCES` (Permission Denied).

---

## 13. TIMESTAMP MODEL

BOFS stores four independent timestamps in every Inode:
1. `crtime`: Creation Time (Birth Time) — set once when file is created; never changes.
2. `mtime`: Content Modification Time — updated whenever file data bytes are written or truncated.
3. `ctime`: Metadata Change Time — updated whenever Inode attributes (mode, UID, GID, link count) change.
4. `atime`: Access Time — updated when file data is read. (To prevent excessive write amplification on NVMe SSDs, BOFS implements `relatime` semantics: `atime` is updated only if the previous `atime` is older than `mtime` or `ctime`, or older than 24 hours).

**Resolution & Epoch**:
- 64-bit seconds + 32-bit nanoseconds.
- Coordinated Universal Time (UTC) relative to the standard UNIX Epoch (`1970-01-01 00:00:00 UTC`).

---

## 14. JOURNAL & CRASH RECOVERY ARCHITECTURE

### 14.1 Journaling Mode: Ordered Metadata Logging
BOFS standardizes on **Ordered Metadata Logging** (`data=ordered`):
1. User file data blocks are written to physical storage.
2. An explicit cache flush (`block_device_flush()`) is issued to the NVMe controller.
3. Metadata changes (Inodes, Bitmaps, Directory blocks) are recorded in the in-memory transaction buffer.
4. The transaction is serialized to the circular **Journal Ring Buffer** on disk.
5. A Commit Record is written and flushed to the journal.
6. In-place filesystem metadata blocks are updated on disk.

### 14.2 Circular Journal Ring Geometry
- Occupies a dedicated fixed contiguous region: **Blocks 4 to 8,195 (32 Megabytes)**.
- Maintained via two pointers in the Superblock: `journal_head` and `journal_tail`.
- Every transaction has a monotonically increasing 64-bit sequence number (`uint64_t txn_seq`).

### 14.3 Crash Recovery Protocol (Mount-Time Replay)
When `bofs_mount()` detects `BOFS_STATE_DIRTY`:
1. The driver scans the Journal Ring from `journal_tail` to `journal_head`.
2. For each transaction block, it computes the CRC32 checksum. If checksum matches and a valid Commit Record exists, the transaction is **committed**.
3. Committed transactions are replayed sequentially, copying updated metadata blocks into their primary partition locations.
4. Any partially written transaction lacking a valid Commit Record or failing CRC32 is **discarded** (atomic rollback).
5. The Superblock state is marked `BOFS_STATE_CLEAN`, flushed, and mount proceeds with zero lost structures.

---

## 15. INTEGRITY & CORRUPTION DETECTION

| Filesystem Structure | Checksum Algorithm | Scope of Protection | Kernel Action on Mismatch |
| :--- | :--- | :--- | :--- |
| **Superblock (Block 0)** | IEEE 802.3 CRC32 | Bytes 0 to 4,091 of Block 0 | Attempt failover to Backup Superblock (Block 1). If both fail, refuse mount (`-EIO`). |
| **Inode (512 Bytes)** | IEEE 802.3 CRC32 | First 508 bytes of Inode record | Quarantine Inode; refuse access; return `-EIO`. Do not overwrite corrupt Inode. |
| **Directory Block (4 KB)** | IEEE 802.3 CRC32 | Bytes 0 to 4,091 of Directory Block | Halt path traversal; return `-EIO`; remount volume read-only. |
| **Journal Descriptor (4 KB)**| IEEE 802.3 CRC32 | Transaction Header + Block Vectors | Invalidate transaction during recovery; discard uncommitted data. |

---

## 16. VFS INTEGRATION & ADAPTER LAYER

### 16.1 Mapping to Current ATOMS VFS

```text
[Syscall Layer: sys_open / sys_read / sys_write]
       │
       ▼
[kernel/vfs/vfs_legacy/src/vfs.c]
       │
       ▼ (Dispatches via FilesystemDriver callback table)
[kernel/vfs/bofs/bofs_vfs.c] -> BOFS VFS Adapter
       │
       ├─► bofs_mount()
       ├─► bofs_open()
       ├─► bofs_read()
       ├─► bofs_write()
       ├─► bofs_close()
       ├─► bofs_readdir()
       ├─► bofs_mkdir()
       ├─► bofs_create()
       ├─► bofs_rename()
       └─► bofs_delete()
```

### 16.2 API Gap Analysis

| VFS Operation | Current ATOMS VFS Prototype | BOFS V1 Implementation Requirement | Target Phase |
| :--- | :--- | :--- | :--- |
| `mount` | `VFS_Node* (*mount)(BlockDevice* dev)` | Parse Superblock, replay journal, allocate volume context, return root `VFS_Node`. | Phase 9 |
| `read` | `int (*read)(VFS_Node*, uint64_t, uint32_t, void*)` | Translate offset to extent, read 4KB blocks via NVMe, handle partial edges. | Phase 5 & 9 |
| `write` | `int (*write)(VFS_Node*, uint64_t, uint32_t, void*)` | Allocate blocks via bitmap, write data, log Inode transaction, update size. | Phase 5 & 9 |
| `readdir` | `int (*readdir)(VFS_Node*, const char*, int, vfs_dirent_t*)` | Enumerate directory B+Tree entries, populate name, size, type, inode. | Phase 6 & 9 |
| `stat` | **MISSING IN CURRENT VFS** | Must return 64-bit size, UID, GID, mode, 4 timestamps, allocated blocks. | Phase 9 |
| `chmod` | **MISSING IN CURRENT VFS** | Update Inode permission mode bits in journal transaction. | Phase 7 & 9 |
| `chown` | **MISSING IN CURRENT VFS** | Update Inode UID and GID in journal transaction. | Phase 7 & 9 |

---

## 17. SYSCALL & USERSPACE INTERFACE SPECIFICATION

To expose native BOFS capabilities to userspace applications, the ATOMS syscall layer (`kernel/core/syscall/`) will be expanded in Phase 9 with standard POSIX-compatible signatures:

```c
/* Extended File Syscalls (Phase 9 Integration) */
int64_t sys_stat(const char *path, bofs_stat_t *buf);
int64_t sys_fstat(int fd, bofs_stat_t *buf);
int64_t sys_readdir(int fd, bofs_dirent_t *dirp, uint32_t count);
int64_t sys_mkdir(const char *path, uint32_t mode);
int64_t sys_create(const char *path, uint32_t mode);
int64_t sys_rename(const char *oldpath, const char *newpath);
int64_t sys_unlink(const char *path);
int64_t sys_rmdir(const char *path);
int64_t sys_chmod(const char *path, uint32_t mode);
int64_t sys_chown(const char *path, uint32_t uid, uint32_t gid);
int64_t sys_sync(void);
int64_t sys_fsync(int fd);
```

**Pointer Safety Requirement**: Every user-supplied buffer or string pointer MUST pass `syscall_validate_user_string()` and `syscall_validate_user_ptr()` before the syscall gateway touches memory `[OBSERVED]`.

---

## 18. FILE MANAGER & EXPLORER CONTRACT

In ATOMS OS, **Explorer is a Pure View Layer** (`kernel/shell/apps/explorer.h`) and possesses **zero filesystem logic** `[OBSERVED]`:
- Explorer never issues raw block I/O.
- Explorer never inspects on-disk filesystem structures.
- Explorer talks strictly to the **BSOM (BOS Structured Object Model)** layer via `BSOM_OpenObject()` and `BSOM_GetChildren()` `[OBSERVED]`.

### The Contract
1. **Dynamic Mount Discovery**: Explorer queries VFS for active mount points (`/`, `/DATA`, `/MEDIA`). Hardcoded Windows drive letters (`C:`, `D:`, `E:`) and static folders are completely forbidden.
2. **Metadata Presentation**: BOFS supplies standard directory enumeration records containing filename, 64-bit file size, POSIX mode bits, and modification timestamps.
3. **Icon Resolution**: BSOM resolves icons based on file extensions (`.BOSX` $\rightarrow$ executable icon, `.PNG` $\rightarrow$ image icon, directory $\rightarrow$ folder icon) without filesystem interference.

---

## 19. BOSX RUNTIME & PROCESS LOADER BOUNDARY

A sharp boundary is enforced between file storage and process execution:
- **BOFS Responsibility**: Store raw file bytes across contiguous 4 KB block extents; maintain file size and POSIX permission mode bits (`0755`); enforce access control.
- **BOSX Loader Responsibility**: Read file bytes; validate `BOSX_Header.magic == 0x58534F42`; parse section tables; allocate virtual memory pages; map code/data sections; configure Ring 3 stack; jump to `entry_point`.
- **The Execution Gate**: The loader checks `inode->mode & S_IXUSR`. If the file lacks executable permission, the loader immediately aborts with `-EACCES` before allocating any virtual address space.

---

## 20. NATIVE VS. FOREIGN FILESYSTEM MODEL

ATOMS OS distinguishes between storage roles:
- **Native Filesystem (BOFS)**:
  - Read/Write enabled.
  - Primary OS boot partition, user homes, system binaries, application data.
  - Formatted with unique GPT partition GUID: `GUID_BOFS_SYSTEM`.
- **Foreign / Compatibility Filesystems**:
  - **NTFS**: Strict Read-Only Forensic Probe `[REAL HARDWARE PROVEN]`. Used exclusively for Windows diagnostic data acquisition.
  - **FAT32**: Read/Write enabled for EFI System Partitions (ESP) and USB flash drives.

---

## 21. ERROR CODE TAXONOMY

BOFS standardizes on POSIX-compliant negative integer error codes:

| Error Constant | Value | Semantic Meaning |
| :--- | :--- | :--- |
| `BOFS_SUCCESS` | `0` | Operation completed successfully. |
| `BOFS_ERR_EPERM` | `-1` | Operation not permitted. |
| `BOFS_ERR_ENOENT`| `-2` | No such file or directory. |
| `BOFS_ERR_EIO` | `-5` | Physical hardware I/O or CRC checksum error. |
| `BOFS_ERR_EBADF` | `-9` | Bad file descriptor. |
| `BOFS_ERR_ENOMEM`| `-12`| Kernel out of memory (heap budget exceeded). |
| `BOFS_ERR_EACCES`| `-13`| Permission denied (DAC failure). |
| `BOFS_ERR_EEXIST`| `-17`| File or directory already exists. |
| `BOFS_ERR_ENOTDIR`| `-20`| Component of path is not a directory. |
| `BOFS_ERR_EISDIR`| `-21`| Is a directory (cannot perform file write on dir). |
| `BOFS_ERR_EINVAL`| `-22`| Invalid argument or corrupted metadata block. |
| `BOFS_ERR_ENOSPC`| `-28`| No space left on device (bitmaps exhausted). |
| `BOFS_ERR_EROFS` | `-30`| Read-only filesystem. |

---

## 22. EXPLICIT V1 ARCHITECTURAL LIMITS

| Metric | Architectural Limit | Rationale / Derivation |
| :--- | :--- | :--- |
| **Max Volume Size** | **16 Exabytes** | Native 64-bit block pointers ($2^{64} \times 4096$). |
| **Max File Size** | **16 Exabytes** | Native 64-bit `uint64_t size_bytes` in Inode. |
| **Max Inodes per Volume** | **65,536 Inodes** | Fixed 32 MB Inode table budget in V1. |
| **Max Directory Entries** | **1,048,576 files/dir** | B+Tree depth $\le 4$ with 4096-byte blocks. |
| **Max Filename Length** | **255 Bytes** | Fits comfortably in 512-byte Inode and directory structures. |
| **Max Absolute Path Length**| **4,096 Bytes** | Matches ATOMS `BDE_PATH_MAX` and POSIX standard. |
| **Max Extents per File** | **87,052 Extents** | 12 direct + 170 indirect + 86,870 double-indirect. |
| **Max Open Files** | **32 System-Wide** | Bound by current VFS `g_fd_table[32]` table `[OBSERVED]`. |
| **Max Journal Size** | **32 Megabytes** | 8,192 4KB blocks; holds up to 2,000 active transactions. |
| **Max Nested Directory Depth**| **32 Levels** | Prevents kernel stack overflow during recursive resolution. |

---

## 23. SECURITY & CORRUPTION THREAT MODEL

1. **Malicious Userspace Payloads**:
   - *Threat*: Ring 3 process passes bad pointer or malformed path string to trigger kernel panic.
   - *Mitigation*: Syscall gateway enforces `syscall_validate_user_string()` and `syscall_validate_user_ptr()` before invoking any BOFS function `[OBSERVED]`.
2. **Privilege Escalation**:
   - *Threat*: Unprivileged user attempts to overwrite root-owned system binary `/SYSTEM/SHELL.BOSX`.
   - *Mitigation*: DAC engine compares caller's `security.user_id` against Inode `uid` and `mode`. Write request is denied with `-EACCES`.
3. **Abrupt Power Loss / Hard Reset**:
   - *Threat*: Machine loses power while updating directory records and allocating blocks.
   - *Mitigation*: Ordered metadata journaling. Disk cache is flushed before journal commit; uncommitted transactions roll back on next boot; committed transactions replay cleanly.
4. **Physical Flash Bit-Flips (Silent Corruption)**:
   - *Threat*: Storage sector degrades over time, returning bad metadata.
   - *Mitigation*: IEEE 802.3 CRC32 checksums on every metadata block. Bad blocks fail verification and trigger read-only fallback before corrupt data touches memory.

---

## 24. PROGRESSIVE PHASE IMPLEMENTATION BOUNDARIES (PHASES 3–13)

```text
PHASE 3: BOFS On-Disk Format Binary Specification
- Input: Phase 2 Architecture Blueprint.
- Output: Exact C struct headers (`bofs_format.h`), byte offsets, endianness rules, pack pragma.
- Must NOT Touch: Kernel source, VFS, drivers.

PHASE 4: BOFS Block & Inode Allocator Engine
- Input: `bofs_format.h`.
- Output: `bofs_alloc.c` implementing bitmap scanning, buddy search, and free-space proof.
- Pass Criteria: Zero bit collisions across 10,000 synthetic allocations.

PHASE 5: BOFS Inode Management & File Extent Engine
- Input: Allocator engine.
- Output: `bofs_inode.c` reading/writing 512B Inodes, decoding extents, handling sparse files.
- Pass Criteria: Reading and writing 10 MB fragmented file with 100% byte verification.

PHASE 6: BOFS Uniform B+Tree Directory Engine
- Input: Inode and extent engine.
- Output: `bofs_dir.c` path lookup, dirent insertion, median-key node splitting, `.` and `..`.
- Pass Criteria: Creating 1,000 files in a single directory with deterministic $O(\log N)$ lookup.

PHASE 7: BOFS Security & Permission Engine
- Input: Directory and Inode engines.
- Output: `bofs_security.c` DAC gateway evaluating UID, GID, mode bits, and SetUID semantics.
- Pass Criteria: Unprivileged user blocked from reading 0600 root files.

PHASE 8: BOFS Write-Ahead Journaling & Reliability Engine
- Input: Metadata engines.
- Output: `bofs_journal.c` circular ring manager, commit blocks, mount-time replay engine.
- Pass Criteria: Simulated crash during file creation recovers 100% cleanly on reboot.

PHASE 9: VFS Integration & POSIX Syscall Gateway
- Input: Full BOFS core engine.
- Output: `bofs_vfs.c` implementing `FilesystemDriver`, registering into ATOMS VFS.
- Pass Criteria: Standard `vfs_open`, `vfs_read`, `vfs_write`, `vfs_close` operational on BOFS.

PHASE 10: BOSX Executable Loader Integration
- Input: VFS integration.
- Output: Process manager loading native `.BOSX` binaries directly from BOFS partitions.
- Pass Criteria: Executing `SHELL.BOSX` from a BOFS volume into Ring 3 userspace.

PHASE 11: BOFS Userspace File Manager & Utilities
- Input: Syscall gateway.
- Output: `mkfs.bofs` formatter, `fsck.bofs` consistency checker, Desktop Explorer integration.
- Pass Criteria: Formatting a blank partition and browsing files visually in Explorer.

PHASE 12: Interactive Visual Forensic Telemetry Dashboard
- Input: Live BOFS filesystem.
- Output: Real-time 1080p GOP diagnostic screen showing volume metrics, bitmaps, and transactions.
- Pass Criteria: Visual rendering of active transactions and allocation map.

PHASE 13: Real-Hardware Certification
- Input: Certified build.
- Output: Formal hardware pass certificate on ASUS PRIME B750M-K with WD Blue SN5000 NVMe SSD.
- Pass Criteria: 10,000 write-verify-delete stress cycles without a single CRC error or panic.
```

---

## 25. OPEN DESIGN QUESTIONS

1. **Backup Superblock Placement**: Should the second backup superblock reside at the exact final block of the partition (traditional Unix style) or at Block 1 (fast-seek style)?
   * *Tentative Resolution*: Store backup at Block 1 AND at the final block for dual redundancy.
2. **Inline Small File Data**: Should files smaller than 256 bytes be stored directly inside the Inode's `direct_extents` union to save a 4 KB data block?
   * *Tentative Resolution*: Defer inline data to BOFS V2 to preserve strict separation between metadata and data in V1.
3. **Dynamic Inode Table Growth**: Should Inode capacity expand dynamically in 4 KB chunks?
   * *Tentative Resolution*: No. Keep Inode table fixed at 65,536 Inodes (32 MB) in V1 for deterministic simplicity.

---

## 26. FINAL ARCHITECTURE DECISION RECORD (ADR)

- **ADR-001: Storage Quantum**: Standardize on a fixed **4,096-byte logical block**. *Rejected*: 512-byte blocks (excessive metadata overhead) and variable block sizes (complex fragmentation).
- **ADR-002: Inode Geometry**: Fixed **512-byte Inodes**. *Rejected*: 1024-byte MFT-style records (wasteful memory footprint) and 256-byte Inodes (insufficient inline extent capacity).
- **ADR-003: Allocation Mechanism**: **Contiguous Free-Space Bitmaps**. *Rejected*: Linked free-lists (non-contiguous, torn-write sensitive).
- **ADR-004: Directory Indexing**: **Uniform B+Tree with 64-bit Hash Keys**. *Rejected*: NTFS dual $INDEX_ROOT / $INDEX_ALLOCATION (split-state corruption).
- **ADR-005: Integrity**: **IEEE 802.3 CRC32 on every metadata block**. *Rejected*: Sector fixup bytes (archaic 1990s hack).
- **ADR-006: Crash Consistency**: **Metadata-Only Ordered Write-Ahead Journaling**. *Rejected*: Full data journaling (doubles NVMe write amplification).
- **ADR-007: Security**: **Native POSIX 32-bit UID/GID + 12-bit Mode Bits**. *Rejected*: Windows Security Descriptors (overly complex, foreign to ATOMS).

---

### FINAL ARCHITECTURAL STATUS:
**PHASE 2 REQUIREMENTS & ARCHITECTURAL SPECIFICATION FORMALLY COMPLETE.**  
**ZERO PRODUCTION SOURCE CODE MODIFIED.**  
**READY FOR HUMAN / CTO REVIEW PRIOR TO PHASE 3.**
