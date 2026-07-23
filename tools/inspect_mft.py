import struct

with open('build/ntfs_real_test.raw', 'rb') as f:
    f.seek(2048 * 512 + 16384)
    data = f.read(32 * 1024)

for i in range(20):
    rec = data[i * 1024 : (i + 1) * 1024]
    magic = rec[0:4]
    in_use = struct.unpack_from("<I", rec, 0x18)[0]
    alloc = struct.unpack_from("<I", rec, 0x1C)[0]
    print(f"Record {i:2d}: magic={magic}, in_use={in_use}, alloc={alloc}")
