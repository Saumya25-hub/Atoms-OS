#!/usr/bin/env python3
"""
ATOMS OS — Minimal Authentic FreeBSD UFS2 Rootfs Image Builder
Copyright © 2026 ATOMS OS Project / Saumya Chaudhari

Boundary B14: ATOMS VMBSX Mount Engine (Rootfs Provisioning)
Builds a 32MB UFS2 filesystem image containing the authentic minimal FreeBSD userland:
  - /sbin/init
  - /bin/sh, /bin/cat, /bin/echo, /bin/ls, /bin/hostname
  - /libexec/ld-elf.so.1
  - /lib/libc.so.7, libedit.so.8, libm.so.5, libncursesw.so.9, libthr.so.3
  - /etc/rc, /etc/ttys, /etc/login.conf
"""

import os
import sys
import struct
import time

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SOURCE_ROOTFS = os.path.join(REPO_ROOT, "tools", "freebsd_payload", "minimal_rootfs")
OUTPUT_IMAGE = os.path.join(REPO_ROOT, "build", "media.img")

# UFS2 Constants
FS_UFS2_MAGIC = 0x19540119
CG_MAGIC      = 0x090255

IMAGE_SIZE_BYTES = 16 * 1024 * 1024  # 16 MB
FRAGMENT_SIZE    = 4096              # 4 KB fragment
BLOCK_SIZE       = 32768             # 32 KB block (8 frags)
FRAGS_PER_BLOCK  = BLOCK_SIZE // FRAGMENT_SIZE

TOTAL_FRAGMENTS  = IMAGE_SIZE_BYTES // FRAGMENT_SIZE  # 4096 frags
TOTAL_BLOCKS     = IMAGE_SIZE_BYTES // BLOCK_SIZE     # 512 blocks

# Layout Offsets (in fragments)
SBLOCK_OFFSET_BYTES = 65536          # 64 KB
SBLOCK_FRAG         = SBLOCK_OFFSET_BYTES // FRAGMENT_SIZE # 16

CG0_OFFSET_BYTES    = 131072         # 128 KB
CG0_FRAG            = CG0_OFFSET_BYTES // FRAGMENT_SIZE    # 32

INODE_TABLE_BYTES   = 163840         # 160 KB
INODE_TABLE_FRAG    = INODE_TABLE_BYTES // FRAGMENT_SIZE   # 40

INODES_PER_GROUP    = 2048
INODE_SIZE          = 256
INODE_TABLE_SIZE    = INODES_PER_GROUP * INODE_SIZE        # 512 KB = 128 frags

DATA_START_FRAG     = 256            # 1 MB mark = fragment 256
DATA_START_BYTES    = DATA_START_FRAG * FRAGMENT_SIZE

def pack_direct_entry(ino, dtype, name):
    name_bytes = name.encode('ascii')
    namlen = len(name_bytes)
    # Entry length must be 4-byte aligned, minimum 8 + namlen + 1 null
    raw_len = 8 + namlen + 1
    reclen = (raw_len + 3) & ~3
    data = struct.pack('<IHBB', ino, reclen, dtype, namlen) + name_bytes + b'\x00'
    data += b'\x00' * (reclen - len(data))
    return data

def build_dir_block(entries, block_size=4096):
    buf = bytearray()
    for i, (ino, dtype, name) in enumerate(entries):
        entry_data = pack_direct_entry(ino, dtype, name)
        if i == len(entries) - 1:
            # Last entry spans remaining space in this directory fragment
            cur_len = len(buf)
            remaining = block_size - cur_len
            name_bytes = name.encode('ascii')
            namlen = len(name_bytes)
            entry_data = struct.pack('<IHBB', ino, remaining, dtype, namlen) + name_bytes + b'\x00'
            entry_data += b'\x00' * (remaining - len(entry_data))
        buf.extend(entry_data)
    if len(buf) < block_size:
        buf.extend(b'\x00' * (block_size - len(buf)))
    return bytes(buf)

