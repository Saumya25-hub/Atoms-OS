import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector, apply_usa_fixup

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

lcn = 522159
indx_raw = apply_usa_fixup(inspector.vdi.read_at(inspector.part_byte_off + lcn * 4096, 4096))
entries_off = struct.unpack('<I', indx_raw[24:28])[0] + 24
total_len = struct.unpack('<I', indx_raw[28:32])[0]

e_off = entries_off
print("=== Native XP Index Entries for Record 169 (os details.txt) ===")
while e_off + 16 <= entries_off + total_len:
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_raw[e_off:e_off+14])
    if entry_len == 0 or (flags & 2 and content_len == 0): break
    child_rec = file_ref & 0xFFFFFFFFFFFF
    if child_rec == 169:
        fn_hdr = indx_raw[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        print(f"Rec {child_rec}: entry_len={entry_len}, content_len={content_len}, flags={flags}, name='{fn_str}', ns={fn_ns}")
    e_off += entry_len

print("\n=== Entries for Record 36 (saumya.txt) ===")
e_off = entries_off
while e_off + 16 <= entries_off + total_len:
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_raw[e_off:e_off+14])
    if entry_len == 0 or (flags & 2 and content_len == 0): break
    child_rec = file_ref & 0xFFFFFFFFFFFF
    if child_rec == 36:
        fn_hdr = indx_raw[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        print(f"Rec {child_rec}: entry_len={entry_len}, content_len={content_len}, flags={flags}, name='{fn_str}', ns={fn_ns}")
    e_off += entry_len

print("\n=== Entries for Record 176 (ATOMSTESTS.TXT) ===")
e_off = entries_off
while e_off + 16 <= entries_off + total_len:
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_raw[e_off:e_off+14])
    if entry_len == 0 or (flags & 2 and content_len == 0): break
    child_rec = file_ref & 0xFFFFFFFFFFFF
    if child_rec == 176:
        fn_hdr = indx_raw[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        print(f"Rec {child_rec}: entry_len={entry_len}, content_len={content_len}, flags={flags}, name='{fn_str}', ns={fn_ns}")
    e_off += entry_len

inspector.close()
