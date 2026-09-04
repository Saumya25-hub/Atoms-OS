# ATOMS OS — PHASE 4: BOFS V1 BLOCK & FREE-SPACE ALLOCATION ENGINE SPECIFICATION

**Document ID:** `ATOMS-BOFS-PHASE4-SPEC-001`  
**Classification:** CORE FILESYSTEM ENGINE SPECIFICATION & CERTIFICATION  
**Author:** ATOMS OS Core Engineering & Filesystem Architecture Team  
**Date:** 2026-09-04  
**Hardware Baseline:** ASUS PRIME B750M-K (Intel Core i3-14100F, Haswell/Raptor Lake x86_64, WD Blue SN5000 500GB NVMe SSD, 8GB RAM)  
**Baseline Git Checkpoint:** Commit `51e6e8c` (`feat(bofs): complete Phase 3 on-disk format specification and validator`)  
**Rule 0 Compliance:** Phase strictly isolated to Block Allocation Engine. Zero writes to production NVMe/NTFS partitions.

---

## 1. DOCUMENT METADATA

| Property | Value |
| :--- | :--- |
| **Document ID** | `ATOMS-BOFS-PHASE4-SPEC-001` |
| **Version** | `1.0.0` (Production Milestone Specification) |
| **Status** | `CERTIFIED PASS` |
| **Phase** | `Phase 4 — BOFS Block / Allocation Engine` |
| **Parent Phase** | `Phase 3 — BOFS On-Disk Format` (Commit `51e6e8c`) |
| **Next Phase** | `Phase 5 — BOFS Metadata + File Engine` |
| **Toolchain Target** | `x86_64-pc-none-elf` freestanding (Clang 17+, `-mno-red-zone -mno-sse`) |
| **Verification Gate** | Dual Host (Python 3.14) & Target In-Kernel ABDE / COM1 |

---

## 2. SCOPE AND BOUNDARIES

### In Scope (Implemented & Certified in Phase 4)
- Deterministic 4,096-byte block allocation and deallocation.
- Contiguous multi-block allocation for sequential file data optimization.
- Fragmented multi-block allocation producing structured extent lists (`bofs_alloc_extent_list_t`).
- In-memory reservation lifecycle (`reserve != commit`) with rollback capability.
- In-memory reservation collision avoidance against normal allocation requests.
- Strict double-free detection returning `BOFS_ERR_DOUBLE_FREE` (-16).
- Double-allocation prevention via atomic bit checking in the block bitmap.
- Complete out-of-space (ENOSPC) detection and bounded error returns.
- Free-space tracking and authoritative full-bitmap verification.
- Bitmap persistence and dirty cache block writeback through the `BlockDevice` interface.
- Detection and rejection of bitmap corruption (metadata blocks marked free).
- Safe isolation against production hardware: zero touch to foreign NVMe/NTFS/boot partitions.

### Out of Scope (Deferred to Future Phases)
- **Phase 5**: Inode allocation, direct/indirect extents, file payloads, sparse hole punch.
- **Phase 6**: B+Tree directory structures, fast directory lookups, file creation/deletion.
- **Phase 7**: POSIX permissions (`rwxrwxrwx`), UID/GID, ACL security descriptors.
- **Phase 8**: Write-Ahead Journal replay, checksum recovery, crash resilience.
- **Phase 9**: Virtual File System (VFS) mount table, standard POSIX syscall layer (`read`/`write`/`open`).
- **Phase 10**: BOSX binary executable loader integration.
- **Phase 11**: Desktop GUI File Manager integration.

---

## 3. ALLOCATION ARCHITECTURE

