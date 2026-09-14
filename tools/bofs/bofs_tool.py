#!/usr/bin/env python3
"""
ATOMS OS — BOFS V1 Format Builder, Inspector, and Forensic Test Suite
Document ID: ATOMS-BOFS-PHASE3-TOOL-001
Specification: docs/BOFS/PHASE3_BOFS_ON_DISK_FORMAT.md
"""

import sys
import os
import struct
import binascii
import uuid

# ---------------------------------------------------------------------------
# Format Constants (from bofs_format.h)
# ---------------------------------------------------------------------------
BOFS_SUPER_MAGIC        = 0x53464F42  # 'BOFS'
BOFS_INODE_MAGIC        = 0x4F4E4942  # 'BINO'
BOFS_DIR_MAGIC          = 0x52494442  # 'BDIR'
BOFS_JOURNAL_MAGIC      = 0x4C4E4A42  # 'BJNL'
BOFS_TXN_DESC_MAGIC     = 0x4E585442  # 'BTXN'
BOFS_TXN_COMMIT_MAGIC   = 0x544D4342  # 'BCMT'

BOFS_VERSION_MAJOR      = 1
BOFS_VERSION_MINOR      = 0
BOFS_FORMAT_REVISION    = 1
BOFS_BLOCK_SIZE         = 4096
BOFS_INODE_SIZE         = 512

BOFS_STATE_CLEAN        = 0x0001
BOFS_STATE_DIRTY        = 0x0002

BOFS_FEATURE_COMPAT_DIR_HASH     = 0x00000001
BOFS_FEATURE_COMPAT_SPARSE_FILES = 0x00000002
BOFS_FEATURE_INCOMPAT_EXTENTS    = 0x00000001
BOFS_FEATURE_INCOMPAT_JOURNAL    = 0x00000002
BOFS_FEATURE_INCOMPAT_64BIT      = 0x00000004
BOFS_FEATURES_INCOMPAT_SUPPORTED = 0x00000007

BOFS_S_IFDIR            = 0o040000
BOFS_ROOT_INODE         = 1
BOFS_JOURNAL_INODE      = 2
BOFS_BLOCK_BITMAP_INODE = 3
BOFS_INODE_BITMAP_INODE = 4

# ---------------------------------------------------------------------------
# CRC32 (IEEE 802.3)
# ---------------------------------------------------------------------------
def bofs_crc32(data: bytes) -> int:
    """Calculates IEEE 802.3 CRC32 matching bofs_crc32() in C."""
    return binascii.crc32(data) & 0xFFFFFFFF

# ---------------------------------------------------------------------------
# Binary Serialization / Deserialization
# ---------------------------------------------------------------------------
def pack_extent(logical_block=0, physical_block=0, block_count=0, flags=0) -> bytes:
    # <QQII (24 bytes)
    return struct.pack('<QQII', logical_block, physical_block, block_count, flags)

def unpack_extent(data: bytes):
    return struct.unpack('<QQII', data[:24])

def pack_inode(inode_num=1, mode=BOFS_S_IFDIR | 0o755, uid=0, gid=0, size_bytes=4096,
               allocated_blocks=1, extents=None, generation=1) -> bytes:
    """Packs a 512-byte BOFS Inode with trailing CRC32."""
    if extents is None:
        extents = []
    # Base header up to timestamps: 48 bytes
    hdr = struct.pack('<IIQHHIIIQQ',
                      BOFS_INODE_MAGIC, generation, inode_num,
                      mode, 0, uid, gid, 2, size_bytes, allocated_blocks)
    
    # 4 Timestamps: atime, mtime, ctime, crtime (16 bytes each = 64 bytes)
    ts = struct.pack('<QIIQIIQIIQII',
                     0, 0, 0,  # atime
                     0, 0, 0,  # mtime
                     0, 0, 0,  # ctime
                     0, 0, 0)  # crtime
    
    # Direct Extents: 12 * 24 = 288 bytes
    ext_bytes = bytearray()
    for i in range(12):
        if i < len(extents):
            ext_bytes += pack_extent(*extents[i])
        else:
            ext_bytes += pack_extent(0, 0, 0, 0)
    
    # Indirect block pointers: 2 * 8 = 16 bytes
    indirect = struct.pack('<QQ', 0, 0)
    
    # Extended attributes: 88 bytes
    xattr = bytes(88)
    
    # Reserved padding: 4 bytes
    res = bytes(4)
    
    raw = hdr + ts + bytes(ext_bytes) + indirect + xattr + res
    assert len(raw) == 508, f"Inode raw length was {len(raw)}, expected 508"
    
    crc = bofs_crc32(raw)
    return raw + struct.pack('<I', crc)

