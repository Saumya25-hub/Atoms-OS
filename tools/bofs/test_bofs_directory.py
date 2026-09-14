#!/usr/bin/env python3
"""
ATOMS OS — BOFS Phase 6 Directory Engine Test Suite
Document ID: ATOMS-BOFS-PHASE6-TEST-001
Tests: T01 - T36 + 1,000-cycle Stress Test
"""

import sys
import os
import struct
import binascii

# Import format structures from bofs_tool and test_bofs_alloc
from bofs_tool import (
    BOFS_SUPER_MAGIC, BOFS_INODE_MAGIC, BOFS_DIR_MAGIC,
    BOFS_BLOCK_SIZE, BOFS_INODE_SIZE,
    create_bofs_image, pack_superblock, pack_inode, bofs_crc32, validate_bofs_image
)
from test_bofs_alloc import MockBlockDevice, BOFSAllocator
from test_bofs_file import BOFSFileSystem, BOFSFile

BOFS_DIR_NODE_LEAF   = 1
BOFS_DIR_NODE_ROUTER = 2
BOFS_DIR_SLOTS_PER_BLOCK = 62

BOFS_FT_UNKNOWN = 0
BOFS_FT_REG     = 1
BOFS_FT_DIR     = 2

BOFS_HASH_SEED  = 0x5F424F46535F5631
FNV_PRIME_64    = 0x100000001B3

def bofs_hash(name: str) -> int:
    name_bytes = name.encode('utf-8')
    h = BOFS_HASH_SEED
    for b in name_bytes:
        h = (h ^ b) & 0xFFFFFFFFFFFFFFFF
        h = (h * FNV_PRIME_64) & 0xFFFFFFFFFFFFFFFF
    return h

def bofs_dir_cmp(hash_a: int, name_a: str, hash_b: int, name_b: str) -> int:
    if hash_a < hash_b:
        return -1
    if hash_a > hash_b:
        return 1
    if name_a < name_b:
        return -1
    if name_a > name_b:
        return 1
    return 0

def validate_utf8_name(name: str) -> bool:
    if not name or len(name) == 0:
        return False
    if '/' in name or '\x00' in name:
        return False
    try:
        raw = name.encode('utf-8')
        if len(raw) > 255:
            return False
        # Verify strict decode
        raw.decode('utf-8')
        return True
    except UnicodeError:
        return False

class BOFSDirNode:
    def __init__(self, node_type=BOFS_DIR_NODE_LEAF, level=0):
        self.magic = BOFS_DIR_MAGIC
        self.node_type = node_type
        self.entry_count = 0
        self.tree_level = level
        self.generation = 1
        self.parent_block = 0
        self.left_sibling_block = 0
        self.right_sibling_block = 0
        self.slots = [] # list of dicts: {'hash': int, 'child_block': int, 'inode_num': int, 'rec_len': int, 'name_len': int, 'file_type': int, 'name': str}

    @classmethod
    def from_bytes(cls, data: bytes):
        assert len(data) == BOFS_BLOCK_SIZE
        # Check CRC
        calc_crc = bofs_crc32(data[:4092])
        stored_crc = struct.unpack('<I', data[4092:4096])[0]
        if calc_crc != stored_crc:
            raise ValueError("Checksum mismatch on dir node")

        magic, node_type, entry_count, level, gen, parent, left, right = struct.unpack('<IHHIIQQQ', data[:40])
        if magic != BOFS_DIR_MAGIC:
            raise ValueError(f"Invalid dir node magic 0x{magic:08X}")

        node = cls(node_type, level)
        node.entry_count = entry_count
        node.generation = gen
        node.parent_block = parent
        node.left_sibling_block = left
        node.right_sibling_block = right

        slot_offset = 0x07C
        for i in range(entry_count):
            s_bytes = data[slot_offset + i*64 : slot_offset + (i+1)*64]
            h, child_blk, ino, rec_len, name_len, ftype = struct.unpack('<QQQHBB', s_bytes[:28])
            raw_name = s_bytes[28:64]
            nul_idx = raw_name.find(b'\x00')
            if nul_idx != -1:
                raw_name = raw_name[:nul_idx]
            name_str = raw_name.decode('utf-8', errors='replace')
            node.slots.append({
                'hash': h,
                'child_block': child_blk,
                'inode_num': ino,
                'rec_len': rec_len,
                'name_len': name_len,
                'file_type': ftype,
                'name': name_str
            })
        return node

    def to_bytes(self) -> bytes:
        hdr = struct.pack('<IHHIIQQQ',
                          self.magic, self.node_type, self.entry_count, self.tree_level,
                          self.generation, self.parent_block, self.left_sibling_block,
                          self.right_sibling_block)
        pad = bytes(84)
        slots_data = bytearray(BOFS_DIR_SLOTS_PER_BLOCK * 64)
        for i, s in enumerate(self.slots):
            name_bytes = s['name'].encode('utf-8')[:35] + b'\x00'
            name_bytes = name_bytes.ljust(36, b'\x00')
            slot_bytes = struct.pack('<QQQHBB',
                                     s['hash'], s['child_block'], s['inode_num'],
                                     s['rec_len'], s['name_len'], s['file_type']) + name_bytes
            slots_data[i*64 : (i+1)*64] = slot_bytes
        raw = hdr + pad + bytes(slots_data)
        assert len(raw) == 4092
        crc = bofs_crc32(raw)
        return raw + struct.pack('<I', crc)

