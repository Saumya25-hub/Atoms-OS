import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector, apply_usa_fixup

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

lcn = 522159
indx_virt_off = inspector.part_byte_off + lcn * inspector.bytes_per_cluster
indx_raw = apply_usa_fixup(inspector.vdi.read_at(indx_virt_off, 4096))

print("========================================================")
print(f" DUMPING ALL ENTRIES IN INDX BLOCK AT LCN {lcn}")
print("========================================================")

entries_off = struct.unpack('<I', indx_raw[24:28])[0] + 24
total_len = struct.unpack('<I', indx_raw[28:32])[0]
print(f"Entries Offset: {entries_off}, Total Size: {total_len}")

e_off = entries_off
idx = 0

while e_off + 16 <= entries_off + total_len:
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_raw[e_off:e_off+14])
    if entry_len == 0: break
    child_rec = file_ref & 0xFFFFFFFFFFFF
    child_seq = (file_ref >> 48) & 0xFFFF
    
    if content_len >= 66:
        fn_hdr = indx_raw[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        file_flags = struct.unpack('<I', fn_hdr[56:60])[0]
        hidden_sys = (file_flags & 6) != 0
        print(f" Entry #{idx:2d}: '{fn_str:25s}' | MFT #{child_rec:3d} (seq {child_seq}) | NS={fn_ns} | Flags=0x{file_flags:04X} {'[HIDDEN]' if hidden_sys else ''}")
    else:
        print(f" Entry #{idx:2d}: [LAST ENTRY MARKER] | flags={flags}")
        
    if flags & 2: break
    e_off += entry_len
    idx += 1

inspector.close()
