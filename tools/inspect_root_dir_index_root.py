import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector, apply_usa_fixup

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

def inspect_record_5():
    raw = inspector.vdi.read_at(inspector.mft_byte_off + 5 * 1024, 1024)
    first_attr_off = struct.unpack('<H', raw[20:22])[0]
    bytes_in_use = struct.unpack('<I', raw[24:28])[0]
    off = first_attr_off
    
    print("========================================================")
    print(" INSPECTING RECORD 5 ($INDEX_ROOT vs $INDEX_ALLOCATION)")
    print("========================================================")
    
    while off + 4 <= bytes_in_use:
        atype = struct.unpack('<I', raw[off:off+4])[0]
        if atype == 0xFFFFFFFF: break
        alen = struct.unpack('<H', raw[off+4:off+6])[0]
        if alen == 0: break
        
        if atype == 0x90: # $INDEX_ROOT
            voff = struct.unpack('<H', raw[off+20:off+22])[0]
            val = raw[off+voff : off+alen]
            idx_flags = val[12]
            print(f"Attr 0x90 ($INDEX_ROOT): Len={alen}, ValOff={voff}, Flags={idx_flags} (1=Large/Allocation)")
            idx_hdr_off = 16
            entries_off = struct.unpack('<I', val[idx_hdr_off:idx_hdr_off+4])[0] + idx_hdr_off
            total_sz = struct.unpack('<I', val[idx_hdr_off+4:idx_hdr_off+8])[0]
            print(f"  Root Entries Offset={entries_off}, Total Size={total_sz}")
            
            e_off = entries_off
            print("  Root Index Entries:")
            while e_off + 16 <= len(val):
                file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', val[e_off:e_off+14])
                if entry_len == 0: break
                child_rec = file_ref & 0xFFFFFFFFFFFF
                if content_len >= 66:
                    fn_hdr = val[e_off+16 : e_off+16+66]
                    fn_len = fn_hdr[64]
                    fn_str = val[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
                    print(f"    - Entry: '{fn_str}' -> MFT #{child_rec} (flags={flags})")
                else:
                    subnode_vcn = struct.unpack('<Q', val[e_off+entry_len-8:e_off+entry_len])[0] if flags & 1 else -1
                    print(f"    - Dummy/End Entry (flags={flags}, subnode_vcn={subnode_vcn})")
                if flags & 2: break
                e_off += entry_len

        elif atype == 0xA0: # $INDEX_ALLOCATION
            print(f"Attr 0xA0 ($INDEX_ALLOCATION): Len={alen}")

        off += alen

inspect_record_5()
inspector.close()