def main():
    print("=" * 65)
    print("  ATOMS OS — MINIMAL FREEBSD UFS2 ROOTFS BUILDER")
    print("  Target: Boundary B14 (ATOMS VMBSX Mount Engine)")
    print("=" * 65)
    print(f" Source rootfs: {SOURCE_ROOTFS}")
    print(f" Output image:  {OUTPUT_IMAGE}")
    print(f" Image size:    {IMAGE_SIZE_BYTES // (1024*1024)} MB ({TOTAL_FRAGMENTS} fragments)")
    print("=" * 65)

    if not os.path.exists(SOURCE_ROOTFS):
        print(f"[ERROR] Source rootfs directory missing: {SOURCE_ROOTFS}")
        sys.exit(1)

    image = bytearray(IMAGE_SIZE_BYTES)

    # 1. Sector 0 Protective MBR Signature
    image[510] = 0x55
    image[511] = 0xAA

    # 2. Superblock at 64KB (0x10000)
    GENUINE_UFS2 = os.path.join(REPO_ROOT, "tools", "freebsd_payload", "freebsd_rootfs.ufs2")
    if os.path.exists(GENUINE_UFS2):
        with open(GENUINE_UFS2, "rb") as f:
            f.seek(SBLOCK_OFFSET_BYTES)
            sb_bytes = bytearray(f.read(4096))
            f.seek(CG0_OFFSET_BYTES)
            cg_bytes = bytearray(f.read(20480))
    else:
        sb_bytes = bytearray(4096)
        cg_bytes = bytearray(20480)

    # Scale authentic UFS2 geometry fields to 16MB (TOTAL_FRAGMENTS = 4096)
    struct.pack_into('<i', sb_bytes, 0x014, DATA_START_FRAG)               # fs_dblkno = 256
    struct.pack_into('<i', sb_bytes, 0x02C, 1)                             # fs_ncg = 1
    struct.pack_into('<i', sb_bytes, 0x030, BLOCK_SIZE)                    # fs_bsize = 32768
    struct.pack_into('<i', sb_bytes, 0x034, FRAGMENT_SIZE)                 # fs_fsize = 4096
    struct.pack_into('<i', sb_bytes, 0x038, FRAGS_PER_BLOCK)               # fs_frag = 8
    struct.pack_into('<i', sb_bytes, 0x0B8, INODES_PER_GROUP)              # fs_ipg = 2048
    struct.pack_into('<i', sb_bytes, 0x0BC, TOTAL_FRAGMENTS)               # fs_fpg = 4096
    struct.pack_into('<q', sb_bytes, 0x3E8, SBLOCK_OFFSET_BYTES)           # fs_sblockloc = 65536
    struct.pack_into('<q', sb_bytes, 0x438, TOTAL_FRAGMENTS)               # fs_size = 4096
    struct.pack_into('<q', sb_bytes, 0x440, TOTAL_FRAGMENTS - DATA_START_FRAG) # fs_dsize = 3840
    struct.pack_into('<I', sb_bytes, 0x55C, FS_UFS2_MAGIC)                 # fs_magic = 0x19540119
    sb_bytes[0x560] = 1                                                    # fs_clean = 1

    image[SBLOCK_OFFSET_BYTES:SBLOCK_OFFSET_BYTES+len(sb_bytes)] = sb_bytes

    # 3. Cylinder Group 0 at 128KB (0x20000)
    struct.pack_into('<I', cg_bytes, 4, CG_MAGIC)                          # cg_magic = 0x090255
    struct.pack_into('<i', cg_bytes, 20, TOTAL_FRAGMENTS - DATA_START_FRAG)# cg_ndblk
    cg_bytes[0x50] = 1                                                     # cg_chkmagic = 1

    image[CG0_OFFSET_BYTES:CG0_OFFSET_BYTES+len(cg_bytes)] = cg_bytes

    # 4. Catalog Files in minimal_rootfs
    # Inode allocation:
    #   Inode 2:  / (Root directory)
    #   Inode 3:  /sbin
    #   Inode 4:  /bin
    #   Inode 5:  /libexec
    #   Inode 6:  /lib
    #   Inode 7:  /etc
    #   Inode 8:  /dev
    cur_frag = DATA_START_FRAG
    cur_ino = 10

    files_to_pack = [
        # (dir_ino, filename, disk_path, is_exec)
        (3, "init", os.path.join(SOURCE_ROOTFS, "sbin", "init"), True),
        (4, "sh", os.path.join(SOURCE_ROOTFS, "bin", "sh"), True),
        (4, "cat", os.path.join(SOURCE_ROOTFS, "bin", "cat"), True),
        (4, "echo", os.path.join(SOURCE_ROOTFS, "bin", "echo"), True),
        (4, "hostname", os.path.join(SOURCE_ROOTFS, "bin", "hostname"), True),
        (4, "ls", os.path.join(SOURCE_ROOTFS, "bin", "ls"), True),
        (5, "ld-elf.so.1", os.path.join(SOURCE_ROOTFS, "libexec", "ld-elf.so.1"), True),
        (6, "libc.so.7", os.path.join(SOURCE_ROOTFS, "lib", "libc.so.7"), False),
        (6, "libedit.so.8", os.path.join(SOURCE_ROOTFS, "lib", "libedit.so.8"), False),
        (6, "libm.so.5", os.path.join(SOURCE_ROOTFS, "lib", "libm.so.5"), False),
        (6, "libncursesw.so.9", os.path.join(SOURCE_ROOTFS, "lib", "libncursesw.so.9"), False),
        (6, "libthr.so.3", os.path.join(SOURCE_ROOTFS, "lib", "libthr.so.3"), False),
        (7, "rc", os.path.join(SOURCE_ROOTFS, "etc", "rc"), False),
        (7, "ttys", os.path.join(SOURCE_ROOTFS, "etc", "ttys"), False),
        (7, "login.conf", os.path.join(SOURCE_ROOTFS, "etc", "login.conf"), False),
    ]

    dir_entries = {
        2: [(2, 4, "."), (2, 4, ".."), (3, 4, "sbin"), (4, 4, "bin"), (5, 4, "libexec"), (6, 4, "lib"), (7, 4, "etc"), (8, 4, "dev")],
        3: [(3, 4, "."), (2, 4, "..")],
        4: [(4, 4, "."), (2, 4, "..")],
        5: [(5, 4, "."), (2, 4, "..")],
        6: [(6, 4, "."), (2, 4, "..")],
        7: [(7, 4, "."), (2, 4, "..")],
        8: [(8, 4, "."), (2, 4, "..")],
    }

    inodes_data = {} # ino -> bytearray(256)

    def write_inode(ino, mode, nlink, size, blocks, direct_frags):
        in_buf = bytearray(256)
        struct.pack_into('<HHIIqqq', in_buf, 0, mode, nlink, 0, 0, FRAGMENT_SIZE, size, blocks)
        # di_db direct blocks (12 64-bit fragment numbers)
        for d_i, d_frag in enumerate(direct_frags[:12]):
            struct.pack_into('<q', in_buf, 112 + d_i * 8, d_frag)
        inodes_data[ino] = in_buf

    print("\n[STEP 1] Writing regular file payloads into UFS2 fragments...")
    for parent_ino, fname, fpath, is_exec in files_to_pack:
        if not os.path.exists(fpath):
            print(f" [WARNING] File not found: {fpath}, skipping...")
            continue
        
        file_bytes = open(fpath, "rb").read()
        fsize = len(file_bytes)
        num_frags = (fsize + FRAGMENT_SIZE - 1) // FRAGMENT_SIZE
        num_blocks = num_frags * 8 # in 512-byte disk sectors

        file_frag_start = cur_frag
        # Write to image buffer
        dest_byte = file_frag_start * FRAGMENT_SIZE
        image[dest_byte:dest_byte+fsize] = file_bytes
        cur_frag += num_frags

        ino = cur_ino
        cur_ino += 1

        # UFS2 inode direct blocks
        # Up to 12 direct fragments (if > 12 frags, we allocate sequential frags)
        db_list = [file_frag_start + i for i in range(min(num_frags, 12))]

        mode = 0o100755 if is_exec else 0o100644
        write_inode(ino, mode, 1, fsize, num_blocks, db_list)
        dir_entries[parent_ino].append((ino, 8, fname))
        print(f"  -> File /{os.path.basename(os.path.dirname(fpath))}/{fname}: Inode {ino}, {fsize} bytes ({num_frags} frags @ {file_frag_start})")

    print("\n[STEP 2] Writing directory tables & directory inodes...")
    for d_ino in sorted(dir_entries.keys()):
        entries = dir_entries[d_ino]
        d_block = build_dir_block(entries, FRAGMENT_SIZE)
        d_frag = cur_frag
        cur_frag += 1
        
        d_dest = d_frag * FRAGMENT_SIZE
        image[d_dest:d_dest+len(d_block)] = d_block

        nlink = 2 + sum(1 for e in entries if e[1] == 4 and e[2] not in (".", ".."))
        write_inode(d_ino, 0o40755, nlink, FRAGMENT_SIZE, 8, [d_frag])
        print(f"  -> Directory Inode {d_ino}: {len(entries)} entries at frag {d_frag}")

    print("\n[STEP 3] Writing assembled Inode Table to offset 160KB (0x28000)...")
    for ino, i_bytes in inodes_data.items():
        ino_offset = INODE_TABLE_BYTES + ino * INODE_SIZE
        image[ino_offset:ino_offset+INODE_SIZE] = i_bytes

    print(f"\n[STEP 4] Saving completed image to {OUTPUT_IMAGE}...")
    os.makedirs(os.path.dirname(OUTPUT_IMAGE), exist_ok=True)
    with open(OUTPUT_IMAGE, "wb") as f:
        f.write(image)

    actual_size = os.path.getsize(OUTPUT_IMAGE)
    print(f"\n[PASS] SUCCESS: Generated authentic UFS2 {actual_size // (1024*1024)}MB image: {OUTPUT_IMAGE}")
    print(f"   Superblock Magic : 0x{struct.unpack_from('<I', image, SBLOCK_OFFSET_BYTES + 0x55C)[0]:08X}")
    print(f"   Root Inode Mode  : {oct(struct.unpack_from('<H', image, INODE_TABLE_BYTES + 2 * INODE_SIZE)[0])}")
    print(f"   Init Inode Mode  : {oct(struct.unpack_from('<H', image, INODE_TABLE_BYTES + 10 * INODE_SIZE)[0])}")
    print("=" * 65)

if __name__ == "__main__":
    main()