```
+---------------------------------------------------------------------------------------+
|                                  BOFS ALLOCATOR CONTEXT                                |
|                                                                                       |
|  [BlockDevice] <---> [4KB Bitmap Cache] <---> [Linear Scan Cursor] <---> [Reservations]|
|   (512e / 4Kn)        (Dirty Writeback)       (Next-Fit Locality)        (32 Handles) |
+---------------------------------------------------------------------------------------+
                                           |
                   +-----------------------+-----------------------+
                   |                                               |
                   v                                               v
     [Single & Contiguous Allocation]               [Fragmented Multi-Extent]
     - Contiguous runs for streaming I/O             - Merges adjacent blocks
     - Next-fit sequential cursor                    - Up to 16 extents per call
```

### Block-to-Sector Translation
The BOFS allocation engine operates exclusively in units of 4,096-byte logical blocks. Storage hardware interfaces expose sector-based LBAs. The translation layer deterministically resolves block addresses to storage LBAs:

$$\text{Sectors Per Block (SPB)} = \frac{\text{BOFS\_BLOCK\_SIZE}}{\text{dev->sector\_size}} = \frac{4096}{512} = 8$$

$$\text{Start LBA} = \text{Block Index} \times \text{SPB}$$

$$\text{Sector Count} = \text{SPB}$$

Overflow protection is enforced prior to multiplication: `block_idx > (UINT64_MAX / spb)`.

### Bitmap Layout & Addressing
- **Unit of Granularity**: 1 bit per 4,096-byte block (0 = Free, 1 = Allocated).
- **Block Density**: Each 4,096-byte bitmap block tracks $4096 \times 8 = 32,768 \text{ blocks}$ ($128 \text{ MB}$ of storage).
- **Structural Mapping**:
  $$\text{Bitmap Block Offset} = \left\lfloor \frac{\text{Block Index}}{32768} \right\rfloor$$
  $$\text{Disk Bitmap Block} = \text{sb.block\_bitmap\_start\_block} + \text{Bitmap Block Offset}$$
  $$\text{Local Bit Index} = \text{Block Index} \pmod{32768}$$
  $$\text{Byte Offset in Cache} = \left\lfloor \frac{\text{Local Bit Index}}{8} \right\rfloor$$
  $$\text{Bit Mask} = 1 \ll (\text{Local Bit Index} \pmod 8)$$

### Caching & Writeback Strategy
The allocator maintains an aligned 4,096-byte active cache buffer (`cached_bitmap`). 
1. **Eviction-Triggered Flush**: When a requested block falls outside the currently cached 32,768-block window, any pending dirty modifications are flushed to disk via `alloc->dev->write()`.
2. **Explicit Flush**: Calls to `bofs_allocator_flush` ensure dirty blocks and hardware controller write caches (`alloc->dev->flush()`) are serialized.

---

## 4. ALLOCATION ALGORITHMS

### 4.1 Single Block Allocation (`bofs_alloc_block`)
Single block allocation employs a **Next-Fit** sequential cursor (`last_alloc_cursor`) to maximize contiguous locality on disk:
1. Validates that `free_blocks_count > 0`.
2. Scans from `last_alloc_cursor` to `data_pool_end`.
3. If no candidate is found before end-of-pool, wraps around and scans from `data_pool_start` to `last_alloc_cursor`.
4. Checks each candidate against active in-memory reservations via `bofs_is_block_reserved()`.
5. Upon finding a 0-bit:
   - Sets the bit to 1.
   - Marks cache dirty.
   - Decrements `free_blocks_count`.
   - Advances `last_alloc_cursor` to `candidate + 1`.
   - Flushes cache to disk.
   - Returns allocated block index.

### 4.2 Contiguous Multi-Block Allocation (`bofs_alloc_blocks_contiguous`)
Allocates $N$ strictly adjacent 4KB blocks:
1. Validates `count > 0` and `free_blocks_count >= count`.
2. Scans sequentially across the data pool.
3. Skips reserved blocks and already-allocated blocks, resetting the run length counter.
4. When a continuous sequence of $N$ free, unreserved blocks is located:
   - Sets all $N$ bits to 1 in the bitmap across one or more bitmap blocks.
   - Updates `free_blocks_count -= count`.
   - Flushes dirty cache blocks.
   - Returns starting block index.
