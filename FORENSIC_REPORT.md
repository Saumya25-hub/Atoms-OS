# ATOMS OS — Forensic Investigation Report: MFT Traversal Freeze & Framebuffer Stall
**Investigation Date:** 2026-09-04  
**Investigator:** ATOMS Forensic Team  
**Subject:** Root Cause of OS Freeze ("OS STUCK HO GAYA") on Bare-Metal Target PC during Forensic Collection  
**Classification:** CRITICAL PERFORMANCE & STALL FORENSIC AUDIT (100% READ-ONLY)  

---

## 1. Executive Summary

When booting the physical target PC (ASUS PRIME B750M-K) into ATOMS OS Windows Forensic Collector, the bootloader and kernel initialize cleanly, and the storage stack mounts Partition 3. However, immediately upon reaching file collection, the operating system appears completely frozen ("OS stuck ho gaya he, active spinner bhi stuck").

Forensic code tracing of the MFT read and linear fallback engine reveals a **severe synchronous I/O and framebuffer rendering stall** that blocks the CPU for hours, simulating a complete kernel deadlock.

---

## 2. Root Cause Analysis

### Defect 1: Synchronous Framebuffer Dump on Every MFT Record
- **Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` (lines 790, 794).
- **Mechanism:** In `ntfs_mft_read_record()`, every single record read unconditionally invokes:
  ```c
  ntfs_mft_dump_diagnostics(rec, "2D: Primary MFT Read & Validation", true, NULL);
  // or on failure:
  ntfs_mft_dump_diagnostics(NULL, "2E: Primary MFT Record Failure", false, primary_err);
  ```
- **The Framebuffer Scrolling Bottleneck:**
  `ntfs_mft_dump_diagnostics()` executes 20 consecutive calls to `display_print()`.
  In graphical framebuffer mode (1920x1080 32bpp), each newline triggers a full-screen vertical scroll copying 8.3 MB of VRAM.
- **Mathematical Proof of Stall:**
  - Total MFT records on 243 GB partition: ~262,144 records.
  - Total calls to `display_print()`: $262,144 \times 20 = 5,242,880$ calls.
  - At ~5–10 milliseconds per framebuffer text scroll, scanning the MFT takes:
    $$\frac{5,242,880 \times 0.005\text{ s}}{60\text{ s/min}} \approx 436\text{ minutes} = \mathbf{7.2\text{ HOURS}}$$
  - The CPU is 100% pinned doing software blits to the video card. The user sees a frozen screen with zero responsiveness.

### Defect 2: Heap Thrashing & Cache Eviction Storm in `ntfs_find_file_in_mft()`
- **Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` (lines 650–789, 1855).
- **Mechanism:** `ntfs_find_file_in_mft()` called `ntfs_mft_read_record()` in a tight loop. On every single record:
  1. It performs heap allocation (`kmalloc`) for `raw_buf`.
  2. It performs heap allocation for `rec` (`NTFS_FileRecord`).
  3. It searches the 32-entry MFT cache, evicts existing entries, and allocates another 1024-byte `store_buf` on heap.
  4. It frees `rec` and `raw_buf`.
- **Result:** 262,144 $\times$ 3 = ~786,000 heap allocations/deallocations, exhausting heap buckets and causing severe allocator fragmentation and latency.

### Defect 3: Starvation of Heartbeat Spinner & NIC Polling
- **Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` inside `ntfs_find_file_in_mft()`.
- **Mechanism:** The loop `for (uint32_t rec_num = 16; rec_num < scan_limit; rec_num++)` ran synchronously without yielding CPU cycles to `update_spinner()` or `r8168_poll_receive()`.
- **Result:** The rotating spinner in the top right stopped rotating immediately upon entering file collection, giving the operator the appearance of a total system hang.

---

## 3. Suspected Fix Strategy (NO CODE)

1. **Silence `ntfs_mft_dump_diagnostics()` in Production:**
   Convert `ntfs_mft_dump_diagnostics()` into an empty no-op (or guard with a diagnostic flag) so that `ntfs_mft_read_record()` never writes to the framebuffer.
2. **High-Speed Zero-Allocation Inode Probe in `ntfs_find_file_in_mft()`:**
   Instead of full `NTFS_FileRecord` heap instantiation, read 1024-byte raw sectors directly into a single reused stack/scratch buffer.
   - If magic != `"FILE"`: skip immediately (1 CPU clock).
   - If `!(flags & NTFS_FILE_IN_USE)`: skip immediately.
   - Only when a `$FILE_NAME` matches `target_filename`, instantiate the full `NTFS_File` object and return.
3. **Heartbeat & Network Pacing in MFT Scanner:**
   Every 1,024 records scanned, call `r8168_poll_receive()` and update the ABDE dashboard spinner.
   This guarantees the spinner continuously rotates (`| / - \`) at high speed throughout the scan, providing visual proof of active operation.
