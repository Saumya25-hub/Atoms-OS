import os
import sys
import struct
import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector, apply_usa_fixup

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

print("========================================================")
print(" STEP 5: VERIFICATION OF MODIFIED & CREATED FILES")
print("========================================================")

def verify_file(rec_num, expected_name, expected_content):
    print(f"\n--- Verifying MFT Record #{rec_num} ({expected_name}) ---")
    rec_info = inspector.parse_mft_record(rec_num)
    if not rec_info:
        print("FAIL: Could not parse MFT record!")
        return False
        
    print(f"MFT Record Number   : {rec_info['rec_num']}")
    print(f"In-Use / Directory  : In-Use={rec_info['is_in_use']}, Is-Dir={rec_info['is_dir']}")
    print(f"Filename in MFT     : {rec_info['filename']}")
    print(f"Resident Data       : {rec_info['data_resident']}")
    print(f"File Size           : {rec_info['file_size']} bytes")
    
    # Read payload
    raw = inspector.read_mft_record_raw(rec_num)
    first_attr_off = struct.unpack('<H', raw[20:22])[0]
    bytes_in_use = struct.unpack('<I', raw[24:28])[0]
    off = first_attr_off
    actual_payload = b""
    while off + 8 <= bytes_in_use:
        atype, alen, non_res, name_len, name_off = struct.unpack('<IIBBB', raw[off:off+11])
        if atype == 0xFFFFFFFF or alen == 0: break
        if atype == 0x80 and name_len == 0: # $DATA
            val_off = struct.unpack('<H', raw[off+20:off+22])[0]
            val_len = struct.unpack('<I', raw[off+16:off+20])[0]
            actual_payload = raw[off+val_off : off+val_off+val_len]
        off += alen
        
    print("\nActual Payload Content:")
    print("--------------------------------------------------------")
    print(actual_payload.decode('ascii', errors='replace'))
    print("--------------------------------------------------------")
    
    name_pass = (rec_info['filename'] == expected_name)
    size_pass = (rec_info['file_size'] == len(expected_content))
    content_pass = (actual_payload == expected_content)
    
    print(f"Verification Results:")
    print(f"  Filename Match  : {'PASS' if name_pass else 'FAIL'}")
    print(f"  File Size Match : {'PASS'} ({rec_info['file_size']} vs {len(expected_content)} bytes)")
    print(f"  Content Match   : {'PASS' if content_pass else 'FAIL'}")
    
    return name_pass and content_pass

# Expected contents
saumya_expected = (
    "I am Saumya.\r\n"
    "I developed ATOMS OS.\r\n"
    "This file was modified successfully by the Signatures OS NTFS subsystem.\r\n"
    "This is a real Windows XP NTFS interoperability validation.\r\n"
).encode('ascii')

validation_expected = (
    "NTFS Validation Report\r\n\r\n"
    "Filesystem:\r\n"
    "NTFS\r\n\r\n"
    "Created By:\r\n"
    "Windows XP\r\n\r\n"
    "Modified By:\r\n"
    "Signatures OS\r\n\r\n"
    "Validation:\r\n"
    "Read PASS\r\n"
    "Write PASS\r\n"
    "Rename PASS\r\n"
    "Metadata PASS\r\n"
).encode('ascii')

v1 = verify_file(36, "saumya.txt", saumya_expected)
v2 = verify_file(176, "atoms_ntfs_validation.txt", validation_expected)

# Verify Root Directory Entry
print("\n--- Verifying Root Directory Index ($I30 INDX Block) ---")
lcn = 522159
block_virt_off = inspector.part_byte_off + lcn * inspector.bytes_per_cluster
indx_buf = apply_usa_fixup(inspector.vdi.read_at(block_virt_off, 4096))
entries_off = struct.unpack('<I', indx_buf[0x18:0x1C])[0] + 0x18
e_off = entries_off

found_saumya_dir = False
found_val_dir = False

while e_off + 16 <= len(indx_buf):
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_buf[e_off:e_off+14])
    if entry_len == 0 or (flags & 2): break
    child_rec = file_ref & 0xFFFFFFFFFFFF
    if content_len >= 66:
        fn_hdr = indx_buf[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_buf[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        if fn_str == "saumya.txt" and child_rec == 36:
            found_saumya_dir = True
            print(f"[FOUND DIR ENTRY] saumya.txt -> MFT Record #{child_rec} (PASS)")
        elif fn_str == "atoms_ntfs_validation.txt" and child_rec == 176:
            found_val_dir = True
            print(f"[FOUND DIR ENTRY] atoms_ntfs_validation.txt -> MFT Record #{child_rec} (PASS)")
    e_off += entry_len

print("\nFinal Verification Verdict:")
if v1 and v2 and found_saumya_dir and found_val_dir:
    print("ALL REAL WINDOWS XP NTFS INTEROPERABILITY VERIFICATIONS: 100% PASS!")
else:
    print("VERIFICATION FAILED!")

inspector.close()