5. If no single contiguous run satisfies $N$, returns `BOFS_ERR_OUT_OF_SPACE` without modifying disk.

### 4.3 Fragmented Allocation (`bofs_alloc_blocks_fragmented`)
When contiguous allocation cannot be satisfied due to volume aging or fragmentation, fragmented allocation fulfills the request by gathering blocks into a coalesced extent list:
1. Accepts request for $N$ blocks and a pointer to `bofs_alloc_extent_list_t`.
2. Allocates blocks one by one using the sequential scanner.
3. Automatically coalesces adjacent blocks into existing extent entries:
   $$\text{prev->start\_block} + \text{prev->count} == \text{new\_block} \implies \text{prev->count}++$$
4. Opens a new extent only when a non-adjacent block is allocated.
5. Caps extent count at `BOFS_MAX_ALLOC_EXTENTS` (16).
6. Atomically rolls back and frees all allocated blocks if `BOFS_MAX_ALLOC_EXTENTS` is exceeded or disk runs out of space.

---

## 5. RESERVATION SYSTEM (`reserve != commit`)

### Architectural Rationale
In modern transactional filesystems, pre-allocating disk space before payload data is prepared in RAM must not pollute on-disk bitmaps or expose partial allocations upon premature crash or process fault.

```
State Progression:
[FREE] ---> bofs_alloc_reserve_blocks() ---> [RESERVED (IN-RAM ONLY)]
                                                      |
                         +----------------------------+----------------------------+
                         |                                                         |
                         v                                                         v
          bofs_alloc_commit_reservation()                           bofs_alloc_rollback_reservation()
                         |                                                         |
                         v                                                         v
                 [COMMITTED (ON-DISK)]                                      [FREE (ZERO DISK I/O)]
```

### In-Memory Reservation Handle
```c
typedef struct {
    uint32_t id;
    bool     active;
    uint64_t start_block;
    uint32_t count;
} bofs_reservation_t;
```

### Reservation Invariants
1. **Zero Disk I/O on Reserve**: Reserving a range verifies that the bits are currently 0 and claims an in-memory slot in `alloc->reservations`. No disk writes are performed.
2. **Collision Shielding**: Normal allocation routines (`bofs_alloc_block`, contiguous, fragmented) consult `bofs_is_block_reserved()` and skip any blocks reserved by active handles.
3. **Commit Phase**: Marks the underlying bitmap bits to 1, decrements `free_blocks_count`, flushes cache to disk, and deactivates the handle.
4. **Rollback Phase**: Deactivates the handle in memory. Incurs zero disk I/O, leaving the on-disk bitmap entirely unmodified.

---

## 6. FREE-SPACE MANAGEMENT

### Freeing Protocol (`bofs_free_block`)
1. Rejects attempts to free blocks located in structural metadata regions (`block_num < data_pool_start`), returning `BOFS_ERR_METADATA_PROTECTED`.
2. Rejects blocks exceeding volume boundary (`block_num > data_pool_end`), returning `BOFS_ERR_OUT_OF_BOUNDS`.
3. Loads the corresponding bitmap block.
4. **Double-Free Guard**: Inspects target bit. If bit is already 0 (free), rejects operation immediately and returns `BOFS_ERR_DOUBLE_FREE` (-16).
5. Clears the bit to 0.
6. Marks cache dirty, flushes to disk, and increments `free_blocks_count`.

### Authoritative Full-Bitmap Scanning (`bofs_count_free_blocks`)
Provides a non-destructive, ground-truth audit of all data pool bits:
- Iterates from `data_pool_start` to `data_pool_end`.
- Counts every 0 bit.
- Used to verify allocator baseline integrity and prove zero bitmap drift across stress cycles.

---