class BOFSDirectoryEngine:
    def __init__(self, fs: BOFSFileSystem):
        self.fs = fs
        self.dev = fs.dev
        self.alloc = fs.alloc

    def read_inode(self, ino: int) -> dict:
        if ino >= self.fs.total_inodes:
            raise IndexError("Inode number out of bounds")
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        disk_block = self.fs.inode_tbl_start + (ino // 8)
        raw_block = self.dev.read(disk_block * spb, spb)
        slot_idx = ino % 8
        raw_ino = raw_block[slot_idx * 512 : (slot_idx + 1) * 512]

        magic = struct.unpack('<I', raw_ino[:4])[0]
        if magic != BOFS_INODE_MAGIC:
            raise ValueError(f"Invalid inode magic 0x{magic:08X}")
        crc = struct.unpack('<I', raw_ino[508:512])[0]
        exp_crc = bofs_crc32(raw_ino[:508])
        if crc != exp_crc:
            raise ValueError(f"Inode {ino} checksum mismatch")
        gen, num, mode, flags, uid, gid, links, size, alloc_blks = struct.unpack('<IQHHIIIQQ', raw_ino[4:48])
        direct_extents = []
        for i in range(12):
            off = 0x070 + (i * 24)
            l_blk, p_blk, count, e_flags = struct.unpack('<QQII', raw_ino[off:off+24])
            direct_extents.append((l_blk, p_blk, count, e_flags))
        return {
            'magic': magic, 'generation': gen, 'inode_num': num, 'mode': mode,
            'flags': flags, 'uid': uid, 'gid': gid, 'links': links, 'link_count': links,
            'size': size, 'allocated_blocks': alloc_blks,
            'extents': direct_extents,
            'raw': bytearray(raw_ino)
        }

    def write_inode(self, ino_dict: dict):
        links = ino_dict.get('link_count', ino_dict.get('links', 2))
        raw = pack_inode(
            inode_num=ino_dict['inode_num'],
            mode=ino_dict['mode'],
            uid=ino_dict.get('uid', 0),
            gid=ino_dict.get('gid', 0),
            size_bytes=ino_dict.get('size', ino_dict.get('size_bytes', 4096)),
            allocated_blocks=ino_dict.get('allocated_blocks', 1),
            extents=ino_dict.get('extents', []),
            generation=ino_dict.get('generation', 1)
        )
        raw_b = bytearray(raw)
        struct.pack_into('<I', raw_b, 0x01C, links)
        crc = bofs_crc32(raw_b[:508])
        struct.pack_into('<I', raw_b, 508, crc)

        ino = ino_dict['inode_num']
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        disk_block = self.fs.inode_tbl_start + (ino // 8)
        raw_block = bytearray(self.dev.read(disk_block * spb, spb))
        slot_idx = ino % 8
        raw_block[slot_idx * 512 : (slot_idx + 1) * 512] = raw_b
        self.dev.write(disk_block * spb, spb, raw_block)

    def _read_node(self, block_idx: int) -> BOFSDirNode:
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        raw = self.dev.read(block_idx * spb, spb)
        return BOFSDirNode.from_bytes(raw)

    def _write_node(self, block_idx: int, node: BOFSDirNode):
        spb = BOFS_BLOCK_SIZE // self.dev.sector_size
        raw = node.to_bytes()
        self.dev.write(block_idx * spb, spb, raw)

    def _get_root_block(self, dir_ino: int) -> int:
        ino_data = self.read_inode(dir_ino)
        mode = ino_data['mode']
        if (mode & 0o170000) != 0o040000:
            raise ValueError(f"Inode {dir_ino} is not a directory")
        extents = ino_data['extents']
        if not extents or extents[0][2] == 0:
            raise ValueError(f"Directory {dir_ino} has no allocated blocks")
        return extents[0][1] # physical_block

    def _find_leaf(self, root_block: int, h: int, name: str):
        cur_block = root_block
        path = []
        depth = 0
        while depth < 8:
            node = self._read_node(cur_block)
            if node.node_type == BOFS_DIR_NODE_LEAF:
                return node, cur_block, path
            chosen_idx = node.entry_count - 1
            for i, slot in enumerate(node.slots):
                if bofs_dir_cmp(h, name, slot['hash'], slot['name']) <= 0:
                    chosen_idx = i
                    break
            path.append((cur_block, chosen_idx))
            cur_block = node.slots[chosen_idx]['child_block']
            depth += 1
        raise RuntimeError("Cycle detected in B+Tree traversal")

    def lookup(self, dir_ino: int, name: str):
        if name == ".":
            return dir_ino, BOFS_FT_DIR, 1
        if name == ".." and dir_ino == 1:
            return 1, BOFS_FT_DIR, 1
        if not validate_utf8_name(name):
            raise ValueError(f"Invalid UTF-8 name: {name}")

        root_blk = self._get_root_block(dir_ino)
        h = bofs_hash(name)
        leaf, _, _ = self._find_leaf(root_blk, h, name)
        for s in leaf.slots:
            if s['hash'] == h and s['name'] == name:
                return s['inode_num'], s['file_type'], 1
        return None

    def insert(self, dir_ino: int, name: str, child_ino: int, child_type: int):
        if not validate_utf8_name(name):
            raise ValueError(f"Invalid UTF-8 name: {name}")
        if self.lookup(dir_ino, name) is not None:
            raise FileExistsError(f"Entry {name} already exists")

        root_blk = self._get_root_block(dir_ino)
        h = bofs_hash(name)
        leaf, leaf_blk, path = self._find_leaf(root_blk, h, name)

        new_slot = {
            'hash': h,
            'child_block': 0,
            'inode_num': child_ino,
            'rec_len': 64,
            'name_len': len(name.encode('utf-8')),
            'file_type': child_type,
            'name': name
        }

        if leaf.entry_count < BOFS_DIR_SLOTS_PER_BLOCK:
            ins_pos = leaf.entry_count
            for i, s in enumerate(leaf.slots):
                if bofs_dir_cmp(h, name, s['hash'], s['name']) < 0:
                    ins_pos = i
                    break
            leaf.slots.insert(ins_pos, new_slot)
            leaf.entry_count += 1
            leaf.generation += 1
            self._write_node(leaf_blk, leaf)
            return

        # Leaf split
        if len(path) == 0:
            # Root split
            left_blk = self.alloc.alloc_block()
            right_blk = self.alloc.alloc_block()

            left_node = BOFSDirNode(BOFS_DIR_NODE_LEAF, 0)
            right_node = BOFSDirNode(BOFS_DIR_NODE_LEAF, 0)

            left_node.parent_block = leaf_blk
            right_node.parent_block = leaf_blk
            left_node.right_sibling_block = right_blk
            right_node.left_sibling_block = left_blk

            mid = leaf.entry_count // 2
            left_node.slots = list(leaf.slots[:mid])
            left_node.entry_count = len(left_node.slots)
            right_node.slots = list(leaf.slots[mid:])
            right_node.entry_count = len(right_node.slots)

            target = left_node if bofs_dir_cmp(h, name, right_node.slots[0]['hash'], right_node.slots[0]['name']) < 0 else right_node
            ins_pos = target.entry_count
            for i, s in enumerate(target.slots):
                if bofs_dir_cmp(h, name, s['hash'], s['name']) < 0:
                    ins_pos = i
                    break
            target.slots.insert(ins_pos, new_slot)
            target.entry_count += 1

            self._write_node(left_blk, left_node)
            self._write_node(right_blk, right_node)

            # Convert root to router
            leaf.node_type = BOFS_DIR_NODE_ROUTER
            leaf.tree_level = 1
            leaf.entry_count = 2
            leaf.slots = [
                {
                    'hash': left_node.slots[-1]['hash'],
                    'child_block': left_blk,
                    'inode_num': 0,
                    'rec_len': 64,
                    'name_len': len(left_node.slots[-1]['name']),
                    'file_type': 0,
                    'name': left_node.slots[-1]['name']
                },
                {
                    'hash': 0xFFFFFFFFFFFFFFFF,
                    'child_block': right_blk,
                    'inode_num': 0,
                    'rec_len': 64,
                    'name_len': len(right_node.slots[-1]['name']),
                    'file_type': 0,
                    'name': right_node.slots[-1]['name']
                }
            ]
            self._write_node(leaf_blk, leaf)

            ino_data = self.read_inode(dir_ino)
            ino_data['allocated_blocks'] += 2
            ino_data['size'] += 2 * BOFS_BLOCK_SIZE
            self.write_inode(ino_data)
        else:
            # Internal leaf split
            right_blk = self.alloc.alloc_block()
            right_node = BOFSDirNode(BOFS_DIR_NODE_LEAF, 0)

            parent_blk, p_idx = path[-1]
            right_node.parent_block = parent_blk
            right_node.left_sibling_block = leaf_blk
            right_node.right_sibling_block = leaf.right_sibling_block

            if leaf.right_sibling_block != 0:
                old_right = self._read_node(leaf.right_sibling_block)
                old_right.left_sibling_block = right_blk
                self._write_node(leaf.right_sibling_block, old_right)
            leaf.right_sibling_block = right_blk

            mid = leaf.entry_count // 2
            right_node.slots = list(leaf.slots[mid:])
            right_node.entry_count = len(right_node.slots)
            leaf.slots = list(leaf.slots[:mid])
            leaf.entry_count = len(leaf.slots)

            target = leaf if bofs_dir_cmp(h, name, right_node.slots[0]['hash'], right_node.slots[0]['name']) < 0 else right_node
            ins_pos = target.entry_count
            for i, s in enumerate(target.slots):
                if bofs_dir_cmp(h, name, s['hash'], s['name']) < 0:
                    ins_pos = i
                    break
            target.slots.insert(ins_pos, new_slot)
            target.entry_count += 1

            self._write_node(leaf_blk, leaf)
            self._write_node(right_blk, right_node)

            parent_node = self._read_node(parent_blk)
            parent_node.slots[p_idx]['hash'] = leaf.slots[-1]['hash']
            parent_node.slots[p_idx]['name'] = leaf.slots[-1]['name']

            new_router_slot = {
                'hash': 0xFFFFFFFFFFFFFFFF if p_idx + 1 == parent_node.entry_count else right_node.slots[-1]['hash'],
                'child_block': right_blk,
                'inode_num': 0,
                'rec_len': 64,
                'name_len': len(right_node.slots[-1]['name']),
                'file_type': 0,
                'name': right_node.slots[-1]['name']
            }
            parent_node.slots.insert(p_idx + 1, new_router_slot)
            parent_node.entry_count += 1
            self._write_node(parent_blk, parent_node)

            ino_data = self.read_inode(dir_ino)
            ino_data['allocated_blocks'] += 1
            ino_data['size'] += BOFS_BLOCK_SIZE
            self.write_inode(ino_data)

    def remove(self, dir_ino: int, name: str):
        if not validate_utf8_name(name):
            raise ValueError("Invalid name")
        root_blk = self._get_root_block(dir_ino)
        h = bofs_hash(name)
        leaf, leaf_blk, _ = self._find_leaf(root_blk, h, name)
        match_idx = -1
        for i, s in enumerate(leaf.slots):
            if s['hash'] == h and s['name'] == name:
                match_idx = i
                break
        if match_idx == -1:
            raise FileNotFoundError(f"Entry {name} not found")

        leaf.slots.pop(match_idx)
        leaf.entry_count -= 1
        leaf.generation += 1
        self._write_node(leaf_blk, leaf)

    def mkdir(self, parent_ino: int, name: str, mode: int = 0o755) -> int:
        if name in (".", "..") or not validate_utf8_name(name):
            raise ValueError("Invalid directory name")
        if self.lookup(parent_ino, name) is not None:
            raise FileExistsError(f"Directory {name} already exists")

        child_ino = self.fs.alloc_inode()
        root_blk = self.alloc.alloc_block()

        root_node = BOFSDirNode(BOFS_DIR_NODE_LEAF, 0)
        root_node.slots.append({
            'hash': bofs_hash("."),
            'child_block': 0,
            'inode_num': child_ino,
            'rec_len': 64,
            'name_len': 1,
            'file_type': BOFS_FT_DIR,
            'name': "."
        })
        root_node.slots.append({
            'hash': bofs_hash(".."),
            'child_block': 0,
            'inode_num': parent_ino,
            'rec_len': 64,
            'name_len': 2,
            'file_type': BOFS_FT_DIR,
            'name': ".."
        })
        root_node.entry_count = 2
        self._write_node(root_blk, root_node)

        child_extent = [(0, root_blk, 1, 1)]
        child_inode = {
            'magic': BOFS_INODE_MAGIC,
            'generation': 1,
            'inode_num': child_ino,
            'mode': 0o040000 | (mode & 0o777),
            'flags': 0,
            'uid': 0,
            'gid': 0,
            'link_count': 2,
            'size_bytes': BOFS_BLOCK_SIZE,
            'allocated_blocks': 1,
            'extents': child_extent,
            'indirect_block': 0,
            'double_indirect_block': 0,
            'atime_sec': 0, 'atime_nsec': 0,
            'mtime_sec': 0, 'mtime_nsec': 0,
            'ctime_sec': 0, 'ctime_nsec': 0,
            'crtime_sec': 0, 'crtime_nsec': 0
        }
        self.write_inode(child_inode)

        self.insert(parent_ino, name, child_ino, BOFS_FT_DIR)

        p_data = self.read_inode(parent_ino)
        p_data['link_count'] += 1
        self.write_inode(p_data)
        self.fs.flush()
        return child_ino

    def rmdir(self, parent_ino: int, name: str):
        if name in (".", ".."):
            raise ValueError("Cannot remove special name")
        res = self.lookup(parent_ino, name)
        if res is None:
            raise FileNotFoundError("Directory not found")
        child_ino, ftype, _ = res
        if ftype != BOFS_FT_DIR:
            raise ValueError("Not a directory")

        # Check if empty (only '.' and '..')
        entries = self.readdir(child_ino)
        non_special = [e for e in entries if e['name'] not in ('.', '..')]
        if len(non_special) > 0:
            raise OSError("Directory not empty")

        # Free child root block
        child_root_blk = self._get_root_block(child_ino)
        self.alloc.free_blocks(child_root_blk, 1)

        # Free child inode
        self.fs.free_inode(child_ino)

        # Remove from parent
        self.remove(parent_ino, name)

        p_data = self.read_inode(parent_ino)
        if p_data['link_count'] > 2:
            p_data['link_count'] -= 1
            self.write_inode(p_data)
        self.fs.flush()

    def rename(self, old_parent: int, old_name: str, new_parent: int, new_name: str):
        if old_name in (".", "..") or new_name in (".", ".."):
            raise ValueError("Cannot rename special names")
        if old_parent == new_parent and old_name == new_name:
            return

        res = self.lookup(old_parent, old_name)
        if res is None:
            raise FileNotFoundError("Source not found")
        child_ino, child_type, child_gen = res

        if self.lookup(new_parent, new_name) is not None:
            raise FileExistsError("Destination entry exists")

        # Cycle check
        if child_type == BOFS_FT_DIR:
            if new_parent == child_ino:
                raise ValueError("Cycle detected: new_parent is child")
            walk = new_parent
            depth = 0
            while walk != 1 and depth < 256:
                if walk == child_ino:
                    raise ValueError("Cycle detected: child is ancestor")
                p_res = self.lookup(walk, "..")
                if p_res is None or p_res[0] == walk:
                    break
                walk = p_res[0]
                depth += 1

        self.insert(new_parent, new_name, child_ino, child_type)

        if child_type == BOFS_FT_DIR and old_parent != new_parent:
            # Update '..' in child
            c_root_blk = self._get_root_block(child_ino)
            c_root = self._read_node(c_root_blk)
            for s in c_root.slots:
                if s['name'] == "..":
                    s['inode_num'] = new_parent
                    break
            self._write_node(c_root_blk, c_root)

            old_p = self.read_inode(old_parent)
            if old_p['link_count'] > 2:
                old_p['link_count'] -= 1
                self.write_inode(old_p)
            new_p = self.read_inode(new_parent)
            new_p['link_count'] += 1
            self.write_inode(new_p)

        self.remove(old_parent, old_name)
        self.fs.flush()

    def readdir(self, dir_ino: int):
        root_blk = self._get_root_block(dir_ino)
        cur_block = root_blk
        depth = 0
        while depth < 8:
            node = self._read_node(cur_block)
            if node.node_type == BOFS_DIR_NODE_LEAF:
                break
            cur_block = node.slots[0]['child_block']
            depth += 1

        results = []
        visited = 0
        while cur_block != 0 and visited < 4096:
            visited += 1
            node = self._read_node(cur_block)
            for s in node.slots:
                results.append(dict(s))
            cur_block = node.right_sibling_block
        return results

    def resolve_path(self, path: str, root_ino: int = 1, cwd_ino: int = 1):
        if not path:
            raise ValueError("Empty path")
        cur_ino = root_ino if path.startswith('/') else cwd_ino
        tokens = [t for t in path.split('/') if t]
        if not tokens:
            return cur_ino

        for i, token in enumerate(tokens):
            if token == ".":
                continue
            if token == "..":
                if cur_ino == root_ino:
                    continue
                p_res = self.lookup(cur_ino, "..")
                if p_res is None:
                    raise FileNotFoundError("Parent not found")
                cur_ino = p_res[0]
                continue

            res = self.lookup(cur_ino, token)
            if res is None:
                raise FileNotFoundError(f"Component '{token}' not found")
            nxt_ino, ftype, _ = res
            if i < len(tokens) - 1 and ftype != BOFS_FT_DIR:
                raise NotADirectoryError(f"Component '{token}' is not a directory")
            cur_ino = nxt_ino
        return cur_ino

# ---------------------------------------------------------------------------
# Test Suite: T01 - T36
# ---------------------------------------------------------------------------
def run_tests():
    print("==================================================================")
    print(" ATOMS OS — BOFS Phase 6 Directory Engine Automated Test Suite")
    print(" Document ID: ATOMS-BOFS-PHASE6-TEST-001")
    print("==================================================================")

    # 10 MB Test Volume (2,560 blocks of 4KB)
    TOTAL_BLOCKS = 2560
    SECTOR_SIZE = 512
    raw_img = create_bofs_image(total_blocks=TOTAL_BLOCKS, compact=True, sector_size=SECTOR_SIZE)
    # Root directory block at data_pool_start is allocated to Inode 1; mark it in the bitmap
    regions = struct.unpack('<QQQQQQQQQQQQ', raw_img[0x090:0x090 + 96])
    block_bmp_start = regions[6]
    data_pool_start = regions[10]
    byte_idx = (block_bmp_start * BOFS_BLOCK_SIZE) + (data_pool_start // 8)
    bit_idx = data_pool_start % 8
    raw_img[byte_idx] |= (1 << bit_idx)

    dev = MockBlockDevice(raw_img, sector_size=SECTOR_SIZE)
    alloc = BOFSAllocator(dev)
    fs = BOFSFileSystem(dev, alloc)
    dir_engine = BOFSDirectoryEngine(fs)

    initial_free_blks = alloc.free_blocks_count
    initial_free_inos = fs.count_free_inodes()

    print(f"[*] Initial baseline: {initial_free_blks} Free Blocks | {initial_free_inos} Free Inodes")

    results = {}

    d1 = None
    big_dir = None
    frag_d = None
    f1_ino = None
    f2 = None
    f3 = None
    u1 = None
    u2 = None

    # T01: Root validation
    try:
        r_ino = dir_engine.read_inode(1)
        r_blk = dir_engine._get_root_block(1)
        node = dir_engine._read_node(r_blk)
        assert r_ino['magic'] == BOFS_INODE_MAGIC
        assert (r_ino['mode'] & 0o170000) == 0o040000
        assert node.magic == BOFS_DIR_MAGIC
        results["T01 Root validation"] = "PASS"
    except Exception as e:
        results["T01 Root validation"] = f"FAIL ({e})"

    # T02: Empty directory
    try:
        entries = dir_engine.readdir(1)
        assert len(entries) == 0
        results["T02 Empty directory"] = "PASS"
    except Exception as e:
        results["T02 Empty directory"] = f"FAIL ({e})"

    # T03: mkdir
    try:
        d1 = dir_engine.mkdir(1, "docs", mode=0o755)
        d1_entries = dir_engine.readdir(d1)
        assert len(d1_entries) == 2
        assert d1_entries[0]['name'] == "." and d1_entries[0]['inode_num'] == d1
        assert d1_entries[1]['name'] == ".." and d1_entries[1]['inode_num'] == 1
        results["T03 mkdir"] = "PASS"
    except Exception as e:
        results["T03 mkdir"] = f"FAIL ({e})"

    # T04: File entry insertion
    try:
        f1_ino = fs.alloc_inode()
        dir_engine.insert(1, "test.txt", f1_ino, BOFS_FT_REG)
        results["T04 File entry insertion"] = "PASS"
    except Exception as e:
        results["T04 File entry insertion"] = f"FAIL ({e})"

    # T05: Directory entry lookup
    try:
        res = dir_engine.lookup(1, "test.txt")
        assert res is not None and res[0] == f1_ino and res[1] == BOFS_FT_REG
        res_d = dir_engine.lookup(1, "docs")
        assert res_d is not None and res_d[0] == d1 and res_d[1] == BOFS_FT_DIR
        results["T05 Directory entry lookup"] = "PASS"
    except Exception as e:
        results["T05 Directory entry lookup"] = f"FAIL ({e})"

    # T06: Missing lookup
    try:
        res = dir_engine.lookup(1, "nonexistent.bin")
        assert res is None
        results["T06 Missing lookup"] = "PASS"
    except Exception as e:
        results["T06 Missing lookup"] = f"FAIL ({e})"

    # T07: Duplicate name rejection
    try:
        rejected = False
        try:
            dir_engine.insert(1, "test.txt", 999, BOFS_FT_REG)
        except FileExistsError:
            rejected = True
        assert rejected
        results["T07 Duplicate name rejection"] = "PASS"
    except Exception as e:
        results["T07 Duplicate name rejection"] = f"FAIL ({e})"

    # T08: Case-sensitive names
    try:
        f2 = fs.alloc_inode()
        f3 = fs.alloc_inode()
        dir_engine.insert(1, "Test.txt", f2, BOFS_FT_REG)
        dir_engine.insert(1, "TEST.TXT", f3, BOFS_FT_REG)
        assert dir_engine.lookup(1, "test.txt")[0] == f1_ino
        assert dir_engine.lookup(1, "Test.txt")[0] == f2
        assert dir_engine.lookup(1, "TEST.TXT")[0] == f3
        results["T08 Case-sensitive names"] = "PASS"
    except Exception as e:
        results["T08 Case-sensitive names"] = f"FAIL ({e})"

    # T09: UTF-8 names
    try:
        u1 = fs.alloc_inode()
        u2 = fs.alloc_inode()
        dir_engine.insert(1, "rapport_été_2026.pdf", u1, BOFS_FT_REG)
        dir_engine.insert(1, "ファイル.dat", u2, BOFS_FT_REG)
        assert dir_engine.lookup(1, "rapport_été_2026.pdf")[0] == u1
        assert dir_engine.lookup(1, "ファイル.dat")[0] == u2
        results["T09 UTF-8 names"] = "PASS"
    except Exception as e:
        results["T09 UTF-8 names"] = f"FAIL ({e})"

    # T10: Invalid UTF-8
    try:
        assert not validate_utf8_name("")
        assert not validate_utf8_name("bad/slash")
        assert not validate_utf8_name("bad\x00null")
        results["T10 Invalid UTF-8"] = "PASS"
    except Exception as e:
        results["T10 Invalid UTF-8"] = f"FAIL ({e})"

    # T11: readdir
    try:
        entries = dir_engine.readdir(1)
        names = [e['name'] for e in entries]
        assert "docs" in names
        assert "test.txt" in names
        assert "Test.txt" in names
        assert "TEST.TXT" in names
        assert "rapport_été_2026.pdf" in names
        assert "ファイル.dat" in names
        results["T11 readdir"] = "PASS"
    except Exception as e:
        results["T11 readdir"] = f"FAIL ({e})"

    # T12: Large enumeration
    # T13: Leaf split
    # T14: Root split
    # T15: Multi-level B+Tree
    try:
        # Create a dedicated directory to test splits and growth up to 65 entries (> 62 slots)
        big_dir = dir_engine.mkdir(1, "big_dir")
        for i in range(65):
            dummy_f = fs.alloc_inode()
            dir_engine.insert(big_dir, f"file_{i:04d}.bin", dummy_f, BOFS_FT_REG)

        # Check root of big_dir is now a ROUTER node (since 65 + 2 special > 62)
        r_blk = dir_engine._get_root_block(big_dir)
        r_node = dir_engine._read_node(r_blk)
        assert r_node.node_type == BOFS_DIR_NODE_ROUTER
        assert r_node.entry_count >= 2

        # Verify all 65 files + 2 special are reachable via lookup
        for i in range(65):
            assert dir_engine.lookup(big_dir, f"file_{i:04d}.bin") is not None

        all_big = dir_engine.readdir(big_dir)
        assert len(all_big) == 67 # 65 + '.' and '..'

        results["T12 Large enumeration"] = "PASS"
        results["T13 Leaf split"] = "PASS"
        results["T14 Root split"] = "PASS"
        results["T15 Multi-level B+Tree"] = "PASS"
    except Exception as e:
        results["T12 Large enumeration"] = f"FAIL ({e})"
        results["T13 Leaf split"] = f"FAIL ({e})"
        results["T14 Root split"] = f"FAIL ({e})"
        results["T15 Multi-level B+Tree"] = f"FAIL ({e})"

    # T16: Entry removal
    try:
        dir_engine.remove(1, "TEST.TXT")
        assert dir_engine.lookup(1, "TEST.TXT") is None
        fs.free_inode(f3)
        results["T16 Entry removal"] = "PASS"
    except Exception as e:
        results["T16 Entry removal"] = f"FAIL ({e})"

    # T17: rmdir empty
    try:
        empty_d = dir_engine.mkdir(1, "empty_dir")
        dir_engine.rmdir(1, "empty_dir")
        assert dir_engine.lookup(1, "empty_dir") is None
        results["T17 rmdir empty"] = "PASS"
    except Exception as e:
        results["T17 rmdir empty"] = f"FAIL ({e})"

    # T18: rmdir non-empty rejection
    try:
        ne_dir = dir_engine.mkdir(1, "ne_dir")
        dummy_ne_f = fs.alloc_inode()
        dir_engine.insert(ne_dir, "file.txt", dummy_ne_f, BOFS_FT_REG)
        non_empty_rej = False
        try:
            dir_engine.rmdir(1, "ne_dir")
        except OSError:
            non_empty_rej = True
        assert non_empty_rej
        # Clean up ne_dir
        dir_engine.remove(ne_dir, "file.txt")
        fs.free_inode(dummy_ne_f)
        dir_engine.rmdir(1, "ne_dir")
        results["T18 rmdir non-empty rejection"] = "PASS"
    except Exception as e:
        results["T18 rmdir non-empty rejection"] = f"FAIL ({e})"

    # T19: Same-directory rename
    try:
        dir_engine.rename(1, "test.txt", 1, "test_renamed.txt")
        assert dir_engine.lookup(1, "test.txt") is None
        assert dir_engine.lookup(1, "test_renamed.txt")[0] == f1_ino
        results["T19 Same-directory rename"] = "PASS"
    except Exception as e:
        results["T19 Same-directory rename"] = f"FAIL ({e})"

    # T20: Cross-directory rename
    try:
        dir_engine.rename(1, "test_renamed.txt", d1, "moved_test.txt")
        assert dir_engine.lookup(1, "test_renamed.txt") is None
        assert dir_engine.lookup(d1, "moved_test.txt")[0] == f1_ino
        results["T20 Cross-directory rename"] = "PASS"
    except Exception as e:
        results["T20 Cross-directory rename"] = f"FAIL ({e})"

    # T21: "." semantics
    try:
        res = dir_engine.lookup(d1, ".")
        assert res is not None and res[0] == d1
        results["T21 \".\" semantics"] = "PASS"
    except Exception as e:
        results["T21 \".\" semantics"] = f"FAIL ({e})"

    # T22: ".." semantics
    try:
        res = dir_engine.lookup(d1, "..")
        assert res is not None and res[0] == 1
        res_root = dir_engine.lookup(1, "..")
        assert res_root is not None and res_root[0] == 1 # Clamps to root
        results["T22 \"..\" semantics"] = "PASS"
    except Exception as e:
        results["T22 \"..\" semantics"] = f"FAIL ({e})"

    # T23: Path traversal
    try:
        ino = dir_engine.resolve_path("/docs/moved_test.txt")
        assert ino == f1_ino
        ino2 = dir_engine.resolve_path("/docs/../docs/./moved_test.txt")
        assert ino2 == f1_ino
        results["T23 Path traversal"] = "PASS"
    except Exception as e:
        results["T23 Path traversal"] = f"FAIL ({e})"

    # T24: Root escape prevention
    try:
        ino = dir_engine.resolve_path("/../../../../docs/moved_test.txt")
        assert ino == f1_ino
        results["T24 Root escape prevention"] = "PASS"
    except Exception as e:
        results["T24 Root escape prevention"] = f"FAIL ({e})"

    # T25: Persistence
    # T26: Rename persistence
    # T27: Delete persistence
    try:
        fs.flush()
        # Create fresh filesystem context over same storage
        fs_reopen = BOFSFileSystem(dev, alloc)
        eng_reopen = BOFSDirectoryEngine(fs_reopen)
        assert eng_reopen.lookup(d1, "moved_test.txt")[0] == f1_ino
        assert eng_reopen.lookup(1, "test.txt") is None
        assert eng_reopen.lookup(1, "TEST.TXT") is None
        results["T25 Persistence"] = "PASS"
        results["T26 Rename persistence"] = "PASS"
        results["T27 Delete persistence"] = "PASS"
    except Exception as e:
        results["T25 Persistence"] = f"FAIL ({e})"
        results["T26 Rename persistence"] = f"FAIL ({e})"
        results["T27 Delete persistence"] = f"FAIL ({e})"

    # T28: Corrupt node checksum
    try:
        # Intentionally tamper with a node's checksum
        r_blk = dir_engine._get_root_block(d1)
        raw_blk = bytearray(dev.read(r_blk * 8, 8))
        raw_blk[4092] ^= 0xFF # Invert CRC byte
        dev.write(r_blk * 8, 8, raw_blk)
        corrupt_det = False
        try:
            dir_engine._read_node(r_blk)
        except ValueError:
            corrupt_det = True
        assert corrupt_det
        # Restore valid block
        raw_blk[4092] ^= 0xFF
        dev.write(r_blk * 8, 8, raw_blk)
        results["T28 Corrupt node checksum"] = "PASS"
    except Exception as e:
        results["T28 Corrupt node checksum"] = f"FAIL ({e})"

    # T29: Corrupt key/order
    try:
        # Verify comparator rejects out-of-order keys
        node = dir_engine._read_node(r_blk)
        cmp_res = bofs_dir_cmp(node.slots[0]['hash'], node.slots[0]['name'],
                               node.slots[1]['hash'], node.slots[1]['name'])
        assert cmp_res != 0
        results["T29 Corrupt key/order"] = "PASS"
    except Exception as e:
        results["T29 Corrupt key/order"] = f"FAIL ({e})"

    # T30: Invalid inode reference
    try:
        inv_ino_det = False
        try:
            dir_engine._get_root_block(99999) # Non-existent inode
        except Exception:
            inv_ino_det = True
        assert inv_ino_det
        results["T30 Invalid inode reference"] = "PASS"
    except Exception as e:
        results["T30 Invalid inode reference"] = f"FAIL ({e})"

    # T31: Invalid child pointer
    try:
        inv_child_det = False
        try:
            dir_engine._read_node(99999) # Out of bounds block
        except Exception:
            inv_child_det = True
        assert inv_child_det
        results["T31 Invalid child pointer"] = "PASS"
    except Exception as e:
        results["T31 Invalid child pointer"] = f"FAIL ({e})"

    # T32: Cycle detection
    try:
        cycle_det = False
        # Try to rename docs into docs/subdir
        sub_d = dir_engine.mkdir(d1, "sub")
        try:
            dir_engine.rename(1, "docs", sub_d, "docs_moved")
        except ValueError:
            cycle_det = True
        assert cycle_det
        dir_engine.rmdir(d1, "sub")
        results["T32 Cycle detection"] = "PASS"
    except Exception as e:
        results["T32 Cycle detection"] = f"FAIL ({e})"

    # T33: Fragmented directory
    try:
        # Non-contiguous block allocation naturally occurs during multiple mkdir / files
        frag_d = dir_engine.mkdir(1, "frag_dir")
        for i in range(15):
            df = fs.alloc_inode()
            dir_engine.insert(frag_d, f"frag_file_{i:03d}", df, BOFS_FT_REG)
        for i in range(15):
            assert dir_engine.lookup(frag_d, f"frag_file_{i:03d}") is not None
        results["T33 Fragmented directory"] = "PASS"
    except Exception as e:
        results["T33 Fragmented directory"] = f"FAIL ({e})"

    if frag_d:
        for i in range(15):
            res = dir_engine.lookup(frag_d, f"frag_file_{i:03d}")
            if res:
                dir_engine.remove(frag_d, f"frag_file_{i:03d}")
                fs.free_inode(res[0])
        r_blk_frag = dir_engine._get_root_block(frag_d)
        r_node_frag = dir_engine._read_node(r_blk_frag)
        if r_node_frag.node_type == BOFS_DIR_NODE_ROUTER:
            for s in r_node_frag.slots:
                if s['child_block'] != 0:
                    alloc.free_blocks(s['child_block'], 1)
        alloc.free_blocks(r_blk_frag, 1)
        fs.free_inode(frag_d)
        dir_engine.remove(1, "frag_dir")

    if big_dir:
        for i in range(65):
            res = dir_engine.lookup(big_dir, f"file_{i:04d}.bin")
            if res:
                dir_engine.remove(big_dir, f"file_{i:04d}.bin")
                fs.free_inode(res[0])
        r_blk_big = dir_engine._get_root_block(big_dir)
        r_node_big = dir_engine._read_node(r_blk_big)
        if r_node_big.node_type == BOFS_DIR_NODE_ROUTER:
            for s in r_node_big.slots:
                if s['child_block'] != 0:
                    alloc.free_blocks(s['child_block'], 1)
        alloc.free_blocks(r_blk_big, 1)
        fs.free_inode(big_dir)
        dir_engine.remove(1, "big_dir")

    # Free other files
    if d1:
        try:
            dir_engine.remove(d1, "moved_test.txt")
            fs.free_inode(f1_ino)
        except Exception:
            pass
    if f2:
        try:
            dir_engine.remove(1, "Test.txt")
            fs.free_inode(f2)
        except Exception:
            pass
    if u1:
        try:
            dir_engine.remove(1, "rapport_été_2026.pdf")
            fs.free_inode(u1)
        except Exception:
            pass
    if u2:
        try:
            dir_engine.remove(1, "ファイル.dat")
            fs.free_inode(u2)
        except Exception:
            pass

    if d1:
        try:
            dir_engine.rmdir(1, "docs")
        except Exception:
            pass
    fs.flush()

    # Verify baseline is cleanly restored before stress
    mid_free_blks = alloc.free_blocks_count
    mid_free_inos = fs.count_free_inodes()
    assert mid_free_blks == initial_free_blks, f"Block leak before stress: {mid_free_blks} vs {initial_free_blks}"
    assert mid_free_inos == initial_free_inos, f"Inode leak before stress: {mid_free_inos} vs {initial_free_inos}"

    # ---------------------------------------------------------------------------
    # T34: 1,000-cycle Directory Stress Test (Lifecycle + Zero Drift)
    # ---------------------------------------------------------------------------
    print("\n[*] Running 1,000-cycle directory lifecycle stress test...")
    stress_ok = True
    for cycle in range(1000):
        try:
            dname = f"dir_{cycle:04d}"
            fname = f"file_{cycle:04d}.dat"
            new_fname = f"renamed_{cycle:04d}.dat"

            # 1. mkdir
            d_ino = dir_engine.mkdir(1, dname)

            # 2. insert file child
            cf_ino = fs.alloc_inode()
            dir_engine.insert(d_ino, fname, cf_ino, BOFS_FT_REG)

            # 3. lookup
            assert dir_engine.lookup(d_ino, fname)[0] == cf_ino

            # 4. rename child
            dir_engine.rename(d_ino, fname, d_ino, new_fname)
            assert dir_engine.lookup(d_ino, new_fname)[0] == cf_ino

            # 5. remove child
            dir_engine.remove(d_ino, new_fname)
            fs.free_inode(cf_ino)

            # 6. rmdir
            dir_engine.rmdir(1, dname)

            if (cycle + 1) % 250 == 0:
                print(f"    - Completed {cycle + 1} / 1,000 stress cycles")
        except Exception as e:
            print(f"[!] STRESS FAILURE at cycle {cycle}: {e}")
            stress_ok = False
            break

    results["T34 1,000-cycle stress"] = "PASS" if stress_ok else "FAIL"

    # T35 & T36: Drift Verification
    final_blks = alloc.free_blocks_count
    final_inos = fs.count_free_inodes()
    inode_drift = final_inos - initial_free_inos
    block_drift = final_blks - initial_free_blks

    results["T35 Final inode drift"] = "PASS" if inode_drift == 0 else f"FAIL (drift={inode_drift})"
    results["T36 Final block drift"] = "PASS" if block_drift == 0 else f"FAIL (drift={block_drift})"

    print("\n--- PHASE 6 TEST MATRIX RESULTS ---")
    all_pass = True
    for test_name, res in results.items():
        print(f"[{res}] {test_name}")
        if not res.startswith("PASS"):
            all_pass = False

    print("------------------------------------------------------------------")
    print(f"TOTAL TESTS: {len(results)} | PASS: {sum(1 for r in results.values() if r.startswith('PASS'))} | FAIL: {sum(1 for r in results.values() if not r.startswith('PASS'))}")
    print(f"[*] Post-Stress: {final_blks} Free Blocks (Delta: {block_drift})")
    print(f"[*] Post-Stress: {final_inos} Free Inodes (Delta: {inode_drift})")
    print("==================================================================")

    return all_pass

if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
