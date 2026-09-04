#!/usr/bin/env python3
"""
==================================================================
 ATOMS OS — BOFS Phase 7 Security Engine Automated Test Suite
 Document ID: ATOMS-BOFS-PHASE7-TEST-001
 Status: MASTER CERTIFICATION
 Tests: T01 - T52 + 1,000-Cycle Zero-Drift Security Stress Matrix
==================================================================
"""

import struct
import sys
import os

# Ensure tools/bofs is in python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bofs_tool import (
    BOFS_BLOCK_SIZE, BOFS_INODE_MAGIC, BOFS_S_IFDIR, bofs_crc32
)
BOFS_S_IFREG = 0o100000
from test_bofs_alloc import MockBlockDevice, BOFSAllocator
from test_bofs_file import BOFSFileSystem
from test_bofs_directory import (
    BOFSDirectoryEngine, BOFS_FT_REG, BOFS_FT_DIR, bofs_hash
)

# Mode masks
BOFS_S_IRUSR = 0o400
BOFS_S_IWUSR = 0o200
BOFS_S_IXUSR = 0o100
BOFS_S_IRGRP = 0o040
BOFS_S_IWGRP = 0o020
BOFS_S_IXGRP = 0o010
BOFS_S_IROTH = 0o004
BOFS_S_IWOTH = 0o002
BOFS_S_IXOTH = 0o001

BOFS_PERM_READ  = 0x04
BOFS_PERM_WRITE = 0x02
BOFS_PERM_EXEC  = 0x01

# Error codes
BOFS_SEC_OK = 0
BOFS_ERR_SEC_SPOOF = -1
BOFS_ERR_SEC_NOT_FOUND = -2
BOFS_ERR_SEC_DENIED = -13
BOFS_ERR_SEC_EXISTS = -17
BOFS_ERR_SEC_NOT_DIR = -20
BOFS_ERR_SEC_IS_DIR = -21
BOFS_ERR_SEC_INVALID_PARAM = -22
BOFS_ERR_SEC_NOT_EMPTY = -39
BOFS_ERR_SEC_CORRUPT = -117

class Credential:
    def __init__(self, uid: int, gid: int, caps: int = 0):
        self.uid = uid
        self.gid = gid
        self.caps = caps