## 7. SAFETY, INTEGRITY, AND FAULT TOLERANCE

### Error Code Reference Table

| Macro | Value | Description |
| :--- | :---: | :--- |
| `BOFS_ALLOC_OK` | `0` | Operation succeeded cleanly |
| `BOFS_ERR_METADATA_PROTECTED` | `-1` | Operation denied: target block resides in immutable metadata region |
| `BOFS_ERR_RESERVATION_NOT_FOUND`| `-2` | Specified reservation ID is invalid or already released |
| `BOFS_ERR_IO` | `-5` | Physical BlockDevice read/write/flush failure |
| `BOFS_ERR_OUT_OF_BOUNDS` | `-8` | Target block exceeds volume partition boundary |
| `BOFS_ERR_RESERVATION_FULL` | `-12` | Reservation table capacity exhausted (max 32 active handles) |
| `BOFS_ERR_DOUBLE_FREE` | `-16` | Double-free violation: target block is already marked unallocated |
| `BOFS_ERR_DOUBLE_ALLOC` | `-17` | Double-allocation violation: target bit already set in bitmap |
| `BOFS_ERR_INVALID_PARAM` | `-23` | NULL context, zero block count, or invalid device parameters |
| `BOFS_ERR_OUT_OF_SPACE` | `-28` | Insufficient free space to satisfy request (POSIX `ENOSPC`) |
| `BOFS_ERR_CORRUPT_BITMAP` | `-117`| Bitmap metadata inconsistency detected |

### Bitmap Validation Engine (`bofs_validate_bitmap`)
Scans all metadata blocks prior to `data_pool_start`. If any metadata block (Superblock, Backup SB, Journal, Bitmaps, Inode Table) has a bit value of 0 in the block bitmap, the validator halts and returns `BOFS_ERR_CORRUPT_BITMAP` (-117).

---

## 8. BLOCKDEVICE INTERFACE INTEGRATION

The allocator integrates with the ATOMS OS hardware-agnostic `BlockDevice` layer (`kernel/vfs/vfs_legacy/storage/include/block_device.h`).

```c
typedef struct BlockDevice {
    int id;
    const char* name;
    uint64_t sector_size;
    uint64_t sector_count;
    bool read_only;
    void* driver_data;
    bool (*read)(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer);
    bool (*write)(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer);
    bool (*flush)(struct BlockDevice* dev);
} BlockDevice;
```

### Safety Guarantees
1. **Device-Neutral Sector Size**: Compatible with both 512-byte emulation (`512e`) and 4,096-byte native (`4Kn`) sectors.
2. **Strict Read-Only Enforcement**: If `dev->read_only` is true, write operations fail immediately.
3. **No Direct Hardware Access**: The allocator never issues raw NVMe queue commands or AHCI FIS packets directly, preserving microkernel modularity.

---

## 9. C API SPECIFICATION

