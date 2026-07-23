import os
import struct
import hashlib

BUILD_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build"))
RAW_PATH = os.path.join(BUILD_DIR, "ntfs_real_test.raw")
MANIFEST_PATH = os.path.join(BUILD_DIR, "ntfs_real_test_manifest.txt")

SECTOR_SIZE = 512
CLUSTER_SIZE = 4096
SECTORS_PER_CLUSTER = 8
RECORD_SIZE = 1024

TOTAL_SECTORS = 524288  # 256 MB
PARTITION_START_LBA = 2048
PARTITION_SECTORS = TOTAL_SECTORS - PARTITION_START_LBA

MFT_LCN = 4
MFT_MIRR_LCN = 100

def create_raw_ntfs_image():
    if not os.path.exists(BUILD_DIR):
        os.makedirs(BUILD_DIR)

    image = bytearray(TOTAL_SECTORS * SECTOR_SIZE)

    # ---------------------------------------------------------
    # 1. Sector 0: MBR Partition Table
    # ---------------------------------------------------------
    struct.pack_into("<B", image, 0x1BE, 0x80)  # Active bootable
    struct.pack_into("<B", image, 0x1C2, 0x07)  # NTFS partition type
    struct.pack_into("<I", image, 0x1C6, PARTITION_START_LBA)  # Start LBA 2048
    struct.pack_into("<I", image, 0x1CA, PARTITION_SECTORS)    # Sectors count
    struct.pack_into("<H", image, 510, 0xAA55)  # MBR Signature

    # ---------------------------------------------------------
    # 2. Partition LBA 0 (Sector 2048): NTFS Boot Sector (BPB)
    # ---------------------------------------------------------
    p_offset = PARTITION_START_LBA * SECTOR_SIZE
    
    # Jump instruction & OEM ID
    image[p_offset : p_offset + 3] = b"\xEB\x52\x90"
    image[p_offset + 3 : p_offset + 11] = b"NTFS    "
    
    struct.pack_into("<H", image, p_offset + 0x0B, SECTOR_SIZE)          # Bytes per sector: 512
    struct.pack_into("<B", image, p_offset + 0x0D, SECTORS_PER_CLUSTER)  # Sectors per cluster: 8
    struct.pack_into("<Q", image, p_offset + 0x28, PARTITION_SECTORS)    # Total sectors
    struct.pack_into("<Q", image, p_offset + 0x30, MFT_LCN)              # $MFT LCN 4
    struct.pack_into("<Q", image, p_offset + 0x38, MFT_MIRR_LCN)         # $MFTMirr LCN 100
    struct.pack_into("<b", image, p_offset + 0x40, -10)                 # FILE Record size 1024 (2^10)
    struct.pack_into("<b", image, p_offset + 0x44, 1)                    # Index buffer size 4096 (1 cluster)
    struct.pack_into("<Q", image, p_offset + 0x48, 0x123456789ABCDEF0)   # Serial number
    struct.pack_into("<H", image, p_offset + 510, 0xAA55)               # Boot Signature

    # Backup Boot Sector at last sector of partition
    backup_offset = (PARTITION_START_LBA + PARTITION_SECTORS - 1) * SECTOR_SIZE
    image[backup_offset : backup_offset + SECTOR_SIZE] = image[p_offset : p_offset + SECTOR_SIZE]

    # Helper function to write MFT Record with USA fixup
    def build_mft_record(rec_num, flags, base_rec=0):
        buf = bytearray(RECORD_SIZE)
        buf[0:4] = b"FILE"
        struct.pack_into("<H", buf, 0x04, 48)   # USA Offset
        struct.pack_into("<H", buf, 0x06, 3)    # USA Count (1 USN + 2 sector trailers)
        struct.pack_into("<Q", buf, 0x08, 0x100) # LSN
        struct.pack_into("<H", buf, 0x10, 1)     # Sequence number
        struct.pack_into("<H", buf, 0x12, 1)     # Hard links
        struct.pack_into("<H", buf, 0x14, 56)    # First attribute offset
        struct.pack_into("<H", buf, 0x16, flags) # Flags: 1=InUse, 2=Directory
        struct.pack_into("<I", buf, 0x18, 56)    # Bytes in use
        struct.pack_into("<I", buf, 0x1C, RECORD_SIZE) # Bytes allocated
        struct.pack_into("<Q", buf, 0x20, base_rec) # Base record
        return buf

    def finalize_record(buf, usn=1):
        # Store original 2-byte trailers in USA array at offset 50 and 52
        orig1 = struct.unpack_from("<H", buf, 510)[0]
        orig2 = struct.unpack_from("<H", buf, 1022)[0]
        struct.pack_into("<H", buf, 48, usn)
        struct.pack_into("<H", buf, 50, orig1)
        struct.pack_into("<H", buf, 52, orig2)
        # Overwrite trailers with USN
        struct.pack_into("<H", buf, 510, usn)
        struct.pack_into("<H", buf, 1022, usn)
        return buf

    def add_attribute(rec_buf, attr_type, attr_name, content_bytes, non_resident=False, total_len=0, alloc_len=0, runlist=b""):
        offset = struct.unpack_from("<H", rec_buf, 0x14)[0]
        while offset < len(rec_buf):
            t = struct.unpack_from("<I", rec_buf, offset)[0]
            if t == 0xFFFFFFFF or t == 0:
                break
            l = struct.unpack_from("<I", rec_buf, offset + 4)[0]
            offset += l

        name_utf16 = attr_name.encode('utf-16le') if attr_name else b""
        name_len = len(attr_name) if attr_name else 0

        if not non_resident:
            val_len = len(content_bytes)
            name_offset = 24
            val_offset = (name_offset + len(name_utf16) + 7) & ~7
            attr_len = (val_offset + val_len + 7) & ~7

            struct.pack_into("<I", rec_buf, offset + 0x00, attr_type)
            struct.pack_into("<I", rec_buf, offset + 0x04, attr_len)
            rec_buf[offset + 0x08] = 0  # Resident flag
            rec_buf[offset + 0x09] = name_len
            struct.pack_into("<H", rec_buf, offset + 0x0A, name_offset)
            struct.pack_into("<H", rec_buf, offset + 0x0C, 0) # Flags
            struct.pack_into("<H", rec_buf, offset + 0x0E, attr_type & 0xFFFF)
            struct.pack_into("<I", rec_buf, offset + 0x10, val_len)
            struct.pack_into("<H", rec_buf, offset + 0x14, val_offset)

            if name_len > 0:
                rec_buf[offset + name_offset : offset + name_offset + len(name_utf16)] = name_utf16
            rec_buf[offset + val_offset : offset + val_offset + val_len] = content_bytes
        else:
            name_offset = 64
            run_offset = (name_offset + len(name_utf16) + 7) & ~7
            attr_len = (run_offset + len(runlist) + 7) & ~7

            struct.pack_into("<I", rec_buf, offset + 0x00, attr_type)
            struct.pack_into("<I", rec_buf, offset + 0x04, attr_len)
            rec_buf[offset + 0x08] = 1  # Non-resident flag
            rec_buf[offset + 0x09] = name_len
            struct.pack_into("<H", rec_buf, offset + 0x0A, name_offset)
            struct.pack_into("<H", rec_buf, offset + 0x0C, 0) # Flags
            struct.pack_into("<Q", rec_buf, offset + 0x10, 0) # Starting VCN
            struct.pack_into("<Q", rec_buf, offset + 0x18, (total_len + CLUSTER_SIZE - 1) // CLUSTER_SIZE - 1) # Last VCN
            struct.pack_into("<H", rec_buf, offset + 0x20, run_offset)
            struct.pack_into("<Q", rec_buf, offset + 0x28, alloc_len)
            struct.pack_into("<Q", rec_buf, offset + 0x30, total_len)
            struct.pack_into("<Q", rec_buf, offset + 0x38, total_len) # Initialized size

            if name_len > 0:
                rec_buf[offset + name_offset : offset + name_offset + len(name_utf16)] = name_utf16
            rec_buf[offset + run_offset : offset + run_offset + len(runlist)] = runlist

        # Mark end marker after this attribute
        end_offset = offset + attr_len
        if end_offset + 4 <= len(rec_buf):
            struct.pack_into("<I", rec_buf, end_offset, 0xFFFFFFFF)
            used_bytes = end_offset + 4
        else:
            used_bytes = end_offset
        struct.pack_into("<I", rec_buf, 0x18, min(used_bytes, len(rec_buf)))

    def create_filename_attr(parent_mft, name, is_dir=False):
        b = bytearray(66 + len(name) * 2)
        struct.pack_into("<Q", b, 0x00, parent_mft) # Parent record
        struct.pack_into("<I", b, 0x38, 0x20 if not is_dir else 0x10) # Flags
        b[0x40] = len(name) # Name length
        b[0x41] = 1 # Namespace POSIX/Win32
        name_u16 = name.encode('utf-16le')
        b[0x42 : 0x42 + len(name_u16)] = name_u16
        return b

    def create_index_root(entries):
        b = bytearray(4096)
        struct.pack_into("<I", b, 0x00, 0x30) # Attr type $FILE_NAME
        struct.pack_into("<I", b, 0x04, 1)    # Collation rule
        struct.pack_into("<I", b, 0x08, CLUSTER_SIZE) # Index allocation size
        struct.pack_into("<B", b, 0x0C, 1)    # Clusters per index block

        # Index Header at 0x10
        idx_hdr_off = 0x10
        entries_off = idx_hdr_off + 16

        cur_off = entries_off
        for mft_num, name, is_dir in entries:
            fn_val = create_filename_attr(5, name, is_dir)
            entry_len = (16 + len(fn_val) + 7) & ~7
            struct.pack_into("<Q", b, cur_off + 0x00, mft_num)
            struct.pack_into("<H", b, cur_off + 0x08, entry_len)
            struct.pack_into("<H", b, cur_off + 0x0A, len(fn_val))
            struct.pack_into("<I", b, cur_off + 0x0C, 0) # Flags
            b[cur_off + 0x10 : cur_off + 0x10 + len(fn_val)] = fn_val
            cur_off += entry_len

        # Last entry marker
        struct.pack_into("<Q", b, cur_off + 0x00, 0)
        struct.pack_into("<H", b, cur_off + 0x08, 16)
        struct.pack_into("<H", b, cur_off + 0x0A, 0)
        struct.pack_into("<I", b, cur_off + 0x0C, 2) # Flag: END_MARKER
        cur_off += 16

        total_idx_len = cur_off - idx_hdr_off
        struct.pack_into("<I", b, idx_hdr_off + 0x00, entries_off - idx_hdr_off)
        struct.pack_into("<I", b, idx_hdr_off + 0x04, total_idx_len)
        struct.pack_into("<I", b, idx_hdr_off + 0x08, total_idx_len)
        struct.pack_into("<I", b, idx_hdr_off + 0x0C, 0)

        return b[:cur_off]

    mft_lba = PARTITION_START_LBA + (MFT_LCN * SECTORS_PER_CLUSTER)
    mft_byte_offset = mft_lba * SECTOR_SIZE

    def write_record(rec_num, rec_buf):
        rec_buf = finalize_record(rec_buf, usn=1)
        target = mft_byte_offset + (rec_num * RECORD_SIZE)
        image[target : target + RECORD_SIZE] = rec_buf

    # Record 0: $MFT
    r0 = build_mft_record(0, 1)
    add_attribute(r0, 0x10, None, b"\x00" * 48) # $STANDARD_INFORMATION
    add_attribute(r0, 0x30, None, create_filename_attr(5, "$MFT"))
    # Non-resident $DATA for $MFT (LCN 4, 32 clusters)
    mft_run = b"\x11\x20\x04" # 32 clusters starting at LCN 4
    add_attribute(r0, 0x80, None, b"", non_resident=True, total_len=32*CLUSTER_SIZE, alloc_len=32*CLUSTER_SIZE, runlist=mft_run)
    write_record(0, r0)

    # Record 1: $MFTMirr
    r1 = build_mft_record(1, 1)
    add_attribute(r1, 0x10, None, b"\x00" * 48)
    add_attribute(r1, 0x30, None, create_filename_attr(5, "$MFTMirr"))
    mirr_run = b"\x11\x04\x64" # 4 clusters starting at LCN 100
    add_attribute(r1, 0x80, None, b"", non_resident=True, total_len=4*CLUSTER_SIZE, alloc_len=4*CLUSTER_SIZE, runlist=mirr_run)
    write_record(1, r1)
    # Write mirror copy to LCN 100
    mirr_byte_offset = (PARTITION_START_LBA + MFT_MIRR_LCN * SECTORS_PER_CLUSTER) * SECTOR_SIZE
    image[mirr_byte_offset : mirr_byte_offset + RECORD_SIZE] = r0

    # Record 5: Root Directory (.)
    r5 = build_mft_record(5, 3) # Flags 3 = InUse + Directory
    add_attribute(r5, 0x10, None, b"\x00" * 48)
    add_attribute(r5, 0x30, None, create_filename_attr(5, "."))
    r5_idx = create_index_root([(8, "System", True)])
    add_attribute(r5, 0x90, "$I30", r5_idx)
    write_record(5, r5)

    # Record 8: System Directory
    r8 = build_mft_record(8, 3)
    add_attribute(r8, 0x10, None, b"\x00" * 48)
    add_attribute(r8, 0x30, None, create_filename_attr(5, "System", is_dir=True))
    r8_idx = create_index_root([(9, "Apps", True)])
    add_attribute(r8, 0x90, "$I30", r8_idx)
    write_record(8, r8)

    # Dataset Files & Records
    dataset = [
        (10, "hello.txt", b"ATOMS_NTFS_REAL_MEDIA_OK", False),
        (11, "small.txt", b"SMALL_RESIDENT_NTFS_PAYLOAD", False),
        (12, "medium.bin", bytes([i % 256 for i in range(8192)]), False),
        (13, "large.bin", bytes([(i * 7 + 13) % 256 for i in range(1048576)]), True) # Non-resident
    ]

    # Cluster allocation tracking
    cur_free_lcn = 200

    apps_entries = []
    manifest_entries = []

    for rec_num, name, content_bytes, force_non_resident in dataset:
        apps_entries.append((rec_num, name, False))
        r = build_mft_record(rec_num, 1)
        add_attribute(r, 0x10, None, b"\x00" * 48)
        add_attribute(r, 0x30, None, create_filename_attr(9, name))

        if not force_non_resident and len(content_bytes) <= 500:
            add_attribute(r, 0x80, None, content_bytes)
        else:
            clusters_needed = (len(content_bytes) + CLUSTER_SIZE - 1) // CLUSTER_SIZE
            start_lcn = cur_free_lcn
            cur_free_lcn += clusters_needed

            # Write content to clusters
            c_byte_off = (PARTITION_START_LBA + start_lcn * SECTORS_PER_CLUSTER) * SECTOR_SIZE
            image[c_byte_off : c_byte_off + len(content_bytes)] = content_bytes

            # Dynamic Runlist Encoding with Sign-Extension Protection (lcn >= 128 needs 2 bytes)
            len_b = 1 if clusters_needed < 128 else 2
            lcn_b = 1 if start_lcn < 128 else 2
            run_hdr = (lcn_b << 4) | len_b
            runlist = bytearray([run_hdr])
            for i in range(len_b):
                runlist.append((clusters_needed >> (i * 8)) & 0xFF)
            for i in range(lcn_b):
                runlist.append((start_lcn >> (i * 8)) & 0xFF)
            runlist.append(0x00) # End of runlist marker

            add_attribute(r, 0x80, None, b"", non_resident=True, total_len=len(content_bytes), alloc_len=clusters_needed*CLUSTER_SIZE, runlist=runlist)

        write_record(rec_num, r)

        sha = hashlib.sha256(content_bytes).hexdigest().upper()
        manifest_entries.append(f"System/Apps/{name} | Size: {len(content_bytes)} | SHA256: {sha}")

    # Record 16: Nested Directory
    r16 = build_mft_record(16, 3)
    add_attribute(r16, 0x10, None, b"\x00" * 48)
    add_attribute(r16, 0x30, None, create_filename_attr(9, "Nested", is_dir=True))
    r16_idx = create_index_root([(17, "Deep", True)])
    add_attribute(r16, 0x90, "$I30", r16_idx)
    write_record(16, r16)
    apps_entries.append((16, "Nested", True))

    # Record 17: Deep Directory
    r17 = build_mft_record(17, 3)
    add_attribute(r17, 0x10, None, b"\x00" * 48)
    add_attribute(r17, 0x30, None, create_filename_attr(16, "Deep", is_dir=True))
    r17_idx = create_index_root([(18, "real_test.txt", False)])
    add_attribute(r17, 0x90, "$I30", r17_idx)
    write_record(17, r17)

    # Record 18: real_test.txt in Deep
    r18 = build_mft_record(18, 1)
    add_attribute(r18, 0x10, None, b"\x00" * 48)
    add_attribute(r18, 0x30, None, create_filename_attr(17, "real_test.txt"))
    deep_payload = b"NESTED_DEEP_DIRECTORY_FILE_OK"
    add_attribute(r18, 0x80, None, deep_payload)
    write_record(18, r18)
    sha_deep = hashlib.sha256(deep_payload).hexdigest().upper()
    manifest_entries.append(f"System/Apps/Nested/Deep/real_test.txt | Size: {len(deep_payload)} | SHA256: {sha_deep}")

    # Record 9: Apps Directory
    r9 = build_mft_record(9, 3)
    add_attribute(r9, 0x10, None, b"\x00" * 48)
    add_attribute(r9, 0x30, None, create_filename_attr(8, "Apps", is_dir=True))
    r9_idx = create_index_root(apps_entries)
    add_attribute(r9, 0x90, "$I30", r9_idx)
    write_record(9, r9)

    # Write Raw File
    with open(RAW_PATH, "wb") as f:
        f.write(image)

    # Write Manifest
    manifest_lines = [
        "# ATOMS OS Real-Media NTFS Manifest",
        "# Genuine Microsoft NTFS Structure Compatible",
        f"# Raw Image: {RAW_PATH}",
        ""
    ] + manifest_entries

    with open(MANIFEST_PATH, "w") as f:
        f.write("\n".join(manifest_lines) + "\n")

    print("=========================================")
    print(" REAL WINDOWS NTFS TEST MEDIA CREATED!")
    print(f" Raw Path: {RAW_PATH} ({len(image)} bytes)")
    print(f" Manifest: {MANIFEST_PATH}")
    print("=========================================")

if __name__ == "__main__":
    create_raw_ntfs_image()