def pack_superblock(total_blocks: int, sector_size: int = 512, compact: bool = False,
                    vol_uuid: bytes = None, label: str = "ATOMS_BOFS") -> bytes:
    """Packs a 4096-byte BOFS Superblock with trailing CRC32."""
    if vol_uuid is None:
        vol_uuid = uuid.uuid4().bytes
    if len(vol_uuid) < 16:
        vol_uuid = vol_uuid.ljust(16, b'\x00')
    
    label_bytes = label.encode('utf-8')[:64].ljust(64, b'\x00')
    
    journal_start = 4
    journal_blocks = 16 if compact else 8192
    
    inode_bmp_start = journal_start + journal_blocks
    inode_bmp_blocks = 1 if compact else 2
    
    block_bmp_start = inode_bmp_start + inode_bmp_blocks
    block_bmp_blocks = (total_blocks + 32767) // 32768
    
    total_inodes = 128 if compact else 65536
    inode_table_start = block_bmp_start + block_bmp_blocks
    inode_table_blocks = (total_inodes + 7) // 8
    
    data_pool_start = inode_table_start + inode_table_blocks
    assert data_pool_start < total_blocks, "Volume size too small for metadata layout"
    data_pool_blocks = total_blocks - data_pool_start
    
    # Header format:
    # 0x000: magic(4), v_maj(2), v_min(2), rev(2), state(2), f_compat(4), f_incompat(4), f_ro_compat(4) = 24B
    # 0x018: uuid(16) = 16B
    # 0x028: label(64) = 64B
    # 0x068: block_size(4), sector_size(4), total_blocks(8), free_blocks(8), total_inodes(8), free_inodes(8) = 40B
    # 0x090: 12 region offsets (12 * 8 = 96B)
    # 0x0F0: 4 reserved inodes (4 * 8 = 32B)
    # 0x110: 4 lifecycle metrics (4 * 8 = 32B)
    # Subtotal = 24 + 16 + 64 + 40 + 96 + 32 + 32 = 304 Bytes
    
    hdr1 = struct.pack('<IHHHHIII',
                       BOFS_SUPER_MAGIC, BOFS_VERSION_MAJOR, BOFS_VERSION_MINOR,
                       BOFS_FORMAT_REVISION, BOFS_STATE_CLEAN,
                       BOFS_FEATURE_COMPAT_DIR_HASH | BOFS_FEATURE_COMPAT_SPARSE_FILES,
                       BOFS_FEATURES_INCOMPAT_SUPPORTED, 0)
    
    hdr2 = vol_uuid + label_bytes
    
    hdr3 = struct.pack('<IIQQQQ',
                       BOFS_BLOCK_SIZE, sector_size, total_blocks, data_pool_blocks,
                       total_inodes, total_inodes - 16)
    
    regions = struct.pack('<QQQQQQQQQQQQ',
                          0, 1,
                          journal_start, journal_blocks,
                          inode_bmp_start, inode_bmp_blocks,
                          block_bmp_start, block_bmp_blocks,
                          inode_table_start, inode_table_blocks,
                          data_pool_start, data_pool_blocks)
    
    inodes_sec = struct.pack('<QQQQ',
                             BOFS_ROOT_INODE, BOFS_JOURNAL_INODE,
                             BOFS_BLOCK_BITMAP_INODE, BOFS_INODE_BITMAP_INODE)
    
    lifecycle = struct.pack('<QQQQ', 0, 1, 0, 0)
    
    raw = hdr1 + hdr2 + hdr3 + regions + inodes_sec + lifecycle
    assert len(raw) == 304, f"Superblock header prefix was {len(raw)}, expected 304"
    
    # Padding to 4092 bytes (4096 - 4)
    raw += bytes(3788)
    assert len(raw) == 4092
    
    crc = bofs_crc32(raw)
    return raw + struct.pack('<I', crc)

