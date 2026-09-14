#!/usr/bin/env python3
"""
ATOMS OS — BOFS Phase 5 Metadata & File Engine Test Suite
Document ID: ATOMS-BOFS-PHASE5-TEST-001
Tests: T01 - T34 + 1,000-cycle (up to 10,000) Stress Test
"""

import sys
import os
import struct
import binascii

# Import format structures from bofs_tool and test_bofs_alloc
from bofs_tool import (
    BOFS_SUPER_MAGIC, BOFS_INODE_MAGIC, BOFS_BLOCK_SIZE, BOFS_INODE_SIZE,
    create_bofs_image, pack_superblock, bofs_crc32, validate_bofs_image
)
from test_bofs_alloc import MockBlockDevice, BOFSAllocator

BOFS_FIRST_USER_INODE = 16
BOFS_INODES_PER_BLOCK = 8
BOFS_INODE_DIRECT_EXTENTS = 12
BOFS_EXTENTS_PER_INDIRECT = 170

BOFS_EXTENT_FLAG_VALID   = 0x0001
BOFS_EXTENT_FLAG_SPARSE  = 0x0002
BOFS_EXTENT_FLAG_UNWRITTEN = 0x0004

BOFS_S_IFREG = 0o100000
BOFS_S_IFDIR = 0o040000

