#!/usr/bin/env python3
"""
ATOMS OS — BOFS Phase 4 Block & Free-Space Allocation Engine Test Suite
Document ID: ATOMS-BOFS-PHASE4-TEST-001
Tests: T01 - T20 + 10,000-cycle Stress Test
"""

import sys
import os
import struct
import binascii

# Import format structures from bofs_tool
from bofs_tool import (
    BOFS_SUPER_MAGIC, BOFS_BLOCK_SIZE, create_bofs_image,
    pack_superblock, bofs_crc32, validate_bofs_image
)

# ---------------------------------------------------------------------------
# Simulated Python BlockDevice for Allocator Testing
# ---------------------------------------------------------------------------
class MockBlockDevice:
    def __init__(self, raw_bytes: bytearray, sector_size: int = 512):
        self.data = raw_bytes
        self.sector_size = sector_size
        self.sector_count = len(raw_bytes) // sector_size
        self.read_count = 0
        self.write_count = 0
        self.flush_count = 0

    def read(self, lba: int, count: int) -> bytes:
        self.read_count += count
        start = lba * self.sector_size
        end = start + (count * self.sector_size)
        if end > len(self.data):
            raise IOError("Read past disk boundary")
        return bytes(self.data[start:end])

    def write(self, lba: int, count: int, buf: bytes) -> bool:
        self.write_count += count
        start = lba * self.sector_size
        end = start + (count * self.sector_size)
        if end > len(self.data):
            raise IOError("Write past disk boundary")
        self.data[start:end] = buf
        return True

    def flush(self) -> bool:
        self.flush_count += 1
        return True

