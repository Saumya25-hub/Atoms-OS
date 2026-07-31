import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

def dump_record_forensics(rec_num):
    print(f"\n========================================================")
    print(f" FORENSIC ANALYSIS FOR MFT RECORD #{rec_num}")
    print(f"========================================================")
    raw = inspector.vdi.read_at(inspector.mft_byte_off + rec_num * inspector.file_record_size, 1024)
    
    magic = raw[0:4]
    usa_off, usa_cnt = struct.unpack('<HH', raw[4:8])
    lsn = struct.unpack('<Q', raw[8:16])[0]
    seq_num, link_cnt = struct.unpack('<HH', raw[16:20])
    first_attr_off, flags = struct.unpack('<HH', raw[20:24])
    bytes_in_use, bytes_alloc = struct.unpack('<II', raw[24:32])
    base_ref = struct.unpack('<Q', raw[32:40])[0]
    next_attr_id = struct.unpack('<H', raw[40:42])[0]

    print(f"Header Field Analysis:")
    print(f"  Magic                : {magic} (Expected b'FILE')")
    print(f"  USA Offset           : {usa_off} (0x{usa_off:04X})")
    print(f"  USA Count            : {usa_cnt}")
    print(f"  LSN                  : {lsn}")
    print(f"  Sequence Number      : {seq_num}")
    print(f"  Link Count           : {link_cnt}")
    print(f"  First Attr Offset    : {first_attr_off} (0x{first_attr_off:04X})")
    print(f"  Flags                : 0x{flags:04X}")
    print(f"  Bytes In Use         : {bytes_in_use} (0x{bytes_in_use:04X})")
    print(f"  Bytes Allocated       : {bytes_alloc} (0x{bytes_alloc:04X})")
    print(f"  Next Attr ID         : {next_attr_id}")
    
    # Check USA Array
    print(f"\nUSA Array Analysis (at offset {usa_off}):")
    usn = struct.unpack('<H', raw[usa_off:usa_off+2])[0]
    print(f"  USN                  : 0x{usn:04X}")
    for i in range(1, usa_cnt):
        saved_trailer = struct.unpack('<H', raw[usa_off+i*2:usa_off+i*2+2])[0]
        actual_trailer = struct.unpack('<H', raw[i*512-2:i*512])[0]
        print(f"  Sector {i-1} Trailer: Actual=0x{actual_trailer:04X}, Saved=0x{saved_trailer:04X} {'[MATCH]' if actual_trailer == usn else '[MISMATCH!]'}")

    # Check Attributes
    print(f"\nAttributes Breakdown:")
    off = first_attr_off
    attr_idx = 0
    while off + 4 <= bytes_in_use:
        atype, alen = struct.unpack('<II', raw[off:off+8])
        if atype == 0xFFFFFFFF:
            print(f"  Attr #{attr_idx}: 0xFFFFFFFF (END MARKER) at offset {off} (0x{off:04X})")
            if off + 4 != bytes_in_use:
                print(f"  WARNING: End marker offset {off+4} != Bytes In Use {bytes_in_use}!")
            break
        if alen == 0:
            print(f"  ERROR: Attr #{attr_idx} has length 0 at offset {off}!")
            break
            
        non_res, nlen, n_off, aflags, attr_id = struct.unpack('<BBHHH', raw[off+8:off+16])
        alignment_valid = (alen % 8 == 0)
        
        print(f"  Attr #{attr_idx}: Type=0x{atype:02X}, Len={alen} (0x{alen:04X}), NonRes={non_res}, AttrID={attr_id}, 8-Byte Aligned={'YES' if alignment_valid else 'NO [ERROR!]'}")
        
        if not non_res: # Resident
            vlen, voff = struct.unpack('<IH', raw[off+16:off+22])[0], struct.unpack('<H', raw[off+20:off+22])[0]
            print(f"    Resident Value: Len={vlen}, ValueOffset={voff} (Absolute: {off+voff})")
            if atype == 0x10 and vlen != 48:
                print(f"    ERROR: $STANDARD_INFORMATION resident value length {vlen} != 48!")
            elif atype == 0x30:
                fn_ns = raw[off+voff+65] if vlen >= 66 else -1
                print(f"    $FILE_NAME Namespace: {fn_ns}")
        else: # Non-Resident
            alloc_sz, real_sz, init_sz = struct.unpack('<QQQ', raw[off+40:off+64])
            print(f"    Non-Resident Value: RealSize={real_sz}, AllocSize={alloc_sz}")
            
        off += alen
        attr_idx += 1

dump_record_forensics(169) # Original Windows XP valid record
dump_record_forensics(36)  # Modified Record 36
dump_record_forensics(176) # Modified Record 176

inspector.close()