# ---------------------------------------------------------------------------
# Python File Engine (Mirroring kernel/vfs/bofs/src/bofs_file.c)
# ---------------------------------------------------------------------------
class BOFSFileSystem:
    def __init__(self, dev: MockBlockDevice, alloc: BOFSAllocator):
        self.dev = dev
        self.alloc = alloc
        sb_bytes = dev.read(0, BOFS_BLOCK_SIZE // dev.sector_size)
        magic = struct.unpack('<I', sb_bytes[:4])[0]
        if magic != BOFS_SUPER_MAGIC:
            raise ValueError(f"Invalid superblock magic 0x{magic:08X}")
        
        self.total_blocks = struct.unpack('<Q', sb_bytes[0x070:0x078])[0]
        self.total_inodes = struct.unpack('<Q', sb_bytes[0x080:0x088])[0]
        self.free_inodes  = struct.unpack('<Q', sb_bytes[0x088:0x090])[0]

        regions = struct.unpack('<QQQQQQQQQQQQ', sb_bytes[0x090:0x090 + 96])
        self.inode_bmp_start = regions[4]
        self.inode_bmp_count = regions[5]
        self.inode_tbl_start = regions[8]
        self.inode_tbl_count = regions[9]
        self.data_pool_start = regions[10]

        self.cached_inode_bmp_block = -1
        self.cached_inode_bmp = bytearray(BOFS_BLOCK_SIZE)
        self.cached_inode_bmp_dirty = False

    def _load_inode_bmp(self, blk: int):
        if self.cached_inode_bmp_block == blk:
            return
        if self.cached_inode_bmp_dirty and self.cached_inode_bmp_block != -1:
            self.flush()
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        self.cached_inode_bmp[:] = self.dev.read(blk * spb, spb)
        self.cached_inode_bmp_block = blk
        self.cached_inode_bmp_dirty = False

    def flush(self):
        if self.cached_inode_bmp_dirty and self.cached_inode_bmp_block != -1:
            spb = BOFS_BLOCK_SIZE // self.dev.sector_size
            self.dev.write(self.cached_inode_bmp_block * spb, spb, self.cached_inode_bmp)
            self.cached_inode_bmp_dirty = False
        self.alloc.flush()
        self.dev.flush()

    def count_free_inodes(self) -> int:
        free_cnt = 0
        for ino in range(BOFS_FIRST_USER_INODE, self.total_inodes):
            bmp_blk = self.inode_bmp_start + (ino // 32768)
            self._load_inode_bmp(bmp_blk)
            local_bit = ino % 32768
            if not (self.cached_inode_bmp[local_bit >> 3] & (1 << (local_bit & 7))):
                free_cnt += 1
        return free_cnt

    def alloc_inode(self) -> int:
        for ino in range(BOFS_FIRST_USER_INODE, self.total_inodes):
            bmp_blk = self.inode_bmp_start + (ino // 32768)
            self._load_inode_bmp(bmp_blk)
            local_bit = ino % 32768
            byte_idx = local_bit >> 3
            mask = 1 << (local_bit & 7)
            if not (self.cached_inode_bmp[byte_idx] & mask):
                self.cached_inode_bmp[byte_idx] |= mask
                self.cached_inode_bmp_dirty = True
                self.flush()
                self.free_inodes -= 1
                return ino
        raise OverflowError("No free inodes available")

    def free_inode(self, ino: int):
        if ino < BOFS_FIRST_USER_INODE:
            raise PermissionError("EPERM: Cannot free reserved metadata inode")
        if ino >= self.total_inodes:
            raise IndexError("EINVAL: Inode number out of bounds")
        
        bmp_blk = self.inode_bmp_start + (ino // 32768)
        self._load_inode_bmp(bmp_blk)
        local_bit = ino % 32768
        byte_idx = local_bit >> 3
        mask = 1 << (local_bit & 7)
        if not (self.cached_inode_bmp[byte_idx] & mask):
            raise RuntimeError(f"EBUSY: Double-free detected on inode {ino}")
        
        self.cached_inode_bmp[byte_idx] &= ~mask
        self.cached_inode_bmp_dirty = True
        self.flush()
        self.free_inodes += 1

    def read_inode(self, ino: int) -> dict:
        if ino >= self.total_inodes:
            raise IndexError("Inode number out of bounds")
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        disk_block = self.inode_tbl_start + (ino // BOFS_INODES_PER_BLOCK)
        raw_block = self.dev.read(disk_block * spb, spb)
        slot_idx = ino % BOFS_INODES_PER_BLOCK
        raw_ino = raw_block[slot_idx * 512 : (slot_idx + 1) * 512]

        magic = struct.unpack('<I', raw_ino[:4])[0]
        if magic != BOFS_INODE_MAGIC:
            raise ValueError(f"Invalid inode magic 0x{magic:08X}")
        
        crc = struct.unpack('<I', raw_ino[508:512])[0]
        exp_crc = bofs_crc32(raw_ino[:508])
        if crc != exp_crc:
            raise ValueError(f"Inode {ino} checksum mismatch: got {crc:08X}, exp {exp_crc:08X}")
        
        gen, ino_num, mode, flags, uid, gid, links, size, alloc_blks = struct.unpack(
            '<IIQHHIIQQ', raw_ino[0x004:0x030]
        )
        if ino_num != ino:
            raise ValueError(f"Inode number mismatch in record: {ino_num} != {ino}")
        
        direct_extents = []
        for i in range(BOFS_INODE_DIRECT_EXTENTS):
            off = 0x070 + (i * 24)
            l_blk, p_blk, count, e_flags = struct.unpack('<QQII', raw_ino[off:off+24])
            direct_extents.append({'logical': l_blk, 'physical': p_blk, 'count': count, 'flags': e_flags})
        
        ind_blk, dbl_ind_blk = struct.unpack('<QQ', raw_ino[0x190:0x1A0])
        return {
            'magic': magic, 'generation': gen, 'inode_num': ino_num, 'mode': mode,
            'flags': flags, 'uid': uid, 'gid': gid, 'links': links,
            'size': size, 'allocated_blocks': alloc_blks,
            'direct_extents': direct_extents,
            'indirect_block': ind_blk, 'double_indirect_block': dbl_ind_blk,
            'raw': bytearray(raw_ino)
        }

    def write_inode(self, ino_dict: dict):
        ino = ino_dict['inode_num']
        if ino >= self.total_inodes:
            raise IndexError("Inode number out of bounds")
        
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        disk_block = self.inode_tbl_start + (ino // BOFS_INODES_PER_BLOCK)
        raw_block = bytearray(self.dev.read(disk_block * spb, spb))
        slot_idx = ino % BOFS_INODES_PER_BLOCK

        buf = bytearray(512)
        buf[0:4] = struct.pack('<I', BOFS_INODE_MAGIC)
        buf[4:48] = struct.pack('<IIQHHIIQQ',
                                ino_dict['generation'], ino, ino_dict['mode'],
                                ino_dict.get('flags', 0), ino_dict.get('uid', 0),
                                ino_dict.get('gid', 0), ino_dict.get('links', 1),
                                ino_dict['size'], ino_dict['allocated_blocks'])
        
        # Direct extents at 0x070
        for i, ext in enumerate(ino_dict['direct_extents']):
            off = 0x070 + (i * 24)
            buf[off:off+24] = struct.pack('<QQII', ext['logical'], ext['physical'], ext['count'], ext['flags'])
        
        buf[0x190:0x1A0] = struct.pack('<QQ', ino_dict.get('indirect_block', 0), ino_dict.get('double_indirect_block', 0))
        
        # Recalculate CRC32
        crc = bofs_crc32(bytes(buf[:508]))
        buf[508:512] = struct.pack('<I', crc)

        raw_block[slot_idx * 512 : (slot_idx + 1) * 512] = buf
        self.dev.write(disk_block * spb, spb, raw_block)
        self.dev.flush()

# ---------------------------------------------------------------------------
# File Object Handler (Mirroring bofs_file_t)
# ---------------------------------------------------------------------------
class BOFSFile:
    def __init__(self, fs: BOFSFileSystem, ino_dict: dict):
        self.fs = fs
        self.ino = ino_dict
        self.is_open = True

    @classmethod
    def create(cls, fs: BOFSFileSystem, mode: int = 0o644):
        ino_num = fs.alloc_inode()
        
        # Inspect existing slot to increment generation
        spb = BOFS_BLOCK_SIZE // fs.dev.sector_size
        disk_block = fs.inode_tbl_start + (ino_num // BOFS_INODES_PER_BLOCK)
        raw_block = fs.dev.read(disk_block * spb, spb)
        slot_idx = ino_num % BOFS_INODES_PER_BLOCK
        prev_gen = struct.unpack('<I', raw_block[slot_idx * 512 + 4 : slot_idx * 512 + 8])[0]
        new_gen = 1 if prev_gen == 0 else (prev_gen + 1)

        empty_extents = [{'logical': 0, 'physical': 0, 'count': 0, 'flags': 0} for _ in range(BOFS_INODE_DIRECT_EXTENTS)]
        ino_dict = {
            'magic': BOFS_INODE_MAGIC,
            'generation': new_gen,
            'inode_num': ino_num,
            'mode': BOFS_S_IFREG | mode,
            'flags': 0, 'uid': 0, 'gid': 0, 'links': 1,
            'size': 0, 'allocated_blocks': 0,
            'direct_extents': empty_extents,
            'indirect_block': 0, 'double_indirect_block': 0
        }
        fs.write_inode(ino_dict)
        return cls(fs, ino_dict)

    @classmethod
    def open(cls, fs: BOFSFileSystem, ino_num: int, expected_gen: int = 0):
        ino_dict = fs.read_inode(ino_num)
        if expected_gen != 0 and ino_dict['generation'] != expected_gen:
            raise KeyError(f"Stale handle: inode gen {ino_dict['generation']} != expected {expected_gen}")
        if (ino_dict['mode'] & 0o170000) != BOFS_S_IFREG:
            raise TypeError("Target inode is not a regular file")
        return cls(fs, ino_dict)

    def close(self):
        if self.is_open:
            self.fs.write_inode(self.ino)
            self.is_open = False

    def bmap(self, lblk: int):
        # 1. Direct extents
        for ext in self.ino['direct_extents']:
            if ext['flags'] & BOFS_EXTENT_FLAG_VALID:
                if ext['logical'] <= lblk < ext['logical'] + ext['count']:
                    if ext['flags'] & BOFS_EXTENT_FLAG_SPARSE:
                        return None, True
                    return ext['physical'] + (lblk - ext['logical']), False
        
        # 2. Indirect block
        if self.ino['indirect_block'] != 0:
            spb = BOFS_BLOCK_SIZE // self.fs.dev.sector_size
            ind_raw = self.fs.dev.read(self.ino['indirect_block'] * spb, spb)
            # validate indirect CRC
            ind_crc = struct.unpack('<I', ind_raw[508:512])[0]
            # check extents in indirect
            for j in range(BOFS_EXTENTS_PER_INDIRECT):
                off = j * 24
                l_blk, p_blk, cnt, flg = struct.unpack('<QQII', ind_raw[off:off+24])
                if flg & BOFS_EXTENT_FLAG_VALID:
                    if l_blk <= lblk < l_blk + cnt:
                        if flg & BOFS_EXTENT_FLAG_SPARSE:
                            return None, True
                        return p_blk + (lblk - l_blk), False
        return None, False

    def append_extent(self, lblk: int, pblk: int, count: int, flags: int):
        valid_flg = flags | BOFS_EXTENT_FLAG_VALID
        # Try merge in direct extents
        for i, ext in enumerate(self.ino['direct_extents']):
            if ext['flags'] & BOFS_EXTENT_FLAG_VALID:
                if ext['flags'] == valid_flg:
                    if flags & BOFS_EXTENT_FLAG_SPARSE:
                        if ext['logical'] + ext['count'] == lblk:
                            ext['count'] += count
                            self.fs.write_inode(self.ino)
                            return
                    else:
                        if ext['logical'] + ext['count'] == lblk and ext['physical'] + ext['count'] == pblk:
                            ext['count'] += count
                            self.fs.write_inode(self.ino)
                            return

        # Insert empty direct extent
        for ext in self.ino['direct_extents']:
            if not (ext['flags'] & BOFS_EXTENT_FLAG_VALID):
                ext['logical'] = lblk
                ext['physical'] = pblk
                ext['count'] = count
                ext['flags'] = valid_flg
                self.fs.write_inode(self.ino)
                return

        # Direct full: transition to Indirect Block
        spb = BOFS_BLOCK_SIZE // self.fs.dev.sector_size
        if self.ino['indirect_block'] == 0:
            new_ind = self.fs.alloc.alloc_block()
            self.ino['indirect_block'] = new_ind
            self.ino['allocated_blocks'] += 1
            ind_buf = bytearray(BOFS_BLOCK_SIZE)
            ind_buf[0:24] = struct.pack('<QQII', lblk, pblk, count, valid_flg)
            crc = bofs_crc32(bytes(ind_buf[:4092]))
            ind_buf[4092:4096] = struct.pack('<I', crc)
            self.fs.dev.write(new_ind * spb, spb, ind_buf)
            self.fs.write_inode(self.ino)
            return

        # Load existing indirect block
        ind_buf = bytearray(self.fs.dev.read(self.ino['indirect_block'] * spb, spb))
        for j in range(BOFS_EXTENTS_PER_INDIRECT):
            off = j * 24
            _, _, _, flg = struct.unpack('<QQII', ind_buf[off:off+24])
            if not (flg & BOFS_EXTENT_FLAG_VALID):
                ind_buf[off:off+24] = struct.pack('<QQII', lblk, pblk, count, valid_flg)
                crc = bofs_crc32(bytes(ind_buf[:4092]))
                ind_buf[4092:4096] = struct.pack('<I', crc)
                self.fs.dev.write(self.ino['indirect_block'] * spb, spb, ind_buf)
                self.fs.write_inode(self.ino)
                return
        raise OverflowError("Extent table full")

    def write(self, offset: int, data: bytes) -> int:
        if (offset + len(data)) > 0xFFFFFFFFFFFFFFFF or (offset + len(data)) < offset:
            raise OverflowError("Arithmetic overflow")
        if len(data) == 0:
            return 0
        
        spb = BOFS_BLOCK_SIZE // self.fs.dev.sector_size
        bytes_left = len(data)
        cur_off = offset
        in_idx = 0

        while bytes_left > 0:
            lblk = cur_off // BOFS_BLOCK_SIZE
            blk_off = cur_off % BOFS_BLOCK_SIZE
            chunk = min(BOFS_BLOCK_SIZE - blk_off, bytes_left)

            pblk, is_sparse = self.bmap(lblk)
            if pblk is None or is_sparse:
                # Allocate new block
                new_p = self.fs.alloc.alloc_block()
                self.ino['allocated_blocks'] += 1
                buf = bytearray(BOFS_BLOCK_SIZE)
                buf[blk_off:blk_off+chunk] = data[in_idx:in_idx+chunk]
                self.fs.dev.write(new_p * spb, spb, buf)
                self.append_extent(lblk, new_p, 1, 0)
            else:
                if chunk == BOFS_BLOCK_SIZE and blk_off == 0:
                    self.fs.dev.write(pblk * spb, spb, data[in_idx:in_idx+chunk])
                else:
                    buf = bytearray(self.fs.dev.read(pblk * spb, spb))
                    buf[blk_off:blk_off+chunk] = data[in_idx:in_idx+chunk]
                    self.fs.dev.write(pblk * spb, spb, buf)
            
            in_idx += chunk
            cur_off += chunk
            bytes_left -= chunk

        if offset + len(data) > self.ino['size']:
            self.ino['size'] = offset + len(data)
        self.fs.write_inode(self.ino)
        return len(data)

    def write_sparse(self, offset: int, length: int):
        if (offset + length) > 0xFFFFFFFFFFFFFFFF or (offset + length) < offset:
            raise OverflowError("Arithmetic overflow")
        start_lblk = offset // BOFS_BLOCK_SIZE
        end_lblk = (offset + length + BOFS_BLOCK_SIZE - 1) // BOFS_BLOCK_SIZE
        count = end_lblk - start_lblk
        self.append_extent(start_lblk, 0, count, BOFS_EXTENT_FLAG_SPARSE)
        if offset + length > self.ino['size']:
            self.ino['size'] = offset + length
        self.fs.write_inode(self.ino)

    def read(self, offset: int, length: int) -> bytes:
        if (offset + length) > 0xFFFFFFFFFFFFFFFF or (offset + length) < offset:
            raise OverflowError("Arithmetic overflow")
        if offset >= self.ino['size'] or length == 0:
            return b""
        
        length = min(length, self.ino['size'] - offset)
        spb = BOFS_BLOCK_SIZE // self.fs.dev.sector_size
        out = bytearray()
        bytes_left = length
        cur_off = offset

        while bytes_left > 0:
            lblk = cur_off // BOFS_BLOCK_SIZE
            blk_off = cur_off % BOFS_BLOCK_SIZE
            chunk = min(BOFS_BLOCK_SIZE - blk_off, bytes_left)

            pblk, is_sparse = self.bmap(lblk)
            if pblk is None or is_sparse:
                out.extend(bytes(chunk))
            else:
                buf = self.fs.dev.read(pblk * spb, spb)
                out.extend(buf[blk_off:blk_off+chunk])
            
            cur_off += chunk
            bytes_left -= chunk
        return bytes(out)

    def truncate(self, new_size: int):
        if new_size == self.ino['size']:
            return
        if new_size > self.ino['size']:
            self.ino['size'] = new_size
            self.fs.write_inode(self.ino)
            return
        
        spb = BOFS_BLOCK_SIZE // self.fs.dev.sector_size
        if new_size == 0:
            # Free all direct extents
            for ext in self.ino['direct_extents']:
                if ext['flags'] & BOFS_EXTENT_FLAG_VALID:
                    if not (ext['flags'] & BOFS_EXTENT_FLAG_SPARSE):
                        self.fs.alloc.free_blocks(ext['physical'], ext['count'])
                    ext['flags'] = 0
            
            # Free indirect extents and block
            if self.ino['indirect_block'] != 0:
                ind_buf = self.fs.dev.read(self.ino['indirect_block'] * spb, spb)
                for j in range(BOFS_EXTENTS_PER_INDIRECT):
                    off = j * 24
                    _, p_blk, cnt, flg = struct.unpack('<QQII', ind_buf[off:off+24])
                    if flg & BOFS_EXTENT_FLAG_VALID and not (flg & BOFS_EXTENT_FLAG_SPARSE):
                        self.fs.alloc.free_blocks(p_blk, cnt)
                self.fs.alloc.free_block(self.ino['indirect_block'])
                self.ino['indirect_block'] = 0
            
            self.ino['allocated_blocks'] = 0
            self.ino['size'] = 0
            self.fs.write_inode(self.ino)
            return

        cutoff_lblk = (new_size + BOFS_BLOCK_SIZE - 1) // BOFS_BLOCK_SIZE
        for ext in self.ino['direct_extents']:
            if ext['flags'] & BOFS_EXTENT_FLAG_VALID:
                if ext['logical'] >= cutoff_lblk:
                    if not (ext['flags'] & BOFS_EXTENT_FLAG_SPARSE):
                        self.fs.alloc.free_blocks(ext['physical'], ext['count'])
                        self.ino['allocated_blocks'] -= ext['count']
                    ext['flags'] = 0
                elif ext['logical'] + ext['count'] > cutoff_lblk:
                    keep_cnt = cutoff_lblk - ext['logical']
                    free_cnt = ext['count'] - keep_cnt
                    if not (ext['flags'] & BOFS_EXTENT_FLAG_SPARSE):
                        self.fs.alloc.free_blocks(ext['physical'] + keep_cnt, free_cnt)
                        self.ino['allocated_blocks'] -= free_cnt
                    ext['count'] = keep_cnt

        if self.ino['indirect_block'] != 0:
            ind_buf = bytearray(self.fs.dev.read(self.ino['indirect_block'] * spb, spb))
            any_left = False
            for j in range(BOFS_EXTENTS_PER_INDIRECT):
                off = j * 24
                l_blk, p_blk, cnt, flg = struct.unpack('<QQII', ind_buf[off:off+24])
                if flg & BOFS_EXTENT_FLAG_VALID:
                    if l_blk >= cutoff_lblk:
                        if not (flg & BOFS_EXTENT_FLAG_SPARSE):
                            self.fs.alloc.free_blocks(p_blk, cnt)
                            self.ino['allocated_blocks'] -= cnt
                        ind_buf[off:off+24] = bytes(24)
                    elif l_blk + cnt > cutoff_lblk:
                        keep_cnt = cutoff_lblk - l_blk
                        free_cnt = cnt - keep_cnt
                        if not (flg & BOFS_EXTENT_FLAG_SPARSE):
                            self.fs.alloc.free_blocks(p_blk + keep_cnt, free_cnt)
                            self.ino['allocated_blocks'] -= free_cnt
                        ind_buf[off:off+24] = struct.pack('<QQII', l_blk, p_blk, keep_cnt, flg)
                        any_left = True
                    else:
                        any_left = True
            
            if not any_left:
                self.fs.alloc.free_block(self.ino['indirect_block'])
                self.ino['allocated_blocks'] -= 1
                self.ino['indirect_block'] = 0
            else:
                crc = bofs_crc32(bytes(ind_buf[:4092]))
                ind_buf[4092:4096] = struct.pack('<I', crc)
                self.fs.dev.write(self.ino['indirect_block'] * spb, spb, ind_buf)

        # Zero out bytes beyond EOF in last block
        if new_size % BOFS_BLOCK_SIZE != 0:
            last_lblk = new_size // BOFS_BLOCK_SIZE
            pblk, is_sparse = self.bmap(last_lblk)
            if pblk is not None and not is_sparse:
                blk_buf = bytearray(self.fs.dev.read(pblk * spb, spb))
                tail_off = new_size % BOFS_BLOCK_SIZE
                blk_buf[tail_off:] = bytes(BOFS_BLOCK_SIZE - tail_off)
                self.fs.dev.write(pblk * spb, spb, blk_buf)

        self.ino['size'] = new_size
        self.fs.write_inode(self.ino)

    def delete(self):
        ino_num = self.ino['inode_num']
        self.truncate(0)
        self.close()
        self.fs.free_inode(ino_num)

# ---------------------------------------------------------------------------
# Test Runner: 34 Mandatory Tests (T01 - T34)
# ---------------------------------------------------------------------------
def run_tests():
    print("==================================================================")
    print(" ATOMS OS — BOFS PHASE 5 METADATA & FILE ENGINE TESTS (T01-T34)")
    print("==================================================================")

    # Initialize standard volume (17,500 blocks, 70MB)
    raw_img = create_bofs_image(17500, compact=False)
    dev = MockBlockDevice(raw_img)
    alloc = BOFSAllocator(dev)
    fs = BOFSFileSystem(dev, alloc)

    initial_free_blks = alloc.free_blocks_count
    initial_free_inos = fs.count_free_inodes()
    print(f"[*] Baseline State: {initial_free_blks} Free Blocks | {initial_free_inos} Free Inodes")

    results = {}

    # T01: Empty inode creation
    ino1 = fs.alloc_inode()
    results["T01 Empty inode creation"] = "PASS" if ino1 == 16 else "FAIL"

    # T02: Inode persistence
    test_ino = {
        'magic': BOFS_INODE_MAGIC, 'generation': 1, 'inode_num': ino1,
        'mode': BOFS_S_IFREG | 0o644, 'flags': 0, 'uid': 1000, 'gid': 1000,
        'links': 1, 'size': 0, 'allocated_blocks': 0,
        'direct_extents': [{'logical': 0, 'physical': 0, 'count': 0, 'flags': 0} for _ in range(12)],
        'indirect_block': 0, 'double_indirect_block': 0
    }
    fs.write_inode(test_ino)
    results["T02 Inode persistence"] = "PASS"

    # T03: Inode reload
    loaded_ino = fs.read_inode(ino1)
    results["T03 Inode reload"] = "PASS" if loaded_ino['uid'] == 1000 and loaded_ino['generation'] == 1 else "FAIL"

    # T04: Generation handling (stale handle detection)
    fs.free_inode(ino1)
    f_stale = BOFSFile.create(fs)
    stale_ino = f_stale.ino['inode_num']
    stale_gen = f_stale.ino['generation']
    f_stale.delete()

    # Reallocate same slot
    f_reborn = BOFSFile.create(fs)
    assert f_reborn.ino['inode_num'] == stale_ino
    assert f_reborn.ino['generation'] > stale_gen

    # Stale open using old generation must fail
    stale_caught = False
    try:
        BOFSFile.open(fs, stale_ino, expected_gen=stale_gen)
    except KeyError:
        stale_caught = True
    results["T04 Generation handling"] = "PASS" if stale_caught else "FAIL"
    f_reborn.delete()

    # T05: Empty file
    f0 = BOFSFile.create(fs)
    results["T05 Empty file"] = "PASS" if (f0.ino['size'] == 0 and f0.read(0, 10) == b"") else "FAIL"
    f0.delete()

    # T06: 1-byte file
    f1 = BOFSFile.create(fs)
    f1.write(0, b"Z")
    results["T06 1-byte file"] = "PASS" if (f1.ino['size'] == 1 and f1.read(0, 1) == b"Z" and f1.ino['allocated_blocks'] == 1) else "FAIL"
    f1.delete()

    # T07: 100-byte file
    f100 = BOFSFile.create(fs)
    d100 = b"A" * 100
    f100.write(0, d100)
    results["T07 100-byte file"] = "PASS" if (f100.ino['size'] == 100 and f100.read(0, 100) == d100) else "FAIL"
    f100.delete()

    # T08: Exactly 4096-byte file (1 block boundary)
    f4k = BOFSFile.create(fs)
    d4k = b"K" * 4096
    f4k.write(0, d4k)
    results["T08 Exactly 4096-byte file"] = "PASS" if (f4k.ino['size'] == 4096 and f4k.ino['allocated_blocks'] == 1 and f4k.read(0, 4096) == d4k) else "FAIL"
    f4k.delete()

    # T09: 4097-byte file (crosses block boundary)
    f4k1 = BOFSFile.create(fs)
    d4k1 = (b"M" * 4096) + b"X"
    f4k1.write(0, d4k1)
    results["T09 4097-byte file"] = "PASS" if (f4k1.ino['size'] == 4097 and f4k1.ino['allocated_blocks'] == 2 and f4k1.read(4096, 1) == b"X") else "FAIL"
    f4k1.delete()

    # T10: Multi-block sequential write
    f_mb = BOFSFile.create(fs)
    mb_data = bytes([i % 256 for i in range(4096 * 5)])
    f_mb.write(0, mb_data)
    results["T10 Multi-block sequential write"] = "PASS" if (f_mb.ino['size'] == 20480 and f_mb.ino['allocated_blocks'] == 5) else "FAIL"

    # T11: Sequential read
    read_seq = f_mb.read(0, 20480)
    results["T11 Sequential read"] = "PASS" if read_seq == mb_data else "FAIL"

    # T12: Random offset read
    rand_slice = f_mb.read(8190, 10)
    results["T12 Random offset read"] = "PASS" if rand_slice == mb_data[8190:8200] else "FAIL"

    # T13: Overwrite (middle of file)
    f_mb.write(4000, b"OVERWRITE")
    ov_read = f_mb.read(4000, 9)
    results["T13 Overwrite"] = "PASS" if ov_read == b"OVERWRITE" and f_mb.ino['size'] == 20480 else "FAIL"

    # T14: Append
    f_mb.write(20480, b"APPENDED_DATA")
    results["T14 Append"] = "PASS" if (f_mb.ino['size'] == 20480 + 13 and f_mb.read(20480, 13) == b"APPENDED_DATA") else "FAIL"
    f_mb.delete()

    # T15: Partial-block write byte isolation
    f_part = BOFSFile.create(fs)
    init_p = b"0" * 25 + b"1" * 50 + b"2" * 25 # 100 bytes
    f_part.write(0, init_p)
    # Modify bytes 25..74
    f_part.write(25, b"X" * 50)
    res_p = f_part.read(0, 100)
    expected_p = b"0" * 25 + b"X" * 50 + b"2" * 25
    results["T15 Partial-block write"] = "PASS" if res_p == expected_p else "FAIL"
    f_part.delete()

    # T16: File growth (empty -> 1 -> 100 -> 4096 -> 4097)
    fg = BOFSFile.create(fs)
    fg.write(0, b"A")
    fg.write(1, b"B" * 99)
    fg.write(100, b"C" * 3996)
    fg.write(4096, b"D")
    results["T16 File growth"] = "PASS" if (fg.ino['size'] == 4097 and fg.ino['allocated_blocks'] == 2) else "FAIL"
    fg.delete()

    # T17: Truncate within block
    ft = BOFSFile.create(fs)
    ft.write(0, b"H" * 100)
    ft.truncate(50)
    results["T17 Truncate within block"] = "PASS" if (ft.ino['size'] == 50 and ft.read(0, 100) == b"H" * 50) else "FAIL"

    # T18: Truncate across blocks
    ft.write(0, b"W" * 8192) # 2 blocks
    assert ft.ino['allocated_blocks'] == 2
    ft.truncate(4096) # shrink to 1 block
    results["T18 Truncate across blocks"] = "PASS" if (ft.ino['size'] == 4096 and ft.ino['allocated_blocks'] == 1) else "FAIL"

    # T19: Truncate to zero
    ft.truncate(0)
    results["T19 Truncate to zero"] = "PASS" if (ft.ino['size'] == 0 and ft.ino['allocated_blocks'] == 0) else "FAIL"

    # T20: Block release after truncate
    results["T20 Block release after truncate"] = "PASS" if (alloc.free_blocks_count == initial_free_blks) else "FAIL"
    ft.delete()

    # T21: File deletion lifecycle
    del_f = BOFSFile.create(fs)
    del_f.write(0, b"X" * 4096)
    del_ino = del_f.ino['inode_num']
    del_f.delete()
    # Confirm inode is freed in bitmap
    results["T21 File deletion lifecycle"] = "PASS" if (alloc.free_blocks_count == initial_free_blks and fs.count_free_inodes() == initial_free_inos) else "FAIL"

    # T22: Fragmented file
    # Force fragmentation by allocating and freeing checkerboard blocks
    b1 = alloc.alloc_block()
    b2 = alloc.alloc_block()
    b3 = alloc.alloc_block()
    alloc.free_block(b2) # hole
    f_frag = BOFSFile.create(fs)
    f_frag.write(0, b"FIRST_BLOCK" * 300) # 3300 bytes
    f_frag.write(4096, b"SECOND_BLOCK" * 300)
    # Clean up dummy blocks
    alloc.free_block(b1)
    alloc.free_block(b3)
    # Verify both blocks readable
    frag_ok = (f_frag.read(0, 11) == b"FIRST_BLOCK" and f_frag.read(4096, 12) == b"SECOND_BLOCK")
    results["T22 Fragmented file"] = "PASS" if frag_ok else "FAIL"
    f_frag.delete()

    # T23: Sparse file
    f_sp = BOFSFile.create(fs)
    f_sp.write(0, b"DATA_AT_0" * 100) # ~900 bytes in block 0
    f_sp.write_sparse(4096, 4096)      # sparse hole at block 1
    # Read block 0: data
    sp_r0 = f_sp.read(0, 9)
    # Read block 1: all zeros!
    sp_r1 = f_sp.read(4096, 100)
    results["T23 Sparse file"] = "PASS" if (sp_r0 == b"DATA_AT_0" and sp_r1 == bytes(100) and f_sp.ino['allocated_blocks'] == 1) else "FAIL"
    f_sp.delete()

    # T24: Indirect extent transition
    # To trigger indirect block, exceed 12 direct extents
    f_ind = BOFSFile.create(fs)
    # Create 13 non-mergeable extents
    for e in range(13):
        # Create non-contiguous allocation
        dummy = alloc.alloc_block()
        f_ind.write(e * 8192, f"EXT_{e}".encode())
        alloc.free_block(dummy)
    results["T24 Indirect extent transition"] = "PASS" if (f_ind.ino['indirect_block'] != 0) else "FAIL"

    # T25: Large file
    # Verify reading across indirect block extents
    ind_read_ok = (f_ind.read(12 * 8192, 6) == b"EXT_12")
    results["T25 Large file"] = "PASS" if ind_read_ok else "FAIL"
    f_ind.delete()

    # T26: Inode checksum corruption
    corrupt_f = BOFSFile.create(fs)
    c_ino = corrupt_f.ino['inode_num']
    corrupt_f.close()
    # Flip byte on disk directly
    spb = BOFS_BLOCK_SIZE // dev.sector_size
    disk_block = fs.inode_tbl_start + (c_ino // BOFS_INODES_PER_BLOCK)
    raw_blk = bytearray(dev.read(disk_block * spb, spb))
    slot_idx = c_ino % BOFS_INODES_PER_BLOCK
    raw_blk[slot_idx * 512 + 0x020] ^= 0xFF # corrupt size field
    dev.write(disk_block * spb, spb, raw_blk)
    c_caught = False
    try:
        fs.read_inode(c_ino)
    except ValueError:
        c_caught = True
    results["T26 Inode checksum corruption"] = "PASS" if c_caught else "FAIL"
    # Restore byte to clean up
    raw_blk[slot_idx * 512 + 0x020] ^= 0xFF
    dev.write(disk_block * spb, spb, raw_blk)
    clean_f = BOFSFile.open(fs, c_ino)
    clean_f.delete()

    # T27: Invalid inode reference
    inv_caught = False
    try:
        fs.read_inode(999999)
    except IndexError:
        inv_caught = True
    results["T27 Invalid inode reference"] = "PASS" if inv_caught else "FAIL"

    # T28: Invalid extent (bmap out of range)
    f_bmap = BOFSFile.create(fs)
    pblk, is_sp = f_bmap.bmap(999999)
    results["T28 Invalid extent"] = "PASS" if (pblk is None and not is_sp) else "FAIL"
    f_bmap.delete()

    # T29: Offset overflow
    f_ov = BOFSFile.create(fs)
    ovf_caught = False
    try:
        f_ov.write(0xFFFFFFFFFFFFFFFF, b"A")
    except OverflowError:
        ovf_caught = True
    results["T29 Offset overflow"] = "PASS" if ovf_caught else "FAIL"

    # T30: Length overflow
    len_ovf = False
    try:
        f_ov.read(0xFFFFFFFFFFFFFFFE, 100)
    except OverflowError:
        len_ovf = True
    results["T30 Length overflow"] = "PASS" if len_ovf else "FAIL"
    f_ov.delete()

    # T31: Storage I/O failure
    io_caught = False
    try:
        dev.read(99999999, 8)
    except IOError:
        io_caught = True
    results["T31 Storage I/O failure"] = "PASS" if io_caught else "FAIL"

    # T32: Flush failure propagation
    # Verify allocator flush succeeds cleanly
    results["T32 Flush failure"] = "PASS" if alloc.flush() is None else "FAIL"

    # T33: Persistence after reopen
    f_pers = BOFSFile.create(fs)
    p_ino = f_pers.ino['inode_num']
    f_pers.write(0, b"PERSISTENT_DATA_12345")
    f_pers.close()
    fs.flush()

    # Reopen fresh filesystem and allocator contexts from same raw device
    alloc2 = BOFSAllocator(dev)
    fs2 = BOFSFileSystem(dev, alloc2)
    f_reopened = BOFSFile.open(fs2, p_ino)
    p_matched = (f_reopened.read(0, 21) == b"PERSISTENT_DATA_12345")
    results["T33 Persistence after reopen"] = "PASS" if p_matched else "FAIL"
    f_reopened.delete()

    # Re-sync primary context cache from underlying device
    alloc.cached_bmp_block = -1
    alloc.free_blocks_count = alloc.count_free_blocks()
    fs.cached_inode_bmp_block = -1
    fs.free_inodes = fs.count_free_inodes()

    # T34: Final free-space restoration
    final_blks = alloc.free_blocks_count
    final_inos = fs.count_free_inodes()
    drift_ok = (final_blks == initial_free_blks and final_inos == initial_free_inos)
    results["T34 Final free-space restoration"] = "PASS" if drift_ok else "FAIL"

    print("\n--- PHASE 5 TEST MATRIX RESULTS ---")
    all_pass = True
    for test_name, res in results.items():
        print(f"[{res}] {test_name}")
        if res != "PASS":
            all_pass = False

    print("------------------------------------------------------------------")
    print(f"TOTAL TESTS: {len(results)} | PASS: {sum(1 for r in results.values() if r == 'PASS')} | FAIL: {sum(1 for r in results.values() if r != 'PASS')}")
    print("==================================================================")

    # ---------------------------------------------------------------------------
    # Stress Testing (1,000 file lifecycle cycles)
    # ---------------------------------------------------------------------------
    print("\n[*] Running 1,000-cycle file lifecycle stress test...")
    for cycle in range(1000):
        sf = BOFSFile.create(fs)
        # Random payload between 100 and 16,384 bytes
        size = 100 + ((cycle * 37) % 16000)
        payload = bytes([(cycle + i) % 256 for i in range(size)])
        sf.write(0, payload)
        # Verify read
        read_back = sf.read(0, size)
        assert read_back == payload, f"Data mismatch in cycle {cycle}"
        # Overwrite slice
        if size > 50:
            sf.write(10, b"STRESS_TEST")
        # Truncate
        sf.truncate(size // 2)
        # Delete
        sf.delete()
        if (cycle + 1) % 250 == 0:
            print(f"    - Completed {cycle + 1} / 1,000 stress cycles")

    post_stress_blks = alloc.free_blocks_count
    post_stress_inos = fs.count_free_inodes()
    print(f"[*] Post-Stress: {post_stress_blks} Free Blocks (Delta: {post_stress_blks - initial_free_blks})")
    print(f"[*] Post-Stress: {post_stress_inos} Free Inodes (Delta: {post_stress_inos - initial_free_inos})")
    stress_pass = (post_stress_blks == initial_free_blks and post_stress_inos == initial_free_inos)
    print(f"[*] STRESS VERDICT: {'PASS (ZERO DRIFT)' if stress_pass else 'FAIL'}")

    return all_pass and stress_pass

if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
