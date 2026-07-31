import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

def audit_rec_bytes_in_use(rec_num):
    raw = inspector.vdi.read_at(inspector.mft_byte_off + rec_num * 1024, 1024)
    if raw[:4] != b'FILE': return
    
    first_attr_off = struct.unpack('<H', raw[20:22])[0]
    header_bytes_in_use = struct.unpack('<I', raw[24:28])[0]
    
    off = first_attr_off
    end_marker_off = None
    
    while off + 4 <= 1024:
        if raw[off:off+4] == b'\xFF\xFF\xFF\xFF':
            end_marker_off = off
            break
        atype = struct.unpack('<I', raw[off:off+4])[0]
        alen = struct.unpack('<H', raw[off+4:off+6])[0]
        if alen == 0 or alen % 8 != 0: break
        off += alen
        
    if end_marker_off is not None:
        expected_raw_end = end_marker_off + 4
        expected_8byte_aligned = (end_marker_off + 4 + 7) & ~7
        print(f"Record #{rec_num:3d}: EndMarkerOff={end_marker_off:4d} (0x{end_marker_off:03X}), "
              f"EndMarker+4={expected_raw_end:4d} (0x{expected_raw_end:03X}), "
              f"Aligned8={expected_8byte_aligned:4d} (0x{expected_8byte_aligned:03X}), "
              f"HeaderBytesInUse={header_bytes_in_use:4d} (0x{header_bytes_in_use:03X}) "
              f"{'[MATCH RAW+4]' if header_bytes_in_use == expected_raw_end else ''}"
              f"{'[MATCH ALIGNED8]' if header_bytes_in_use == expected_8byte_aligned else ''}")

print("========================================================")
print(" FORENSIC AUDIT OF NATIVE WINDOWS XP RECORDS BYTES_IN_USE")
print("========================================================")

for r in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 27, 29, 31, 63, 169, 170, 36, 176]:
    audit_rec_bytes_in_use(r)

inspector.close()