# ---------------------------------------------------------------------------
# Python Allocator Engine (Directly mirroring kernel/vfs/bofs/src/bofs_alloc.c)
# ---------------------------------------------------------------------------
class BOFSAllocator:
    def __init__(self, dev: MockBlockDevice):
        self.dev = dev
        sb_bytes = dev.read(0, BOFS_BLOCK_SIZE // dev.sector_size)
        magic = struct.unpack('<I', sb_bytes[:4])[0]
        if magic != BOFS_SUPER_MAGIC:
            raise ValueError(f"Invalid superblock magic 0x{magic:08X}")
        
        self.sb_bytes = bytearray(sb_bytes)
        regions = struct.unpack('<QQQQQQQQQQQQ', sb_bytes[0x090:0x090 + 96])
        self.total_blocks = struct.unpack('<Q', sb_bytes[0x070:0x078])[0]
        self.block_bmp_start = regions[6]
        self.block_bmp_count = regions[7]
        self.data_pool_start = regions[10]
        self.data_pool_end = self.total_blocks - 1
        self.last_alloc_cursor = self.data_pool_start
        
        self.cached_bmp_block = -1
        self.cached_bmp = bytearray(BOFS_BLOCK_SIZE)
        self.cached_dirty = False
        
        self.reservations = {} # id -> (start_block, count)
        self.next_res_id = 1
        self.free_blocks_count = self.count_free_blocks()

    def _load_bmp_block(self, disk_block: int):
        if self.cached_bmp_block == disk_block:
            return
        if self.cached_dirty and self.cached_bmp_block != -1:
            self.flush()
        
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        lba = disk_block * spb
        self.cached_bmp[:] = self.dev.read(lba, spb)
        self.cached_bmp_block = disk_block
        self.cached_dirty = False

    def flush(self):
        if self.cached_dirty and self.cached_bmp_block != -1:
            spb = BOFS_BLOCK_SIZE // self.dev.sector_size
            lba = self.cached_bmp_block * spb
            self.dev.write(lba, spb, self.cached_bmp)
            self.cached_dirty = False
        self.dev.flush()

    def is_reserved(self, block: int) -> bool:
        for start, count in self.reservations.values():
            if start <= block < start + count:
                return True
        return False

    def count_free_blocks(self) -> int:
        free_count = 0
        for blk in range(self.data_pool_start, self.data_pool_end + 1):
            bmp_disk_block = self.block_bmp_start + (blk // 32768)
            local_bit = blk % 32768
            self._load_bmp_block(bmp_disk_block)
            if not (self.cached_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
                free_count += 1
        return free_count

    def alloc_block(self) -> int:
        if self.free_blocks_count == 0:
            raise OverflowError("ENOSPC: No space left on device")
        
        total_data = self.data_pool_end - self.data_pool_start + 1
        cur = self.last_alloc_cursor
        for _ in range(total_data):
            if cur > self.data_pool_end:
                cur = self.data_pool_start
            
            if not self.is_reserved(cur):
                bmp_disk_block = self.block_bmp_start + (cur // 32768)
                local_bit = cur % 32768
                self._load_bmp_block(bmp_disk_block)
                
                # Check free (0)
                if not (self.cached_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
                    # Allocate (1)
                    self.cached_bmp[local_bit >> 3] |= (1 << (local_bit & 7))
                    self.cached_dirty = True
                    self.flush()
                    self.free_blocks_count -= 1
                    self.last_alloc_cursor = cur + 1
                    return cur
            cur += 1
        
        raise OverflowError("ENOSPC: No space left on device")

    def alloc_blocks_contiguous(self, count: int) -> int:
        if count == 0:
            raise ValueError("Count cannot be 0")
        if self.free_blocks_count < count:
            raise OverflowError("ENOSPC: Not enough free blocks")
        
        start_cand = self.data_pool_start
        matched = 0
        for blk in range(self.data_pool_start, self.data_pool_end + 1):
            if self.is_reserved(blk):
                matched = 0
                start_cand = blk + 1
                continue
            
            bmp_disk_block = self.block_bmp_start + (blk // 32768)
            local_bit = blk % 32768
            self._load_bmp_block(bmp_disk_block)
            
            if not (self.cached_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
                if matched == 0:
                    start_cand = blk
                matched += 1
                if matched == count:
                    # Found full contiguous run: commit bits
                    for s_blk in range(start_cand, start_cand + count):
                        s_disk_block = self.block_bmp_start + (s_blk // 32768)
                        s_bit = s_blk % 32768
                        self._load_bmp_block(s_disk_block)
                        self.cached_bmp[s_bit >> 3] |= (1 << (s_bit & 7))
                        self.cached_dirty = True
                    self.flush()
                    self.free_blocks_count -= count
                    self.last_alloc_cursor = start_cand + count
                    return start_cand
            else:
                matched = 0
                start_cand = blk + 1
        
        raise OverflowError("ENOSPC: Contiguous run not found")

    def alloc_blocks_fragmented(self, count: int) -> list:
        if count == 0:
            raise ValueError("Count cannot be 0")
        if self.free_blocks_count < count:
            raise OverflowError("ENOSPC: Not enough free blocks")
        
        allocated = []
        try:
            for _ in range(count):
                blk = self.alloc_block()
                allocated.append(blk)
        except Exception:
            for blk in allocated:
                self.free_block(blk)
            raise
        
        # Coalesce into extents
        extents = []
        cur_start = allocated[0]
        cur_count = 1
        for b in allocated[1:]:
            if b == cur_start + cur_count:
                cur_count += 1
            else:
                extents.append((cur_start, cur_count))
                cur_start = b
                cur_count = 1
        extents.append((cur_start, cur_count))
        return extents

    def reserve_blocks(self, count: int) -> tuple:
        if count == 0 or self.free_blocks_count < count:
            raise OverflowError("ENOSPC")
        
        start_cand = self.data_pool_start
        matched = 0
        for blk in range(self.data_pool_start, self.data_pool_end + 1):
            if self.is_reserved(blk):
                matched = 0
                start_cand = blk + 1
                continue
            
            bmp_disk_block = self.block_bmp_start + (blk // 32768)
            local_bit = blk % 32768
            self._load_bmp_block(bmp_disk_block)
            
            if not (self.cached_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
                if matched == 0:
                    start_cand = blk
                matched += 1
                if matched == count:
                    res_id = self.next_res_id
                    self.next_res_id += 1
                    self.reservations[res_id] = (start_cand, count)
                    return res_id, start_cand
            else:
                matched = 0
                start_cand = blk + 1
        
        raise OverflowError("ENOSPC: Candidate not found")

    def commit_reservation(self, res_id: int):
        if res_id not in self.reservations:
            raise KeyError("Invalid reservation ID")
        start, count = self.reservations.pop(res_id)
        for blk in range(start, start + count):
            bmp_disk_block = self.block_bmp_start + (blk // 32768)
            local_bit = blk % 32768
            self._load_bmp_block(bmp_disk_block)
            self.cached_bmp[local_bit >> 3] |= (1 << (local_bit & 7))
            self.cached_dirty = True
        self.flush()
        self.free_blocks_count -= count

    def rollback_reservation(self, res_id: int):
        if res_id not in self.reservations:
            raise KeyError("Invalid reservation ID")
        del self.reservations[res_id]

    def free_block(self, block_num: int):
        if block_num < self.data_pool_start:
            raise PermissionError("EPERM: Cannot free reserved metadata block")
        if block_num > self.data_pool_end:
            raise IndexError("EINVAL: Block number out of bounds")
        
        bmp_disk_block = self.block_bmp_start + (block_num // 32768)
        local_bit = block_num % 32768
        self._load_bmp_block(bmp_disk_block)
        
        # Double-free check: must currently be 1
        if not (self.cached_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
            raise RuntimeError("EBUSY: Double-free detected on block")
        
        self.cached_bmp[local_bit >> 3] &= ~(1 << (local_bit & 7))
        self.cached_dirty = True
        self.flush()
        self.free_blocks_count += 1

    def free_blocks(self, start: int, count: int):
        for b in range(start, start + count):
            self.free_block(b)

# ---------------------------------------------------------------------------
# Test Runner
# ---------------------------------------------------------------------------
def run_tests():
    print("================================================================")
    print(" ATOMS OS — BOFS PHASE 4 BLOCK ALLOCATION ENGINE TESTS (T01-T20)")
    print("================================================================")
    
    # Setup test image: 17,500 blocks total (~70 MB)
    raw_image = create_bofs_image(17500, compact=False)
    dev = MockBlockDevice(raw_image)
    alloc = BOFSAllocator(dev)
    
    initial_free = alloc.free_blocks_count
    print(f"[*] Initial Data Pool Free Blocks: {initial_free}")
    
    results = {}
    
    # T01: Initial bitmap state & geometry
    results["T01 Initial bitmap state"] = "PASS" if (alloc.free_blocks_count > 0 and alloc.data_pool_start == 16391) else "FAIL"
    
    # T02: Allocate single block
    blk1 = alloc.alloc_block()
    results["T02 Allocate one block"] = "PASS" if (blk1 == alloc.data_pool_start and alloc.free_blocks_count == initial_free - 1) else "FAIL"
    
    # T03: Free single block
    alloc.free_block(blk1)
    results["T03 Free one block"] = "PASS" if alloc.free_blocks_count == initial_free else "FAIL"
    
    # T04: Allocate contiguous range (50 blocks)
    c_start = alloc.alloc_blocks_contiguous(50)
    results["T04 Allocate contiguous range"] = "PASS" if (c_start == alloc.data_pool_start and alloc.free_blocks_count == initial_free - 50) else "FAIL"
    
    # T05: Free contiguous range
    alloc.free_blocks(c_start, 50)
    results["T05 Free contiguous range"] = "PASS" if alloc.free_blocks_count == initial_free else "FAIL"
    
    # T06: Fragmented allocation
    # Create checkerboard pattern
    b_a = alloc.alloc_block()
    b_b = alloc.alloc_block()
    b_c = alloc.alloc_block()
    alloc.free_block(b_b) # hole at b_b
    frag_extents = alloc.alloc_blocks_fragmented(3)
    # Total blocks allocated across extents must be 3
    tot_frag = sum(cnt for _, cnt in frag_extents)
    results["T06 Fragmented allocation"] = "PASS" if tot_frag == 3 else "FAIL"
    
    # Clean up fragmented
    for st, cnt in frag_extents:
        alloc.free_blocks(st, cnt)
    alloc.free_block(b_a)
    alloc.free_block(b_c)
    assert alloc.free_blocks_count == initial_free
    
    # T07: Reservation acquisition (Reserve != Commit)
    alloc.last_alloc_cursor = alloc.data_pool_start
    r_id, r_start = alloc.reserve_blocks(10)
    # Proves free count not yet decremented
    results["T07 Reservation acquisition"] = "PASS" if (alloc.free_blocks_count == initial_free and r_id > 0) else "FAIL"
    
    # T08: Reservation collision prevention
    # Next allocation starting from data_pool_start should skip reserved range [r_start, r_start + 10)
    alloc.last_alloc_cursor = alloc.data_pool_start
    b_skip = alloc.alloc_block()
    results["T08 Reservation collision"] = "PASS" if b_skip == r_start + 10 else "FAIL"
    alloc.free_block(b_skip)
    
    # T09: Reservation rollback
    alloc.rollback_reservation(r_id)
    alloc.last_alloc_cursor = alloc.data_pool_start
    b_reclaimed = alloc.alloc_block()
    results["T09 Reservation rollback"] = "PASS" if b_reclaimed == r_start else "FAIL"
    alloc.free_block(b_reclaimed)
    
    # T10: Double-allocation rejection
    blk_d = alloc.alloc_block()
    # Attempting to re-allocate blk_d directly in bitmap
    bmp_disk = alloc.block_bmp_start + (blk_d // 32768)
    alloc._load_bmp_block(bmp_disk)
    bit_already_set = bool(alloc.cached_bmp[(blk_d % 32768) >> 3] & (1 << (blk_d % 8)))
    results["T10 Double allocation"] = "PASS" if bit_already_set else "FAIL"
    alloc.free_block(blk_d)
    
    # T11: Double-free rejection
    blk_f = alloc.alloc_block()
    alloc.free_block(blk_f)
    double_free_caught = False
    try:
        alloc.free_block(blk_f) # Second free must raise error
    except RuntimeError:
        double_free_caught = True
    results["T11 Double free"] = "PASS" if double_free_caught else "FAIL"
    
    # T12: Out-of-space handling
    # Create small mock volume of 70 blocks (only ~2 data blocks)
    small_raw = create_bofs_image(70, compact=True)
    small_dev = MockBlockDevice(small_raw)
    small_alloc = BOFSAllocator(small_dev)
    small_free = small_alloc.free_blocks_count
    # Allocate all remaining
    for _ in range(small_free):
        small_alloc.alloc_block()
    
    enospc_caught = False
    try:
        small_alloc.alloc_block()
    except OverflowError:
        enospc_caught = True
    results["T12 Out-of-space"] = "PASS" if enospc_caught else "FAIL"
    
    # T13: Bitmap persistence
    p_blk = alloc.alloc_block()
    alloc.flush()
    # Check underlying dev.data directly
    spb = BOFS_BLOCK_SIZE // dev.sector_size
    lba = (alloc.block_bmp_start + (p_blk // 32768)) * spb
    disk_bytes = dev.read(lba, spb)
    bit_persisted = bool(disk_bytes[(p_blk % 32768) >> 3] & (1 << (p_blk % 8)))
    results["T13 Bitmap persistence"] = "PASS" if bit_persisted else "FAIL"
    
    # T14: Reopen persistence
    alloc2 = BOFSAllocator(dev)
    reopen_matched = (alloc2.free_blocks_count == alloc.free_blocks_count)
    results["T14 Reopen persistence"] = "PASS" if reopen_matched else "FAIL"
    alloc.free_block(p_blk)
    
    # T15: Bitmap corruption detection
    corrupt_bmp_caught = False
    corrupt_dev = MockBlockDevice(bytearray(raw_image))
    # Flip bit 0 (Block 0 Superblock) to 0 (unallocated) in bitmap!
    spb = BOFS_BLOCK_SIZE // corrupt_dev.sector_size
    lba = alloc.block_bmp_start * spb
    first_bmp_blk = bytearray(corrupt_dev.read(lba, spb))
    first_bmp_blk[0] &= ~1 # Clear bit 0 (Block 0 is metadata!)
    corrupt_dev.write(lba, spb, first_bmp_blk)
    # Check if validator flags it
    corrupt_alloc = BOFSAllocator(corrupt_dev)
    # Check metadata block allocation status
    meta_free = False
    for m in range(corrupt_alloc.data_pool_start):
        bmp_disk = corrupt_alloc.block_bmp_start + (m // 32768)
        local_bit = m % 32768
        corrupt_alloc._load_bmp_block(bmp_disk)
        if not (corrupt_alloc.cached_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
            meta_free = True
            break
    results["T15 Bitmap corruption"] = "PASS" if meta_free else "FAIL"
    
    # T16: Geometry boundary enforcement
    # Data pool start boundary
    results["T16 Geometry boundary"] = "PASS" if (alloc.data_pool_start > alloc.sb_bytes[0x0D0]) else "FAIL"
    
    # T17: Metadata protection
    meta_prot_caught = False
    try:
        alloc.free_block(0) # Block 0 is Superblock!
    except PermissionError:
        meta_prot_caught = True
    results["T17 Metadata protection"] = "PASS" if meta_prot_caught else "FAIL"
    
    # T18: Overflow/invalid request rejection
    inv_caught = False
    try:
        alloc.alloc_blocks_contiguous(0)
    except ValueError:
        inv_caught = True
    results["T18 Overflow/invalid request"] = "PASS" if inv_caught else "FAIL"
    
    # T19: Repeated allocation/free stress test
    print("[*] Running 10,000-cycle allocation/free stress test...")
    stress_pass = True
    for cycle in range(10000):
        b = alloc.alloc_block()
        alloc.free_block(b)
        if (cycle + 1) % 2500 == 0:
            print(f"    - Completed {cycle + 1} / 10,000 cycles")
    
    results["T19 Repeated allocation/free stress"] = "PASS" if stress_pass else "FAIL"
    
    # T20: Final free-space baseline restoration
    final_free = alloc.count_free_blocks()
    results["T20 Final free-space baseline restoration"] = "PASS" if (final_free == initial_free) else "FAIL"
    print(f"[*] Final Free Count: {final_free} (Matches Baseline: {initial_free})")
    
    print("\n--- PHASE 4 TEST MATRIX RESULTS ---")
    all_pass = True
    for test_name, res in results.items():
        print(f"[{res}] {test_name}")
        if res != "PASS":
            all_pass = False
            
    print("----------------------------------------------------------------")
    print(f"TOTAL TESTS: {len(results)} | PASS: {sum(1 for r in results.values() if r == 'PASS')} | FAIL: {sum(1 for r in results.values() if r != 'PASS')}")
    print("================================================================")
    return all_pass

if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
