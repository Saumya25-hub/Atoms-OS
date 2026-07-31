import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import NTFSForensicInspector

vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
inspector = NTFSForensicInspector(vdi_path)

def extract_fn_attr(rec_num):
    raw = inspector.vdi.read_at(inspector.mft_byte_off + rec_num * 1024, 1024)
    first_attr_off = struct.unpack('<H', raw[20:22])[0]
    bytes_in_use = struct.unpack('<I', raw[24:28])[0]
    off = first_attr_off
    fn_attrs = []
    while off + 4 <= bytes_in_use:
        atype = struct.unpack('<I', raw[off:off+4])[0]
        if atype == 0xFFFFFFFF: break
        alen = struct.unpack('<H', raw[off+4:off+6])[0]
        if alen == 0: break
        if atype == 0x30:
            fn_attrs.append((off, alen, raw[off:off+alen]))
        off += alen
    return fn_attrs

def print_fn_breakdown(rec_num, name_label, fn_raw):
    hdr = fn_raw[:24]
    val = fn_raw[24:]
    print(f"\n========================================================")
    print(f" MFT RECORD #{rec_num} ({name_label}) - $FILE_NAME (0x30)")
    print(f"========================================================")
    
    atype, alen = struct.unpack('<IH', hdr[0:6])
    non_res, nlen, n_off, aflags, attr_id = struct.unpack('<BBHHH', hdr[8:16])
    vlen = struct.unpack('<I', hdr[16:20])[0]
    voff = struct.unpack('<H', hdr[20:22])[0]
    idx_flag = hdr[22]
    padding1 = hdr[23]
    
    print("ATTRIBUTE HEADER (24 Bytes):")
    print(f"  0x00 Type            : 0x{atype:08X} ({atype})")
    print(f"  0x04 Length          : {alen} bytes")
    print(f"  0x08 Non-Resident    : {non_res}")
    print(f"  0x09 Name Length     : {nlen}")
    print(f"  0x0A Name Offset     : {n_off}")
    print(f"  0x0C Flags           : 0x{aflags:04X}")
    print(f"  0x0E Attribute ID    : {attr_id}")
    print(f"  0x10 Value Length    : {vlen} bytes")
    print(f"  0x14 Value Offset    : {voff} bytes")
    print(f"  0x16 Indexed Flag    : {idx_flag} (Expected 1 for $FILE_NAME)")
    print(f"  0x17 Padding         : {padding1}")
    
    p_ref, cr, mo, mft_c, ac, alloc_sz, real_sz, fflags, rtag, fn_len, fn_ns = struct.unpack('<QQQQQQQIIBB', val[:66])
    p_rec = p_ref & 0xFFFFFFFFFFFF
    p_seq = (p_ref >> 48) & 0xFFFF
    fn_bytes = val[66 : 66 + fn_len*2]
    fn_str = fn_bytes.decode('utf-16le', errors='replace')
    pad_bytes = val[66 + fn_len*2 :]
    
    print("\nRESIDENT VALUE HEADER ($FILE_NAME Payload, 66+ Bytes):")
    print(f"  0x00 Parent Reference: Record #{p_rec}, Sequence #{p_seq} (Raw: 0x{p_ref:016X})")
    print(f"  0x08 Creation Time   : 0x{cr:016X}")
    print(f"  0x10 Mod Time        : 0x{mo:016X}")
    print(f"  0x18 MFT Change Time : 0x{mft_c:016X}")
    print(f"  0x20 Access Time     : 0x{ac:016X}")
    print(f"  0x28 Allocated Size  : {alloc_sz} bytes")
    print(f"  0x30 Real Size       : {real_sz} bytes")
    print(f"  0x38 File Flags      : 0x{fflags:08X}")
    print(f"  0x3C EA/Reparse Tag  : 0x{rtag:08X}")
    print(f"  0x40 Filename Length : {fn_len} chars")
    print(f"  0x41 Namespace       : {fn_ns} (1=Win32, 2=DOS, 3=Win32&DOS)")
    print(f"  0x42 Filename String : '{fn_str}'")
    print(f"  Padding Bytes        : {pad_bytes.hex()}")

# Print WinXP created record #169
fn_169 = extract_fn_attr(169)
for idx, (off, alen, buf) in enumerate(fn_169):
    print_fn_breakdown(169, f"WinXP os details.txt Attr #{idx}", buf)

# Print WinXP created record #31
fn_31 = extract_fn_attr(31)
for idx, (off, alen, buf) in enumerate(fn_31):
    print_fn_breakdown(31, f"WinXP FRAGMENT60.BIN Attr #{idx}", buf)

# Print Record #36
fn_36 = extract_fn_attr(36)
for idx, (off, alen, buf) in enumerate(fn_36):
    print_fn_breakdown(36, f"Modified saumya.txt Attr #{idx}", buf)

# Print Record #176
fn_176 = extract_fn_attr(176)
for idx, (off, alen, buf) in enumerate(fn_176):
    print_fn_breakdown(176, f"Created ATOMSTESTS.TXT Attr #{idx}", buf)

inspector.close()
