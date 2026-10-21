import os
import struct

def inspect(path):
    print("Inspecting:", path)
    with open(path, "rb") as f:
        f.seek(65536)
        sb = f.read(2048)
        magic = struct.unpack_from("<I", sb, 0x55C)[0]
        fsize = struct.unpack_from("<i", sb, 0x34)[0]
        bsize = struct.unpack_from("<i", sb, 0x30)[0]
        print(f"Magic: 0x{magic:08X}, fsize={fsize}, bsize={bsize}")
        
        # Read inode 2
        f.seek(163840 + 2 * 256)
        ino2 = f.read(256)
        mode, nlink, _, _, _, isize, iblocks = struct.unpack_from("<HHIIIqq", ino2, 0)
        db0 = struct.unpack_from("<q", ino2, 112)[0]
        print(f"Inode 2: mode={oct(mode)}, nlink={nlink}, size={isize}, blocks={iblocks}, db0={db0}")
        
        f.seek(db0 * fsize)
        d = f.read(4096)
        off = 0
        while off < min(isize, 4096):
            ino, reclen, dtype, namlen = struct.unpack_from("<IHBB", d, off)
            name = d[off+8:off+8+namlen].decode('ascii', errors='ignore')
            print(f"  off={off:4d}: ino={ino:4d}, reclen={reclen:4d}, dtype={dtype}, namlen={namlen}, name='{name}'")
            if reclen <= 0:
                print("  reclen <= 0! Stopping.")
                break
            off += reclen

print("=== GENUINE ROOTFS ===")
inspect("tools/freebsd_payload/freebsd_rootfs.ufs2")
print("\n=== GENERATED MEDIA.IMG ===")
inspect("build/media.img")