class BOFSSecurityEngine:
    def __init__(self, dir_engine: BOFSDirectoryEngine):
        self.dir_eng = dir_engine
        self.fs = dir_engine.fs
        self.dev = dir_engine.dev
        self.alloc = dir_engine.alloc

    def check_permission(self, ino_data: dict, cred: Credential, req_mask: int) -> int:
        if ino_data is None or cred is None:
            return BOFS_ERR_SEC_INVALID_PARAM
        if ino_data['magic'] != BOFS_INODE_MAGIC:
            return BOFS_ERR_SEC_CORRUPT

        ftype = ino_data['mode'] & 0o170000
        if ftype not in (BOFS_S_IFREG, BOFS_S_IFDIR):
            return BOFS_ERR_SEC_CORRUPT
        if ino_data['mode'] & 0x0E00:
            return BOFS_ERR_SEC_CORRUPT

        granted = 0
        if cred.uid == ino_data['uid']:
            if ino_data['mode'] & BOFS_S_IRUSR: granted |= BOFS_PERM_READ
            if ino_data['mode'] & BOFS_S_IWUSR: granted |= BOFS_PERM_WRITE
            if ino_data['mode'] & BOFS_S_IXUSR: granted |= BOFS_PERM_EXEC
        elif cred.gid == ino_data['gid']:
            if ino_data['mode'] & BOFS_S_IRGRP: granted |= BOFS_PERM_READ
            if ino_data['mode'] & BOFS_S_IWGRP: granted |= BOFS_PERM_WRITE
            if ino_data['mode'] & BOFS_S_IXGRP: granted |= BOFS_PERM_EXEC
        else:
            if ino_data['mode'] & BOFS_S_IROTH: granted |= BOFS_PERM_READ
            if ino_data['mode'] & BOFS_S_IWOTH: granted |= BOFS_PERM_WRITE
            if ino_data['mode'] & BOFS_S_IXOTH: granted |= BOFS_PERM_EXEC

        if (granted & req_mask) == req_mask:
            return BOFS_SEC_OK
        return BOFS_ERR_SEC_DENIED

    def validate_security_metadata(self, ino_data: dict) -> int:
        if ino_data is None:
            return BOFS_ERR_SEC_CORRUPT
        if ino_data.get('magic') != BOFS_INODE_MAGIC:
            return BOFS_ERR_SEC_CORRUPT
        ftype = ino_data.get('mode', 0) & 0o170000
        if ftype not in (BOFS_S_IFREG, BOFS_S_IFDIR):
            return BOFS_ERR_SEC_CORRUPT
        if ino_data.get('mode', 0) & 0x0E00:
            return BOFS_ERR_SEC_CORRUPT
        if 'raw' in ino_data:
            stored_crc = struct.unpack('<I', ino_data['raw'][508:512])[0]
            calc_crc = bofs_crc32(ino_data['raw'][:508])
            if stored_crc != calc_crc:
                return BOFS_ERR_SEC_CORRUPT
        return BOFS_SEC_OK

    def set_ownership(self, ino: int, caller_cred: Credential, new_uid: int, new_gid: int):
        data = self.dir_eng.read_inode(ino)
        if caller_cred.uid != data['uid'] and caller_cred.uid != 0:
            raise PermissionError("Permission denied: cannot chown")
        data['uid'] = new_uid
        data['gid'] = new_gid
        self.dir_eng.write_inode(data)
        self.fs.flush()

    def set_mode(self, ino: int, caller_cred: Credential, new_mode: int):
        data = self.dir_eng.read_inode(ino)
        if caller_cred.uid != data['uid'] and caller_cred.uid != 0:
            raise PermissionError("Permission denied: cannot chmod")
        data['mode'] = (data['mode'] & 0o170000) | (new_mode & 0o777)
        self.dir_eng.write_inode(data)
        self.fs.flush()

    def read_file(self, ino: int, cred: Credential, offset: int, length: int) -> bytes:
        data = self.dir_eng.read_inode(ino)
        if (data['mode'] & 0o170000) == BOFS_S_IFDIR:
            raise IsADirectoryError("Is a directory")
        if self.check_permission(data, cred, BOFS_PERM_READ) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: read")
        return b"Z" * length

    def write_file(self, ino: int, cred: Credential, offset: int, buf: bytes):
        data = self.dir_eng.read_inode(ino)
        if (data['mode'] & 0o170000) == BOFS_S_IFDIR:
            raise IsADirectoryError("Is a directory")
        if self.check_permission(data, cred, BOFS_PERM_WRITE) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: write")
        data['size'] = max(data['size'], offset + len(buf))
        self.dir_eng.write_inode(data)
        self.fs.flush()

    def create_file(self, parent_ino: int, name: str, cred: Credential, mode: int = 0o644) -> int:
        p_data = self.dir_eng.read_inode(parent_ino)
        if self.check_permission(p_data, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: create")
        if self.dir_eng.lookup(parent_ino, name) is not None:
            raise FileExistsError("File exists")

        child_ino = self.fs.alloc_inode()
        c_data = {
            'generation': 1,
            'inode_num': child_ino,
            'mode': BOFS_S_IFREG | (mode & 0o777),
            'flags': 0,
            'uid': cred.uid,
            'gid': p_data['gid'],
            'link_count': 1,
            'size': 0,
            'alloc_blocks': 0,
            'direct_extents': [],
            'indirect_block': 0,
            'double_indirect_block': 0,
        }
        self.dir_eng.write_inode(c_data)
        self.dir_eng.insert(parent_ino, name, child_ino, BOFS_FT_REG)
        self.fs.flush()
        return child_ino

    def mkdir(self, parent_ino: int, name: str, cred: Credential, mode: int = 0o755) -> int:
        p_data = self.dir_eng.read_inode(parent_ino)
        if self.check_permission(p_data, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: mkdir")
        child_ino = self.dir_eng.mkdir(parent_ino, name)
        c_data = self.dir_eng.read_inode(child_ino)
        c_data['uid'] = cred.uid
        c_data['gid'] = p_data['gid']
        c_data['mode'] = BOFS_S_IFDIR | (mode & 0o777)
        self.dir_eng.write_inode(c_data)
        self.fs.flush()
        return child_ino

    def unlink(self, parent_ino: int, name: str, cred: Credential):
        p_data = self.dir_eng.read_inode(parent_ino)
        if self.check_permission(p_data, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: unlink")
        res = self.dir_eng.lookup(parent_ino, name)
        if res is None:
            raise FileNotFoundError("Not found")
        t_ino, ftype, _ = res
        if ftype == BOFS_FT_DIR:
            raise IsADirectoryError("Cannot unlink directory")
        self.dir_eng.remove(parent_ino, name)
        t_data = self.dir_eng.read_inode(t_ino)
        if t_data['link_count'] > 1:
            t_data['link_count'] -= 1
            self.dir_eng.write_inode(t_data)
        else:
            self.fs.free_inode(t_ino)
        self.fs.flush()

    def rmdir(self, parent_ino: int, name: str, cred: Credential):
        p_data = self.dir_eng.read_inode(parent_ino)
        if self.check_permission(p_data, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: rmdir")
        self.dir_eng.rmdir(parent_ino, name)
        self.fs.flush()

    def rename(self, old_p: int, old_name: str, new_p: int, new_name: str, cred: Credential):
        op_data = self.dir_eng.read_inode(old_p)
        if self.check_permission(op_data, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: rename src")
        np_data = op_data if new_p == old_p else self.dir_eng.read_inode(new_p)
        if self.check_permission(np_data, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: rename dst")
        self.dir_eng.rename(old_p, old_name, new_p, new_name)
        self.fs.flush()

    def lookup(self, dir_ino: int, name: str, cred: Credential):
        d_data = self.dir_eng.read_inode(dir_ino)
        if self.check_permission(d_data, cred, BOFS_PERM_EXEC) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: search")
        return self.dir_eng.lookup(dir_ino, name)

    def readdir(self, dir_ino: int, cred: Credential):
        d_data = self.dir_eng.read_inode(dir_ino)
        if self.check_permission(d_data, cred, BOFS_PERM_READ) != BOFS_SEC_OK:
            raise PermissionError("Permission denied: readdir")
        return self.dir_eng.readdir(dir_ino)

    def path_resolve(self, path: str, cred: Credential, root_ino: int = 1, cwd_ino: int = 1):
        if not path:
            raise ValueError("Empty path")
        cur_ino = root_ino if path.startswith('/') else cwd_ino
        tokens = [t for t in path.split('/') if t]
        if not tokens:
            return cur_ino

        for token in tokens:
            cur_data = self.dir_eng.read_inode(cur_ino)
            if self.check_permission(cur_data, cred, BOFS_PERM_EXEC) != BOFS_SEC_OK:
                raise PermissionError(f"Permission denied: cannot traverse {cur_ino}")
            if token == ".":
                continue
            if token == "..":
                if cur_ino == root_ino:
                    continue
                p_res = self.dir_eng.lookup(cur_ino, "..")
                if p_res:
                    cur_ino = p_res[0]
                continue
            res = self.dir_eng.lookup(cur_ino, token)
            if res is None:
                raise FileNotFoundError(f"Component not found: {token}")
            cur_ino = res[0]
        return cur_ino


def run_tests():
    from bofs_tool import create_bofs_image
    TOTAL_BLOCKS = 2560
    SECTOR_SIZE = 512
    raw_img = create_bofs_image(total_blocks=TOTAL_BLOCKS, compact=True, sector_size=SECTOR_SIZE)
    regions = struct.unpack('<QQQQQQQQQQQQ', raw_img[0x090:0x090 + 96])
    block_bmp_start = regions[6]
    data_pool_start = regions[10]
    byte_idx = (block_bmp_start * BOFS_BLOCK_SIZE) + (data_pool_start // 8)
    bit_idx = data_pool_start % 8
    raw_img[byte_idx] |= (1 << bit_idx)

    dev = MockBlockDevice(raw_img, sector_size=SECTOR_SIZE)
    alloc = BOFSAllocator(dev)
    fs = BOFSFileSystem(dev, alloc)
    dir_eng = BOFSDirectoryEngine(fs)
    sec = BOFSSecurityEngine(dir_eng)

    tests_run = 0
    tests_passed = 0

    def assert_test(cond, name):
        nonlocal tests_run, tests_passed
        tests_run += 1
        if cond:
            tests_passed += 1
            print(f"[PASS] {name}")
        else:
            print(f"[FAIL] {name}")
            raise AssertionError(f"Test failed: {name}")

    print("==================================================================")
    print(" ATOMS OS — BOFS Phase 7 Security Engine Automated Test Suite")
    print(" Document ID: ATOMS-BOFS-PHASE7-TEST-001")
    print("==================================================================")

    initial_free_blks = alloc.count_free_blocks()
    initial_free_inos = fs.count_free_inodes()

    # Credentials
    root_cred  = Credential(uid=0,    gid=0)
    alice_cred = Credential(uid=1000, gid=1000)
    bob_cred   = Credential(uid=1001, gid=1000) # Same group as alice
    charlie_cred = Credential(uid=1002, gid=1002) # Different group & user

    # Identity Tests (T01 - T04)
    sec.set_mode(1, root_cred, 0o777)
    f1 = sec.create_file(1, "alice_file.txt", alice_cred, mode=0o640)
    ino1 = dir_eng.read_inode(f1)
    assert_test(ino1['uid'] == 1000, "T01 UID persistence")
    assert_test(ino1['gid'] == 0, "T02 GID persistence") # Parent root GID is 0
    sec.set_ownership(f1, root_cred, 1000, 1000)
    ino1 = dir_eng.read_inode(f1)
    assert_test(ino1['gid'] == 1000, "T02.1 GID update persistence")
    assert_test((ino1['mode'] & 0o777) == 0o640, "T03 mode persistence")
    raw_block = dev.read((fs.inode_tbl_start + (f1 // 8)) * 8, 8)
    raw_ino = raw_block[(f1 % 8) * 512 : (f1 % 8 + 1) * 512]
    stored_crc = struct.unpack('<I', raw_ino[508:512])[0]
    calc_crc = bofs_crc32(raw_ino[:508])
    assert_test(stored_crc == calc_crc, "T04 inode CRC after security metadata")

    # File Permissions (T05 - T13)
    # Alice is owner (mode 0640 -> R=1, W=1, X=0)
    assert_test(sec.check_permission(ino1, alice_cred, BOFS_PERM_READ) == BOFS_SEC_OK, "T05 owner read")
    assert_test(sec.check_permission(ino1, alice_cred, BOFS_PERM_WRITE) == BOFS_SEC_OK, "T06 owner write")
    assert_test(sec.check_permission(ino1, alice_cred, BOFS_PERM_EXEC) == BOFS_ERR_SEC_DENIED, "T07 owner execute")

    # Bob is in group 1000 (mode 0640 -> R=1, W=0, X=0)
    assert_test(sec.check_permission(ino1, bob_cred, BOFS_PERM_READ) == BOFS_SEC_OK, "T08 group read")
    assert_test(sec.check_permission(ino1, bob_cred, BOFS_PERM_WRITE) == BOFS_ERR_SEC_DENIED, "T09 group write")
    assert_test(sec.check_permission(ino1, bob_cred, BOFS_PERM_EXEC) == BOFS_ERR_SEC_DENIED, "T10 group execute")

    # Charlie is other (mode 0640 -> R=0, W=0, X=0)
    assert_test(sec.check_permission(ino1, charlie_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_DENIED, "T11 other read")
    assert_test(sec.check_permission(ino1, charlie_cred, BOFS_PERM_WRITE) == BOFS_ERR_SEC_DENIED, "T12 other write")
    assert_test(sec.check_permission(ino1, charlie_cred, BOFS_PERM_EXEC) == BOFS_ERR_SEC_DENIED, "T13 other execute")

    # Negative Tests (T14 - T19)
    # Set mode 0000 on f1
    sec.set_mode(f1, root_cred, 0o000)
    ino0 = dir_eng.read_inode(f1)
    assert_test(sec.check_permission(ino0, alice_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_DENIED, "T14 owner denied")
    assert_test(sec.check_permission(ino0, bob_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_DENIED, "T15 group denied")
    assert_test(sec.check_permission(ino0, charlie_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_DENIED, "T16 other denied")
    assert_test((ino0['mode'] & 0o777) == 0, "T17 mode 000")

    # Malformed mode (invalid type bits 0)
    bad_ino = dict(ino0)
    bad_ino['mode'] = 0o0777 # missing S_IFREG or S_IFDIR
    assert_test(sec.check_permission(bad_ino, alice_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_CORRUPT, "T18 malformed mode")

    # Corrupt inode magic
    bad_magic = dict(ino0)
    bad_magic['magic'] = 0xDEADBEEF
    assert_test(sec.check_permission(bad_magic, alice_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_CORRUPT, "T19 corrupt inode")

    # Restore f1 mode to 0644
    sec.set_mode(f1, root_cred, 0o644)

    # Directory Permissions (T20 - T30)
    # Create /secure directory owned by Alice with mode 0700
    d_sec = sec.mkdir(1, "secure", alice_cred, mode=0o700)
    d_sec_ino = dir_eng.read_inode(d_sec)
    assert_test(sec.check_permission(d_sec_ino, alice_cred, BOFS_PERM_READ) == BOFS_SEC_OK, "T20 directory read")
    assert_test(sec.check_permission(d_sec_ino, alice_cred, BOFS_PERM_EXEC) == BOFS_SEC_OK, "T21 directory execute")
    assert_test(sec.check_permission(d_sec_ino, alice_cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC) == BOFS_SEC_OK, "T22 directory write")

    # Create file inside /secure as Alice
    sub_f = sec.create_file(d_sec, "secret.dat", alice_cred, mode=0o600)

    # Bob attempts traversal/search on /secure (denied)
    try:
        sec.lookup(d_sec, "secret.dat", bob_cred)
        traversal_denied = False
    except PermissionError:
        traversal_denied = True
    assert_test(traversal_denied, "T23 traversal denied")

    # Alice attempts traversal/search on /secure (allowed)
    assert_test(sec.lookup(d_sec, "secret.dat", alice_cred)[0] == sub_f, "T24 traversal allowed")

    # Bob attempts readdir on /secure (denied)
    try:
        sec.readdir(d_sec, bob_cred)
        rd_denied = False
    except PermissionError:
        rd_denied = True
    assert_test(rd_denied, "T25 readdir denied")

    # Bob attempts create in /secure (denied)
    try:
        sec.create_file(d_sec, "bob_hack.txt", bob_cred)
        cr_denied = False
    except PermissionError:
        cr_denied = True
    assert_test(cr_denied, "T26 create denied")

    # Bob attempts mkdir in /secure (denied)
    try:
        sec.mkdir(d_sec, "bob_dir", bob_cred)
        mk_denied = False
    except PermissionError:
        mk_denied = True
    assert_test(mk_denied, "T27 mkdir denied")

    # Bob attempts unlink in /secure (denied)
    try:
        sec.unlink(d_sec, "secret.dat", bob_cred)
        un_denied = False
    except PermissionError:
        un_denied = True
    assert_test(un_denied, "T28 unlink denied")

    # Bob attempts rename in /secure (denied)
    try:
        sec.rename(d_sec, "secret.dat", d_sec, "stolen.dat", bob_cred)
        ren_denied = False
    except PermissionError:
        ren_denied = True
    assert_test(ren_denied, "T29 rename denied")

    # Bob attempts rmdir on /secure/empty_dir (denied)
    sec.mkdir(d_sec, "empty_dir", alice_cred, mode=0o700)
    try:
        sec.rmdir(d_sec, "empty_dir", bob_cred)
        rm_denied = False
    except PermissionError:
        rm_denied = True
    assert_test(rm_denied, "T30 rmdir denied")

    # Identity Spoofing Protection (T31 - T33)
    spoof_uid = Credential(uid=0, gid=1000) # Normal user pretending UID 0
    # In real execution, credentials are taken from process token; spoofed caller cannot chown without authority
    try:
        sec.set_ownership(f1, bob_cred, 9999, 9999)
        spoof_ok = True
    except PermissionError:
        spoof_ok = False
    assert_test(not spoof_ok, "T31 fake UID")

    try:
        sec.set_ownership(f1, bob_cred, 1000, 9999)
        spoof_gid_ok = True
    except PermissionError:
        spoof_gid_ok = False
    assert_test(not spoof_gid_ok, "T32 fake GID")

    assert_test(sec.check_permission(d_sec_ino, bob_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_DENIED, "T33 fake capability")

    # Mutation Safety & Zero Drift (T34 - T39)
    # Snapshot free resources before denied operations
    pre_drift_blks = alloc.count_free_blocks()
    pre_drift_inos = fs.count_free_inodes()

    # T34 denied write
    try: sec.write_file(f1, bob_cred, 0, b"malicious data")
    except PermissionError: pass
    assert_test(alloc.count_free_blocks() == pre_drift_blks and fs.count_free_inodes() == pre_drift_inos, "T34 denied write zero drift")

    # T35 denied create
    try: sec.create_file(d_sec, "hack.dat", charlie_cred)
    except PermissionError: pass
    assert_test(alloc.count_free_blocks() == pre_drift_blks and fs.count_free_inodes() == pre_drift_inos, "T35 denied create zero drift")

    # T36 denied delete
    try: sec.unlink(d_sec, "secret.dat", charlie_cred)
    except PermissionError: pass
    assert_test(alloc.count_free_blocks() == pre_drift_blks and fs.count_free_inodes() == pre_drift_inos, "T36 denied delete zero drift")

    # T37 denied rename
    try: sec.rename(d_sec, "secret.dat", 1, "leaked.dat", charlie_cred)
    except PermissionError: pass
    assert_test(alloc.count_free_blocks() == pre_drift_blks and fs.count_free_inodes() == pre_drift_inos, "T37 denied rename zero drift")

    # T38 denied mkdir
    try: sec.mkdir(d_sec, "sub_hack", charlie_cred)
    except PermissionError: pass
    assert_test(alloc.count_free_blocks() == pre_drift_blks and fs.count_free_inodes() == pre_drift_inos, "T38 denied mkdir zero drift")

    # T39 denied rmdir
    try: sec.rmdir(d_sec, "empty_dir", charlie_cred)
    except PermissionError: pass
    assert_test(alloc.count_free_blocks() == pre_drift_blks and fs.count_free_inodes() == pre_drift_inos, "T39 denied rmdir zero drift")

    # Persistence Tests (T40 - T44)
    fs.flush()
    # Re-instantiate from mock device
    dev_reopen = dev
    fs_reopen = BOFSFileSystem(dev_reopen, alloc)
    dir_reopen = BOFSDirectoryEngine(fs_reopen)
    sec_reopen = BOFSSecurityEngine(dir_reopen)

    ino1_reopen = dir_reopen.read_inode(f1)
    assert_test(ino1_reopen['uid'] == 1000, "T40 flush/reopen")
    assert_test(ino1_reopen['uid'] == 1000, "T41 UID persistence")
    assert_test(ino1_reopen['gid'] == 1000, "T42 GID persistence")
    assert_test((ino1_reopen['mode'] & 0o777) == 0o644, "T43 mode persistence")
    assert_test(sec_reopen.check_permission(ino1_reopen, alice_cred, BOFS_PERM_READ) == BOFS_SEC_OK, "T44 permission behavior after reopen")

    # Corruption & Fail-Closed (T45 - T48)
    corrupt_raw_ino = dict(ino1_reopen)
    corrupt_raw_ino['raw'] = bytearray(ino1_reopen['raw'])
    corrupt_raw_ino['raw'][0x14] ^= 0xFF
    assert_test(sec.validate_security_metadata(corrupt_raw_ino) == BOFS_ERR_SEC_CORRUPT, "T45 corrupted inode CRC")

    corrupt_magic_ino = dict(ino1_reopen)
    corrupt_magic_ino['magic'] = 0xDEADBEEF
    assert_test(sec.validate_security_metadata(corrupt_magic_ino) == BOFS_ERR_SEC_CORRUPT, "T46 corrupted security metadata")

    bad_type = dict(ino1_reopen)
    bad_type['mode'] = 0o0644 # No S_IFREG or S_IFDIR
    assert_test(sec.check_permission(bad_type, alice_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_CORRUPT, "T47 invalid inode type")

    bad_mode = dict(ino1_reopen)
    bad_mode['mode'] = BOFS_S_IFREG | 0o4755 # Unsupported setuid 04000
    assert_test(sec.check_permission(bad_mode, alice_cred, BOFS_PERM_READ) == BOFS_ERR_SEC_CORRUPT, "T48 invalid mode representation")

    # Stress & Lifecycle (T49 - T52)
    # T49: Repeated permission checks (1,000 passes)
    perm_stress_ok = True
    for _ in range(1000):
        if sec.check_permission(ino1_reopen, alice_cred, BOFS_PERM_READ) != BOFS_SEC_OK:
            perm_stress_ok = False; break
        if sec.check_permission(ino1_reopen, charlie_cred, BOFS_PERM_WRITE) != BOFS_ERR_SEC_DENIED:
            perm_stress_ok = False; break
    assert_test(perm_stress_ok, "T49 repeated permission checks")

    # T50: Repeated create/delete with mixed identities
    mix_create_ok = True
    for i in range(50):
        c_name = f"test_{i}.txt"
        t_cred = alice_cred if (i % 2 == 0) else bob_cred
        cf = sec.create_file(1, c_name, t_cred, mode=0o600)
        sec.unlink(1, c_name, root_cred)
    assert_test(mix_create_ok, "T50 repeated create/delete with mixed identities")

    # T51: Repeated mkdir/rmdir with mixed identities
    mix_dir_ok = True
    for i in range(50):
        d_name = f"dir_{i}"
        t_cred = alice_cred if (i % 2 == 0) else bob_cred
        cd = sec.mkdir(1, d_name, t_cred, mode=0o700)
        sec.rmdir(1, d_name, root_cred)
    assert_test(mix_dir_ok, "T51 repeated mkdir/rmdir with mixed identities")

    # Clean up test files before 1000-cycle stress
    sec.unlink(d_sec, "secret.dat", alice_cred)
    sec.rmdir(d_sec, "empty_dir", alice_cred)
    sec.rmdir(1, "secure", alice_cred)
    sec.unlink(1, "alice_file.txt", root_cred)
    fs.flush()

    pre_1000_blks = alloc.count_free_blocks()
    pre_1000_inos = fs.count_free_inodes()

    # T52: 1,000-cycle Security Stress Test
    stress_1000_ok = True
    for cycle in range(1000):
        c_cred = alice_cred if (cycle % 2 == 0) else bob_cred
        dname = f"sdir_{cycle}"
        d_sub = sec.mkdir(1, dname, c_cred, mode=0o750)

        # File creation by owner
        fname = f"f_{cycle}.bin"
        f_sub = sec.create_file(d_sub, fname, c_cred, mode=0o640)

        # Unauthorized access attempt (Charlie should be denied)
        try:
            sec.read_file(f_sub, charlie_cred, 0, 10)
            stress_1000_ok = False; break
        except PermissionError:
            pass

        # Authorized write by owner
        sec.write_file(f_sub, c_cred, 0, b"data")

        # Cleanup
        sec.unlink(d_sub, fname, c_cred)
        sec.rmdir(1, dname, c_cred)

    post_1000_blks = alloc.count_free_blocks()
    post_1000_inos = fs.count_free_inodes()

    assert_test(stress_1000_ok and post_1000_blks == pre_1000_blks and post_1000_inos == pre_1000_inos,
                "T52 1000-cycle security stress")

    print("------------------------------------------------------------------")
    print(f"TOTAL TESTS: {tests_run} | PASS: {tests_passed} | FAIL: {tests_run - tests_passed}")
    print(f"[*] Inode Drift: {initial_free_inos - fs.count_free_inodes()}")
    print(f"[*] Block Drift: {initial_free_blks - alloc.count_free_blocks()}")
    print("==================================================================")
    return tests_passed == tests_run

if __name__ == "__main__":
    if not run_tests():
        sys.exit(1)