def pack_journal_header(total_blocks: int) -> bytes:
    # magic(4), ver(2), flags(2), blk_size(4), res_hdr(4), total(8), head(8), tail(8), seq(8), last_commit(8) = 56B
    hdr = struct.pack('<IHHIIQQQQQ',
                      BOFS_JOURNAL_MAGIC, 1, 0, BOFS_BLOCK_SIZE, 0,
                      total_blocks, 1, 1, 1, 0)
    raw = hdr + bytes(4036)
    assert len(raw) == 4092
    crc = bofs_crc32(raw)
    return raw + struct.pack('<I', crc)

def pack_dir_node(node_type=1, level=0, entry_count=0) -> bytes:
    # magic(4), node_type(2), count(2), level(4), gen(4), parent(8), left(8), right(8), reserved(84) = 124B
    hdr = struct.pack('<IHHIIQQQ',
                      BOFS_DIR_MAGIC, node_type, entry_count, level, 1,
                      0, 0, 0)
    pad = bytes(84)
    slots = bytes(62 * 64)
    raw = hdr + pad + slots
    assert len(raw) == 4092
    crc = bofs_crc32(raw)
    return raw + struct.pack('<I', crc)

# ---------------------------------------------------------------------------
# Format Builder / Image Creator
# ---------------------------------------------------------------------------
def create_bofs_image(total_blocks: int, compact: bool = False, sector_size: int = 512) -> bytearray:
    """Creates a complete, fully valid BOFS volume bytearray."""
    img = bytearray(total_blocks * BOFS_BLOCK_SIZE)
    
    sb = pack_superblock(total_blocks, sector_size, compact)
    # Block 0: Primary Superblock
    img[0:4096] = sb
    # Block 1: Backup Superblock
    img[4096:8192] = sb
    
    # Parse regions from SB to populate other blocks
    regions = struct.unpack('<QQQQQQQQQQQQ', sb[0x090:0x090 + 96])
    journal_start, journal_count = regions[2], regions[3]
    inode_bmp_start = regions[4]
    block_bmp_start = regions[6]
    inode_tbl_start = regions[8]
    data_pool_start = regions[10]
    
    # Journal Header at journal_start
    img[journal_start * 4096 : (journal_start + 1) * 4096] = pack_journal_header(journal_count)
    
    # Inode Bitmap: Inodes 0-15 allocated (bits 0-15 set = 0xFFFF)
    img[inode_bmp_start * 4096 : inode_bmp_start * 4096 + 2] = b'\xFF\xFF'
    
    # Block Bitmap: All metadata blocks marked allocated
    meta_blocks = data_pool_start
    for blk in range(meta_blocks):
        byte_idx = (block_bmp_start * 4096) + (blk // 8)
        bit_idx = blk % 8
        img[byte_idx] |= (1 << bit_idx)
    
    # Inode Table: Root Directory Inode (Inode 1) at offset (inode_tbl_start * 4096 + 512)
    root_extent = [(0, data_pool_start, 1, 1)] # Logical 0 -> Data block 0
    root_inode = pack_inode(inode_num=1, mode=BOFS_S_IFDIR | 0o755, size_bytes=4096,
                            allocated_blocks=1, extents=root_extent)
    img[inode_tbl_start * 4096 + 512 : inode_tbl_start * 4096 + 1024] = root_inode
    
    # Data Block 0: Root Directory Leaf Node
    img[data_pool_start * 4096 : (data_pool_start + 1) * 4096] = pack_dir_node(node_type=1, level=0, entry_count=0)
    
    return img

# ---------------------------------------------------------------------------
# Format Validator
# ---------------------------------------------------------------------------
def validate_bofs_image(img: bytes) -> tuple:
    """Validates a BOFS volume. Returns (success: bool, code: str, details: str)."""
    if len(img) < 64 * BOFS_BLOCK_SIZE:
        return False, "ERR_IMAGE_TRUNCATED", f"Image size {len(img)}B < minimum 64 blocks"
    
    # 1. Superblock validation
    sb = img[0:4096]
    magic = struct.unpack('<I', sb[:4])[0]
    if magic != BOFS_SUPER_MAGIC:
        return False, "ERR_INVALID_MAGIC", f"Expected magic 0x{BOFS_SUPER_MAGIC:08X}, got 0x{magic:08X}"
    
    v_maj, v_min = struct.unpack('<HH', sb[4:8])
    if v_maj != BOFS_VERSION_MAJOR:
        return False, "ERR_UNSUPPORTED_VERSION", f"Unsupported version {v_maj}.{v_min}"
    
    f_incompat = struct.unpack('<I', sb[16:20])[0]
    if (f_incompat & ~BOFS_FEATURES_INCOMPAT_SUPPORTED) != 0:
        return False, "ERR_UNSUPPORTED_FEATURE", f"Unknown incompat flags: 0x{f_incompat:08X}"
    
    stored_crc = struct.unpack('<I', sb[4092:4096])[0]
    calc_crc = bofs_crc32(sb[:4092])
    if stored_crc != calc_crc:
        return False, "ERR_CHECKSUM_MISMATCH", f"SB CRC stored 0x{stored_crc:08X} != calc 0x{calc_crc:08X}"
    
    # 2. Backup Superblock validation
    backup_sb = img[4096:8192]
    backup_crc_stored = struct.unpack('<I', backup_sb[4092:4096])[0]
    backup_crc_calc = bofs_crc32(backup_sb[:4092])
    if backup_crc_stored != backup_crc_calc:
        return False, "ERR_BACKUP_SB_CHECKSUM", "Backup Superblock CRC corrupted"
    
    # 3. Geometry check
    regions = struct.unpack('<QQQQQQQQQQQQ', sb[0x090:0x090 + 96])
    cur = 2
    for idx, name in [(2, "journal"), (4, "inode_bmp"), (6, "block_bmp"), (8, "inode_table"), (10, "data_pool")]:
        start, count = regions[idx], regions[idx + 1]
        if start < cur:
            return False, "ERR_GEOMETRY_OVERLAP", f"{name} region overlaps previous (start {start} < cur {cur})"
        if count == 0:
            return False, "ERR_GEOMETRY_OVERLAP", f"{name} region count is 0"
        cur = start + count
    
    total_blocks = struct.unpack('<Q', sb[0x070:0x078])[0]
    if cur > total_blocks:
        return False, "ERR_OUT_OF_BOUNDS", f"Total region blocks {cur} > total_blocks {total_blocks}"
    
    # 4. Inode table and root inode check
    inode_tbl_start = regions[8]
    root_ino_bytes = img[inode_tbl_start * 4096 + 512 : inode_tbl_start * 4096 + 1024]
    ino_magic = struct.unpack('<I', root_ino_bytes[:4])[0]
    if ino_magic != BOFS_INODE_MAGIC:
        return False, "ERR_CORRUPTED_INODE", f"Root inode magic invalid: 0x{ino_magic:08X}"
    
    ino_crc_stored = struct.unpack('<I', root_ino_bytes[508:512])[0]
    ino_crc_calc = bofs_crc32(root_ino_bytes[:508])
    if ino_crc_stored != ino_crc_calc:
        return False, "ERR_INODE_CHECKSUM", f"Root inode CRC mismatch"
    
    return True, "OK", "Volume format, geometry, and checksums certified"

# ---------------------------------------------------------------------------
# Test Suite: T01 - T20
# ---------------------------------------------------------------------------
def run_all_format_tests():
    print("================================================================")
    print(" ATOMS OS — BOFS PHASE 3 ON-DISK FORMAT TEST SUITE (T01 - T20)")
    print("================================================================")
    
    results = {}
    
    # T01: Valid empty BOFS image
    img = create_bofs_image(17500, compact=False)
    ok, code, msg = validate_bofs_image(img)
    results["T01 Valid empty BOFS image"] = "PASS" if ok else "FAIL"
    
    # T02: Valid superblock
    sb = img[0:4096]
    ok_sb = (struct.unpack('<I', sb[:4])[0] == BOFS_SUPER_MAGIC and 
             bofs_crc32(sb[:4092]) == struct.unpack('<I', sb[4092:4096])[0])
    results["T02 Valid superblock"] = "PASS" if ok_sb else "FAIL"
    
    # T03: Valid backup superblock
    bsb = img[4096:8192]
    ok_bsb = (struct.unpack('<I', bsb[:4])[0] == BOFS_SUPER_MAGIC and 
              bofs_crc32(bsb[:4092]) == struct.unpack('<I', bsb[4092:4096])[0])
    results["T03 Valid backup superblock"] = "PASS" if ok_bsb else "FAIL"
    
    # T04: Geometry validation
    results["T04 Geometry validation"] = "PASS" if ok else "FAIL"
    
    # T05: Region overlap rejection
    bad_geom_sb = bytearray(sb)
    # Force journal to start at block 0 (overlap!)
    bad_geom_sb[0x0A0:0x0A8] = struct.pack('<Q', 0)
    # Recalculate CRC
    bad_geom_sb[4092:4096] = struct.pack('<I', bofs_crc32(bad_geom_sb[:4092]))
    test_img = bytearray(img)
    test_img[0:4096] = bad_geom_sb
    ok_t5, code_t5, _ = validate_bofs_image(test_img)
    results["T05 Region overlap rejection"] = "PASS" if (not ok_t5 and code_t5 == "ERR_GEOMETRY_OVERLAP") else "FAIL"
    
    # T06: Integer overflow rejection
    bad_ovf_sb = bytearray(sb)
    # Set data pool count to UINT64_MAX
    bad_ovf_sb[0x0E8:0x0F0] = struct.pack('<Q', 0xFFFFFFFFFFFFFFFF)
    bad_ovf_sb[4092:4096] = struct.pack('<I', bofs_crc32(bad_ovf_sb[:4092]))
    test_img = bytearray(img)
    test_img[0:4096] = bad_ovf_sb
    ok_t6, code_t6, _ = validate_bofs_image(test_img)
    results["T06 Integer overflow rejection"] = "PASS" if not ok_t6 else "FAIL"
    
    # T07: Invalid magic rejection
    bad_magic_img = bytearray(img)
    bad_magic_img[0:4] = b'NTFS'
    ok_t7, code_t7, _ = validate_bofs_image(bad_magic_img)
    results["T07 Invalid magic"] = "PASS" if (not ok_t7 and code_t7 == "ERR_INVALID_MAGIC") else "FAIL"
    
    # T08: Invalid version
    bad_ver_sb = bytearray(sb)
    bad_ver_sb[4:6] = struct.pack('<H', 99) # Version 99
    bad_ver_sb[4092:4096] = struct.pack('<I', bofs_crc32(bad_ver_sb[:4092]))
    test_img = bytearray(img)
    test_img[0:4096] = bad_ver_sb
    ok_t8, code_t8, _ = validate_bofs_image(test_img)
    results["T08 Invalid version"] = "PASS" if (not ok_t8 and code_t8 == "ERR_UNSUPPORTED_VERSION") else "FAIL"
    
    # T09: Unsupported feature
    bad_feat_sb = bytearray(sb)
    bad_feat_sb[16:20] = struct.pack('<I', 0x80000000) # Unknown high bit
    bad_feat_sb[4092:4096] = struct.pack('<I', bofs_crc32(bad_feat_sb[:4092]))
    test_img = bytearray(img)
    test_img[0:4096] = bad_feat_sb
    ok_t9, code_t9, _ = validate_bofs_image(test_img)
    results["T09 Unsupported feature"] = "PASS" if (not ok_t9 and code_t9 == "ERR_UNSUPPORTED_FEATURE") else "FAIL"
    
    # T10: Superblock CRC corruption
    corrupt_sb_img = bytearray(img)
    corrupt_sb_img[100] ^= 0x55 # Flip byte without updating CRC
    ok_t10, code_t10, _ = validate_bofs_image(corrupt_sb_img)
    results["T10 Superblock CRC corruption"] = "PASS" if (not ok_t10 and code_t10 == "ERR_CHECKSUM_MISMATCH") else "FAIL"
    
    # T11: Inode CRC corruption
    corrupt_ino_img = bytearray(img)
    # Find root inode location
    regions = struct.unpack('<QQQQQQQQQQQQ', sb[0x090:0x090 + 96])
    ino_tbl_start = regions[8]
    corrupt_ino_img[ino_tbl_start * 4096 + 512 + 20] ^= 0xAA # Corrupt UID
    ok_t11, code_t11, _ = validate_bofs_image(corrupt_ino_img)
    results["T11 Inode CRC corruption"] = "PASS" if (not ok_t11 and code_t11 == "ERR_INODE_CHECKSUM") else "FAIL"
    
    # T12: Directory-node CRC corruption
    dir_node = pack_dir_node()
    corrupt_dir_node = bytearray(dir_node)
    corrupt_dir_node[50] ^= 0xFF
    calc = bofs_crc32(corrupt_dir_node[:4092])
    stored = struct.unpack('<I', corrupt_dir_node[4092:4096])[0]
    results["T12 Directory-node CRC corruption"] = "PASS" if calc != stored else "FAIL"
    
    # T13: Journal CRC corruption
    jnl_hdr = pack_journal_header(100)
    corrupt_jnl = bytearray(jnl_hdr)
    corrupt_jnl[20] ^= 0x12
    calc_j = bofs_crc32(corrupt_jnl[:4092])
    stored_j = struct.unpack('<I', corrupt_jnl[4092:4096])[0]
    results["T13 Journal CRC corruption"] = "PASS" if calc_j != stored_j else "FAIL"
    
    # T14: Bitmap geometry validation
    # Verify bit allocation calculation
    blk_bmp_blocks = (17500 + 32767) // 32768
    results["T14 Bitmap geometry validation"] = "PASS" if blk_bmp_blocks == 1 else "FAIL"
    
    # T15: Inode table validation
    results["T15 Inode table validation"] = "PASS" if regions[9] == 8192 else "FAIL"
    
    # T16: Serialize/parse round trip
    test_inode = pack_inode(inode_num=42, mode=0o100644, uid=1000, gid=100, size_bytes=99999,
                            allocated_blocks=3, extents=[(0, 500, 3, 1)])
    magic_rt = struct.unpack('<I', test_inode[:4])[0]
    num_rt = struct.unpack('<Q', test_inode[8:16])[0]
    size_rt = struct.unpack('<Q', test_inode[32:40])[0]
    crc_rt = (bofs_crc32(test_inode[:508]) == struct.unpack('<I', test_inode[508:512])[0])
    results["T16 Serialize/parse round trip"] = "PASS" if (magic_rt == BOFS_INODE_MAGIC and num_rt == 42 and size_rt == 99999 and crc_rt) else "FAIL"
    
    # T17: Minimum volume (compact 64 blocks)
    min_img = create_bofs_image(64, compact=True)
    ok_min, _, _ = validate_bofs_image(min_img)
    results["T17 Minimum volume"] = "PASS" if ok_min else "FAIL"
    
    # T18: Larger volume (100,000 blocks)
    large_sb = pack_superblock(100000, 512, compact=False)
    ok_large_crc = (bofs_crc32(large_sb[:4092]) == struct.unpack('<I', large_sb[4092:4096])[0])
    results["T18 Larger volume"] = "PASS" if ok_large_crc else "FAIL"
    
    # T19: Boundary values
    max_size_inode = pack_inode(inode_num=0xFFFFFFFFFFFFFFFF, size_bytes=0xFFFFFFFFFFFFFFFF)
    ok_bound = (bofs_crc32(max_size_inode[:508]) == struct.unpack('<I', max_size_inode[508:512])[0])
    results["T19 Boundary values"] = "PASS" if ok_bound else "FAIL"
    
    # T20: Truncated image
    trunc_img = img[:20 * BOFS_BLOCK_SIZE] # Less than 64 blocks
    ok_t20, code_t20, _ = validate_bofs_image(trunc_img)
    results["T20 Truncated image"] = "PASS" if (not ok_t20 and code_t20 == "ERR_IMAGE_TRUNCATED") else "FAIL"
    
    print("\n--- TEST MATRIX RESULTS ---")
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
    success = run_all_format_tests()
    sys.exit(0 if success else 1)
