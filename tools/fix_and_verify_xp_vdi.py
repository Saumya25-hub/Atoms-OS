import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import VDIImage, generate_usa_fixup, apply_usa_fixup, NTFSForensicInspector

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
mft_byte_off = part_byte_off + mft_lcn * bytes_per_cluster

print("========================================================")
print(" APPLYING EXACT FORENSIC FIX FOR WINDOWS XP CHKDSK")
print("========================================================")

def fix_mft_record(rec_num, name, payload_bytes):
    print(f"Fixing MFT Record #{rec_num} ({name})...")
    raw = bytearray(vdi.read_at(mft_byte_off + rec_num * file_record_size, file_record_size))
    raw = bytearray(apply_usa_fixup(raw))
    
    # 1. Set next_attribute_id at offset 40..42 to 4
    raw[40:42] = struct.pack('<H', 4)
    
    # 2. Update Attribute IDs for 0x10, 0x30, 0x80
    first_attr_off = struct.unpack('<H', raw[20:22])[0]
    bytes_in_use = struct.unpack('<I', raw[24:28])[0]
    off = first_attr_off
    
    attr_id_counter = 1
    while off + 4 <= bytes_in_use:
        atype, alen = struct.unpack('<II', raw[off:off+8])
        if atype == 0xFFFFFFFF or alen == 0: break
        
        # Set attribute_id at offset 14..16 of attribute header (off + 14)
        raw[off+14:off+16] = struct.pack('<H', attr_id_counter)
        print(f"  Attr 0x{atype:02X} at offset {off} -> Assigned Attribute ID = {attr_id_counter}")
        attr_id_counter += 1
        
        off += alen
        
    # Re-apply USA fixup and write back to VDI
    raw_final = generate_usa_fixup(raw)
    vdi.write_at(mft_byte_off + rec_num * file_record_size, raw_final)
    print(f"-> Successfully updated MFT Record #{rec_num}")

fix_mft_record(36, "saumya.txt", b"")
fix_mft_record(176, "atoms_ntfs_validation.txt", b"")

vdi.close()
print("\nAll Windows XP CHKDSK attribute ID fixes applied successfully!")
