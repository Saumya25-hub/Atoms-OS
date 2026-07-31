import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import VDIImage, apply_usa_fixup, generate_usa_fixup
from tools.windows_xp_ntfs_compatibility_engine import WindowsXPCHKDSKSimulator

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
vdi = VDIImage(vdi_path)

# Parse BPB
mbr = vdi.read_at(0, 512)
part1 = mbr[446:462]
_, _, _, _, lba_start, sector_count = struct.unpack('<B3sB3sII', part1)
part_byte_off = lba_start * 512

bpb = vdi.read_at(part_byte_off, 512)
bytes_per_sector = struct.unpack('<H', bpb[0x0B:0x0D])[0]
sectors_per_cluster = bpb[0x0D]
bytes_per_cluster = bytes_per_sector * sectors_per_cluster
mft_lcn = struct.unpack('<Q', bpb[0x30:0x38])[0]
encoded_rec_sz = struct.unpack('<b', bpb[0x40:0x41])[0]
file_record_size = 1024 if encoded_rec_sz < 0 else encoded_rec_sz * bytes_per_cluster

root_seq = struct.unpack('<H', vdi.read_at(part_byte_off + mft_lcn * bytes_per_cluster + 5 * file_record_size + 16, 2))[0]
root_ref = 5 | (root_seq << 48)

print("========================================================")
print(" FIXING ROOT DIRECTORY INDX BLOCK FLAGS & ENTRY PARSING")
print("========================================================")

lcn = 522159
indx_virt_off = part_byte_off + lcn * bytes_per_cluster
indx_raw = bytearray(vdi.read_at(indx_virt_off, 4096))
indx_raw = bytearray(apply_usa_fixup(indx_raw))

entries_off = struct.unpack('<I', indx_raw[24:28])[0] + 24
total_len = struct.unpack('<I', indx_raw[28:32])[0]

entries_list = []
e_off = entries_off

while e_off + 16 <= entries_off + total_len:
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_raw[e_off:e_off+14])
    if entry_len == 0 or (flags & 2 and content_len == 0): break
    
    child_rec = file_ref & 0xFFFFFFFFFFFF
    if content_len >= 66:
        fn_hdr = indx_raw[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        
        # Build clean entry buffer with flags = 0
        elen = (16 + content_len + 7) & ~7
        ebuf = bytearray(elen)
        struct.pack_into('<QHHH', ebuf, 0, file_ref, elen, content_len, 0) # CLEAR FLAGS TO 0!
        ebuf[16:16+content_len] = indx_raw[e_off+16 : e_off+16+content_len]
        
        entries_list.append((fn_str, file_ref, ebuf))
        
    e_off += entry_len

# Sort all entries strictly in Unicode $UpCase order!
entries_list.sort(key=lambda x: x[0].upper())

print(f"Rebuilt {len(entries_list)} Root Directory Index Entries:")
for idx, (fn_str, ref, ebuf) in enumerate(entries_list):
    crec = ref & 0xFFFFFFFFFFFF
    print(f"  Entry #{idx:2d}: '{fn_str:25s}' -> MFT #{crec:3d}")

# Write back sorted entries
new_entries_buf = bytearray()
for name_str, ref, raw_e in entries_list:
    new_entries_buf.extend(raw_e)

# Append ONLY ONE Final Last Entry Marker at the very end!
last_e = bytearray(16)
struct.pack_into('<QHHH', last_e, 0, 0, 16, 0, 2) # FLAGS = 2 (LAST ENTRY) ONLY HERE!
new_entries_buf.extend(last_e)

new_total_len = len(new_entries_buf)
indx_raw[28:32] = struct.pack('<I', new_total_len)
indx_raw[entries_off : entries_off + new_total_len] = new_entries_buf

vdi.write_at(indx_virt_off, generate_usa_fixup(indx_raw))
print("-> Root Directory Index ($I30) Rebuilt & Fixed Successfully")

# Verify with CHKDSK Simulator
chk = WindowsXPCHKDSKSimulator(vdi)
chk_result = chk.run_all_checks()

vdi.close()