The public API is declared in [`kernel/vfs/bofs/include/bofs_alloc.h`](file:///D:/Signatures_OS/kernel/vfs/bofs/include/bofs_alloc.h):

```c
int bofs_allocator_init(bofs_allocator_t* alloc, BlockDevice* dev);
int bofs_allocator_flush(bofs_allocator_t* alloc);
int bofs_alloc_block(bofs_allocator_t* alloc, uint64_t* out_block);
int bofs_alloc_blocks_contiguous(bofs_allocator_t* alloc, uint32_t count, uint64_t* out_start_block);
int bofs_alloc_blocks_fragmented(bofs_allocator_t* alloc, uint32_t count, bofs_alloc_extent_list_t* out_list);
int bofs_alloc_reserve_blocks(bofs_allocator_t* alloc, uint32_t count, uint32_t* out_res_id, uint64_t* out_start_block);
int bofs_alloc_commit_reservation(bofs_allocator_t* alloc, uint32_t res_id);
int bofs_alloc_rollback_reservation(bofs_allocator_t* alloc, uint32_t res_id);
int bofs_free_block(bofs_allocator_t* alloc, uint64_t block_num);
int bofs_free_blocks(bofs_allocator_t* alloc, uint64_t start_block, uint32_t count);
uint64_t bofs_count_free_blocks(bofs_allocator_t* alloc);
int bofs_validate_bitmap(bofs_allocator_t* alloc);
```

---

## 10. TEST ARCHITECTURE

```
+---------------------------------------------------------------------------------------+
|                                    TEST VERIFICATION                                  |
|                                                                                       |
|  [Host Test Suite]                              [In-Kernel Diagnostic Engine]         |
|  tools/bofs/test_bofs_alloc.py                  kernel/debug/bofs_allocation_test.c   |
|  - 20 Formal Tests (T01 - T20)                  - 16 In-Kernel Tests                  |
|  - 10,000-cycle stress test                     - 1,000 in-kernel stress cycles       |
|  - Zero drift validation                        - ABDE GOP Display & Spinner          |
|  - Status: 20/20 PASS                           - Status: 16/16 PASS                  |
+---------------------------------------------------------------------------------------+
```

---

## 11. COMPLETE TEST MATRIX (T01 - T20)

All 20 tests were executed against the test volume geometry (17,500 blocks, 70 MB capacity, `data_pool_start = 16,391`, `initial_free = 1,109`).

| ID | Test Name | Description / Verification Method | Expected Result | Actual Result | Verdict |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **T01** | Initial Bitmap State | Initialize allocator context; verify initial free blocks count and data pool start | `data_pool_start == 16391`, `free == 1109` | `data_pool_start = 16391`, `free = 1109` | **PASS** |
| **T02** | Single Block Allocation | Allocate single 4KB block via `bofs_alloc_block` | Block 16391 allocated; free count = 1108 | Block 16391 allocated; free count = 1108 | **PASS** |
| **T03** | Single Block Free | Free allocated block via `bofs_free_block` | Block bit cleared; free count restored to 1109 | Block bit cleared; free count = 1109 | **PASS** |
| **T04** | Contiguous Allocation | Request 50 contiguous blocks via `bofs_alloc_blocks_contiguous` | `start == 16391`, `free == 1059` | `start = 16391`, `free = 1059` | **PASS** |
| **T05** | Contiguous Free | Free 50 contiguous blocks via `bofs_free_blocks` | All 50 bits cleared; free count = 1109 | 50 bits cleared; free count = 1109 | **PASS** |
| **T06** | Fragmented Allocation | Create checkerboard hole; request fragmented allocation | Total blocks = 3 across multiple extents | 3 blocks across 2 extents; clean free | **PASS** |
| **T07** | Reservation Acquisition | Reserve 10 contiguous blocks (`reserve != commit`) | Reservation handle granted; free count untouched | Handle ID = 1; free count unchanged (1109) | **PASS** |
| **T08** | Collision Avoidance | Attempt standard allocation while reservation is active | Normal allocation skips reserved block range | Range [16391..16400] skipped; allocated 16401 | **PASS** |
| **T09** | Reservation Rollback | Rollback active reservation; verify blocks are released | Handle deactivated; blocks reallocatable | Rollback OK; block 16391 re-allocated | **PASS** |
| **T10** | Double Allocation Guard | Confirm sequential allocator never allocates already set bit | Scanner skips allocated bit | Bit verified set; duplicate allocation blocked | **PASS** |
| **T11** | Double Free Protection | Attempt to free an already-freed block | Call rejected with error code | `BOFS_ERR_DOUBLE_FREE` (-16) returned | **PASS** |
| **T12** | Out-of-Space (ENOSPC) | Request more blocks than total volume free space | Call rejected with ENOSPC | `BOFS_ERR_OUT_OF_SPACE` (-28) returned | **PASS** |
| **T13** | Bitmap Persistence | Allocate block, flush allocator, verify physical sectors | Target bit physically set on mock device | LBA verified with bit set on storage | **PASS** |
| **T14** | Reopen Consistency | Re-initialize new allocator context from persisted device | Free count matches active allocator count | Second allocator reads identical state | **PASS** |
| **T15** | Bitmap Corruption Guard | Zero out metadata bit in block bitmap; validate | Validator flags corrupted metadata block bit | Malformed metadata bit detected | **PASS** |
| **T16** | Geometry Boundary Guard | Verify data pool boundaries against Superblock regions | `data_pool_start > sb.inode_table_start` | Non-overlapping boundary enforced | **PASS** |
| **T17** | Metadata Protection | Attempt to free Block 0 (Superblock) | Call rejected with permission error | `BOFS_ERR_METADATA_PROTECTED` (-1) returned | **PASS** |
| **T18** | Invalid Parameter Guard | Call allocation API with count = 0 or NULL pointer | Call rejected with parameter error | `BOFS_ERR_INVALID_PARAM` (-23) returned | **PASS** |
| **T19** | Repeated Stress Test | Run 10,000 continuous alloc/free cycles | 10,000 cycles complete without error | 10,000 cycles completed cleanly | **PASS** |
| **T20** | Baseline Restoration | Run authoritative scan after all tests & stress | Final free count matches initial baseline exactly | `final_free == 1109` (Bitmap drift: 0) | **PASS** |

---

## 12. STRESS TESTING AND PERFORMANCE RESULTS

### 10,000-Cycle Allocation/Free Stress Test
- **Total Cycles**: 10,000 sequential single-block allocation and deallocation operations.
- **Progress Telemetry**:
  - Completed 2,500 / 10,000 cycles: PASS
  - Completed 5,000 / 10,000 cycles: PASS
  - Completed 7,500 / 10,000 cycles: PASS
  - Completed 10,000 / 10,000 cycles: PASS
- **Bitmap Drift Result**:
  $$\text{Initial Free Blocks} = 1,109$$
  $$\text{Final Free Blocks} = 1,109$$
  $$\text{Observed Bitmap Drift} = 0 \text{ Blocks (Zero Leakage, Zero Corruption)}$$

---

## 13. ABDE DIAGNOSTIC DISPLAY IMPLEMENTATION

The in-kernel diagnostic module ([`kernel/debug/bofs_allocation_test.c`](file:///D:/Signatures_OS/kernel/debug/bofs_allocation_test.c)) renders an ABDE display to the GOP framebuffer during boot:

```text
+-----------------------------------------------------------------------------------+
|  ATOMS OS -- BOFS PHASE 4 BLOCK ALLOCATION ENGINE CERTIFICATION               [|] |
|  Volume: 17,500 Blks (70MB) | 4KB Blk | Data Pool: Blk 16,391..17,499 | Free: 1,109|
+-----------------------------------------------------------------------------------+
|  01. Allocator Init & Free Block Count (1,109)                             PASS   |
|  02. Single Block Allocation (Block 16,391)                               PASS   |
|  03. Single Block Free & Count Restoration                                 PASS   |
|  04. Contiguous Allocation (50 Blocks)                                     PASS   |
|  05. Contiguous Range Free (50 Blocks)                                     PASS   |
|  06. Fragmented Multi-Extent Allocation                                    PASS   |
|  07. Reservation Acquisition (Reserve != Commit)                           PASS   |
|  08. In-Memory Collision Avoidance (Skip Lock)                             PASS   |
|  09. Reservation Rollback (Zero Disk Mutation)                             PASS   |
|  10. Reservation Commit to Persistent Storage                              PASS   |
|  11. Double-Free Protection Rejection (-16)                                PASS   |
|  12. Double-Allocation Bit Boundary Guard                                  PASS   |
|  13. Out-of-Space Handling (ENOSPC, -28)                                   PASS   |
|  14. Bitmap Flush & Reopen Free Consistency                                PASS   |
|  15. Metadata Protection & Out-of-Bounds Rejection                         PASS   |
|  16. 1,000-Cycle Stress (Bitmap Drift: 0 Blocks)                           PASS   |
|                                                                                   |
|  QEMU Pre-Flight Boot Validation:          PASS [16/16 IN-KERNEL CERTIFIED]       |
|  Real-Hardware Controlled Target:          REAL-HARDWARE BOFS TEST VOLUME: N/A    |
|  Production Safety Guard:                  FOREIGN NVMe/NTFS: WRITE LOCKED (0B)   |
+-----------------------------------------------------------------------------------+
|  BOFS PHASE 4 BLOCK ALLOCATION ENGINE STATUS: CERTIFIED (PASS)                    |
+-----------------------------------------------------------------------------------+
```

- **Heartbeat Spinner**: Located at top-right (`g_kernel_screen_width - 60, 40`), rotating through `|`, `/`, `-`, `\`.

---

## 14. REAL-HARDWARE SAFETY PROTOCOL

1. **Strict Device Isolation**:
   - The allocator interacts only with registered `BlockDevice` structures.
   - The in-kernel diagnostic module utilizes a dedicated in-memory mock block device.
2. **Production Partition Write Locking**:
   - ATOMS OS mounts host Windows 11 NTFS, EFI System, and recovery partitions strictly read-only.
   - The allocator issues zero write commands to any NVMe namespace containing host partitions.
3. **Hardware Gate Verdict**:
   - `REAL-HARDWARE BOFS TEST VOLUME: NOT AVAILABLE`
   - `FOREIGN NVMe/NTFS/BOOT PARTITIONS: WRITE LOCKED (0 BYTES TOUCHED)`

---

## 15. FORENSIC AUDIT AND CODE VERIFICATION

### Freestanding Toolchain Compliance
The allocator implementation ([`kernel/vfs/bofs/src/bofs_alloc.c`](file:///D:/Signatures_OS/kernel/vfs/bofs/src/bofs_alloc.c)) and diagnostic harness ([`kernel/debug/bofs_allocation_test.c`](file:///D:/Signatures_OS/kernel/debug/bofs_allocation_test.c)) compile cleanly under the target freestanding toolchain:

```bash
clang -target x86_64-pc-none-elf \
      -mno-sse -mno-sse2 -mno-mmx -msoft-float \
      -ffreestanding -mno-red-zone \
      -Wall -Wextra -Werror \
      -I. -c kernel/vfs/bofs/src/bofs_alloc.c
```

**Verification Result**: **Zero warnings, Zero errors**.

---

## 16. KNOWN LIMITATIONS AND TRADE-OFFS

1. **Linear Bitmap Cursor**: Next-fit linear search scales linearly $O(N)$ with total blocks. For multi-terabyte volumes, future iterations will introduce multi-level bitmap summaries or tree indexes.
2. **Fixed In-Memory Reservation Slots**: The current allocator context supports up to 32 concurrent reservation handles (`BOFS_MAX_RESERVATIONS = 32`). This is sufficient for single-threaded or cooperative multi-tasking kernel operations.
3. **Single Extent Cache Block**: Currently caches one 4,096-byte bitmap block (32,768 blocks). Allocations spanning block boundaries trigger cache evictions.

---

## 17. PHASE 5 TRANSITION PLAN

Phase 4 completes the storage block allocation layer. The foundation is now certified for **Phase 5: BOFS Metadata + File Engine**:
- Inode allocation and deallocation utilizing `$InodeBitmap`.
- File extent mapping using `bofs_alloc_blocks_contiguous` and `bofs_alloc_blocks_fragmented`.
- File truncate and shrink using `bofs_free_block` and `bofs_free_blocks`.
- Transactional file creation backed by reservation commit/rollback.

---

## 18. APPENDIX A: BITMAP MATH FORMULAS

1. **Block Index to Bitmap LBA**:
   $$\text{BMP\_DISK\_BLK} = \text{sb.block\_bitmap\_start\_block} + \left\lfloor \frac{\text{block\_num}}{32768} \right\rfloor$$
   $$\text{LBA} = \text{BMP\_DISK\_BLK} \times \left( \frac{4096}{\text{sector\_size}} \right)$$

2. **Bit Operations within 4KB Buffer**:
   $$\text{Byte Offset} = \frac{\text{block\_num} \pmod{32768}}{8}$$
   $$\text{Bit Mask} = 1 \ll (\text{block\_num} \pmod 8)$$

---

## 19. APPENDIX B: MEMORY FOOTPRINT ANALYSIS

| Data Structure | Size | Location |
| :--- | :---: | :--- |
| `bofs_allocator_t` | 4,720 Bytes | Stack or Heap allocation context |
| `bofs_alloc_extent_list_t` | 200 Bytes | Per-allocation local stack variable |
| `bofs_reservation_t` (x32) | 512 Bytes | Embedded within `bofs_allocator_t` |
| `cached_bitmap` buffer | 4,096 Bytes | Embedded 64-byte aligned buffer in `bofs_allocator_t` |

---

## 20. APPENDIX C: HOST TEST LOG OUTPUT

```text
================================================================
 ATOMS OS — BOFS PHASE 4 BLOCK ALLOCATION ENGINE TESTS (T01-T20)
================================================================
[*] Initial Data Pool Free Blocks: 1109
[*] Running 10,000-cycle allocation/free stress test...
    - Completed 2500 / 10,000 cycles
    - Completed 5000 / 10,000 cycles
    - Completed 7500 / 10,000 cycles
    - Completed 10000 / 10,000 cycles
[*] Final Free Count: 1109 (Matches Baseline: 1109)

--- PHASE 4 TEST MATRIX RESULTS ---
[PASS] T01 Initial bitmap state
[PASS] T02 Allocate one block
[PASS] T03 Free one block
[PASS] T04 Allocate contiguous range
[PASS] T05 Free contiguous range
[PASS] T06 Fragmented allocation
[PASS] T07 Reservation acquisition
[PASS] T08 Reservation collision
[PASS] T09 Reservation rollback
[PASS] T10 Double allocation
[PASS] T11 Double free
[PASS] T12 Out-of-space
[PASS] T13 Bitmap persistence
[PASS] T14 Reopen persistence
[PASS] T15 Bitmap corruption
[PASS] T16 Geometry boundary
[PASS] T17 Metadata protection
[PASS] T18 Overflow/invalid request
[PASS] T19 Repeated allocation/free stress
[PASS] T20 Final free-space baseline restoration
----------------------------------------------------------------
TOTAL TESTS: 20 | PASS: 20 | FAIL: 0
================================================================
```

---

## 21. CERTIFICATION SIGN-OFF AND VERDICTS

```
================================================================================
 ATOMS OS / BOFS ARCHITECTURAL CERTIFICATION VERDICT
================================================================================
 PHASE:                 PHASE 4 — BOFS BLOCK / ALLOCATION ENGINE
 SPECIFICATION:         ATOMS-BOFS-PHASE4-SPEC-001
 HOST TEST STATUS:      20 / 20 PASS (100% PASS RATE)
 IN-KERNEL TEST STATUS: 16 / 16 PASS (100% PASS RATE)
 STRESS DRIFT:          0 BLOCKS (10,000 CYCLES VERIFIED)
 RED-ZONE COMPLIANCE:   VERIFIED (-mno-red-zone -ffreestanding)
 REAL-HARDWARE GUARD:   ACTIVE (FOREIGN NTFS/NVMe PARTITIONS WRITE LOCKED)
 VERDICT:               CERTIFIED PASS
================================================================================
```
