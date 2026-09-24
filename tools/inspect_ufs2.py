import struct

with open('tools/freebsd_payload/freebsd_rootfs.ufs2', 'rb') as f:
    f.seek(65536)
    sb = f.read(2048)
    
    # struct fs in FreeBSD sys/ufs/ffs/fs.h
    fields = [
        ("fs_firstfield", 0, "i"),
        ("fs_unused_1", 4, "i"),
        ("fs_sblkno", 0x10, "i"),
        ("fs_cblkno", 0x14, "i"),
        ("fs_iblkno", 0x18, "i"),
        ("fs_dblkno", 0x1c, "i"),
        ("fs_cgoffset", 0x20, "i"),
        ("fs_cgmask", 0x24, "i"),
        ("fs_time", 0x28, "i"),
        ("fs_size (ufs1)", 0x2c, "i"),
        ("fs_dsize (ufs1)", 0x30, "i"),
        ("fs_ncg", 0xd8, "i"),
        ("fs_bsize", 0x30, "i"),
        ("fs_fsize", 0x34, "i"),
        ("fs_frag", 0x38, "i"),
        ("fs_fpg", 0xdc, "i"),
        ("fs_ipg", 0xe0, "i"),
        ("fs_minfree", 0x48, "i"),
        ("fs_magic", 0x55c, "I"),
        ("fs_clean", 0x560, "b"),
    ]
    for name, off, fmt in fields:
        val = struct.unpack_from("<" + fmt, sb, off)[0]
        print(f"{name:20} (offset {hex(off):6}): {val}")

    # Check where fs_ncg really is by scanning for cylinder count
    print("\nScanning for possible ncg/fpg values:")
    for off in range(0, 0x200, 4):
        v = struct.unpack_from("<i", sb, off)[0]
        if v in (4, 8, 16, 32, 64, 128, 16384, 32768, 65536):
            print(f"Offset {hex(off)}: {v}")
