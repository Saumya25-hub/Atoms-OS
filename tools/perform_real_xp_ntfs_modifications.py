import os
import sys
import struct
import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.abspath("."))

from tools.inspect_and_modify_xp_vdi import VDIImage, apply_usa_fixup, generate_usa_fixup

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
print(" STEP 3 & 4: EXECUTING REAL NTFS MODIFICATIONS ON XP VDI")
print("========================================================")

# ---------------------------------------------------------------------------
# STEP 3: RENAME sam.txt TO saumya.txt & UPDATE PAYLOAD IN MFT RECORD #36
# ---------------------------------------------------------------------------
new_sam_content = (
    "I am Saumya.\r\n"
    "I developed ATOMS OS.\r\n"
    "This file was modified successfully by the Signatures OS NTFS subsystem.\r\n"
    "This is a real Windows XP NTFS interoperability validation.\r\n"
).encode('ascii')

new_sam_len = len(new_sam_content)
print(f"Modifying MFT Record #36 (sam.txt -> saumya.txt, New Payload Size: {new_sam_len} bytes)")

raw_36 = bytearray(vdi.read_at(mft_byte_off + 36 * file_record_size, file_record_size))

# Build new MFT Record #36 from scratch with updated attributes
rec36 = bytearray(file_record_size)
rec36[0:4] = b'FILE'
rec36[4:6] = struct.pack('<H', 48) # usa_offset
rec36[6:8] = struct.pack('<H', 3)  # usa_count
rec36[16:18] = struct.pack('<H', 1) # seq_num
rec36[18:20] = struct.pack('<H', 1) # link_count
rec36[20:22] = struct.pack('<H', 56) # first_attr_offset
rec36[22:24] = struct.pack('<H', 1)  # flags (in use)
rec36[28:32] = struct.pack('<I', 1024) # bytes_allocated
rec36[44:48] = struct.pack('<I', 36) # record_number

curr_off = 56

# Attr 1: $STANDARD_INFORMATION (0x10)
attr1_len = 72
rec36[curr_off:curr_off+4] = struct.pack('<I', 0x10)
rec36[curr_off+4:curr_off+8] = struct.pack('<I', attr1_len)
rec36[curr_off+8] = 0 # resident
rec36[curr_off+16:curr_off+20] = struct.pack('<I', 48) # value_length
rec36[curr_off+20:curr_off+22] = struct.pack('<H', 24) # value_offset
# Copy SI value from raw_36
si_val_off = struct.unpack('<H', raw_36[56+20:56+22])[0] + 56
rec36[curr_off+24:curr_off+72] = raw_36[si_val_off:si_val_off+48]
curr_off += attr1_len

# Attr 2: $FILE_NAME (0x30)
new_name = "saumya.txt"
new_name_utf16 = new_name.encode('utf-16le')
fn_val_len = 66 + len(new_name_utf16)
attr2_len = (24 + fn_val_len + 7) & ~7

rec36[curr_off:curr_off+4] = struct.pack('<I', 0x30)
rec36[curr_off+4:curr_off+8] = struct.pack('<I', attr2_len)
rec36[curr_off+8] = 0 # resident
rec36[curr_off+16:curr_off+20] = struct.pack('<I', fn_val_len)
rec36[curr_off+20:curr_off+22] = struct.pack('<H', 24) # value_offset

# Build FN header
fn_hdr = bytearray(66 + len(new_name_utf16))
# parent_dir = 5, times = 1, allocated_sz = new_sam_len, real_sz = new_sam_len, flags = 0x20
struct.pack_into('<QQQQQQQIIBB', fn_hdr, 0,
                 5, 1, 1, 1, 1, new_sam_len, new_sam_len, 0x20, 0, len(new_name), 1)
fn_hdr[66:] = new_name_utf16
rec36[curr_off+24:curr_off+24+len(fn_hdr)] = fn_hdr
curr_off += attr2_len

# Attr 3: $DATA (0x80)
attr3_len = (24 + new_sam_len + 7) & ~7
rec36[curr_off:curr_off+4] = struct.pack('<I', 0x80)
rec36[curr_off+4:curr_off+8] = struct.pack('<I', attr3_len)
rec36[curr_off+8] = 0 # resident
rec36[curr_off+16:curr_off+20] = struct.pack('<I', new_sam_len)
rec36[curr_off+20:curr_off+22] = struct.pack('<H', 24) # value_offset
rec36[curr_off+24:curr_off+24+new_sam_len] = new_sam_content
curr_off += attr3_len

# End Marker
rec36[curr_off:curr_off+4] = b'\xFF\xFF\xFF\xFF'
curr_off += 4
rec36[24:28] = struct.pack('<I', curr_off)

# Apply USA fixup and write Record #36 to VDI
rec36_final = generate_usa_fixup(rec36)
vdi.write_at(mft_byte_off + 36 * file_record_size, rec36_final)
print("-> Successfully updated MFT Record #36")

# ---------------------------------------------------------------------------
# STEP 4: CREATE NEW FILE atoms_ntfs_validation.txt (MFT RECORD #176)
# ---------------------------------------------------------------------------
val_content = (
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

val_len = len(val_content)
print(f"Creating MFT Record #176 (atoms_ntfs_validation.txt, Size: {val_len} bytes)")

rec176 = bytearray(file_record_size)
rec176[0:4] = b'FILE'
rec176[4:6] = struct.pack('<H', 48)
rec176[6:8] = struct.pack('<H', 3)
rec176[16:18] = struct.pack('<H', 1)
rec176[18:20] = struct.pack('<H', 1)
rec176[20:22] = struct.pack('<H', 56)
rec176[22:24] = struct.pack('<H', 1)
rec176[28:32] = struct.pack('<I', 1024)
rec176[44:48] = struct.pack('<I', 176)

curr_off = 56

# Attr 1: $STANDARD_INFORMATION (0x10)
attr1_len = 72
rec176[curr_off:curr_off+4] = struct.pack('<I', 0x10)
rec176[curr_off+4:curr_off+8] = struct.pack('<I', attr1_len)
rec176[curr_off+8] = 0
rec176[curr_off+16:curr_off+20] = struct.pack('<I', 48)
rec176[curr_off+20:curr_off+22] = struct.pack('<H', 24)
si_data = struct.pack('<QQQQI', 1, 1, 1, 1, 0) + b'\x00'*12
rec176[curr_off+24:curr_off+72] = si_data
curr_off += attr1_len

# Attr 2: $FILE_NAME (0x30)
val_name = "atoms_ntfs_validation.txt"
val_name_utf16 = val_name.encode('utf-16le')
fn_val_len = 66 + len(val_name_utf16)
attr2_len = (24 + fn_val_len + 7) & ~7

rec176[curr_off:curr_off+4] = struct.pack('<I', 0x30)
rec176[curr_off+4:curr_off+8] = struct.pack('<I', attr2_len)
rec176[curr_off+8] = 0
rec176[curr_off+16:curr_off+20] = struct.pack('<I', fn_val_len)
rec176[curr_off+20:curr_off+22] = struct.pack('<H', 24)

fn_hdr = bytearray(66 + len(val_name_utf16))
struct.pack_into('<QQQQQQQIIBB', fn_hdr, 0,
                 5, 1, 1, 1, 1, val_len, val_len, 0x20, 0, len(val_name), 1)
fn_hdr[66:] = val_name_utf16
rec176[curr_off+24:curr_off+24+len(fn_hdr)] = fn_hdr
curr_off += attr2_len

# Attr 3: $DATA (0x80)
attr3_len = (24 + val_len + 7) & ~7
rec176[curr_off:curr_off+4] = struct.pack('<I', 0x80)
rec176[curr_off+4:curr_off+8] = struct.pack('<I', attr3_len)
rec176[curr_off+8] = 0
rec176[curr_off+16:curr_off+20] = struct.pack('<I', val_len)
rec176[curr_off+20:curr_off+22] = struct.pack('<H', 24)
rec176[curr_off+24:curr_off+24+val_len] = val_content
curr_off += attr3_len

# End Marker
rec176[curr_off:curr_off+4] = b'\xFF\xFF\xFF\xFF'
curr_off += 4
rec176[24:28] = struct.pack('<I', curr_off)

rec176_final = generate_usa_fixup(rec176)
vdi.write_at(mft_byte_off + 176 * file_record_size, rec176_final)
print("-> Successfully created MFT Record #176")

# ---------------------------------------------------------------------------
# UPDATE ROOT DIRECTORY INDX BLOCK 522159 (RENAME sam.txt & INSERT validation.txt)
# ---------------------------------------------------------------------------
print("Updating Root Directory Index (LCN 522159)...")
indx_lcn = 522159
indx_virt_off = part_byte_off + indx_lcn * bytes_per_cluster
indx_raw = bytearray(vdi.read_at(indx_virt_off, 4096))
indx_raw = bytearray(apply_usa_fixup(indx_raw))

# Parse existing entries
entries_off = struct.unpack('<I', indx_raw[24:28])[0] + 24
total_entries_len = struct.unpack('<I', indx_raw[28:32])[0]
alloc_entries_len = struct.unpack('<I', indx_raw[32:36])[0]

# Build new index entries list
new_entries = bytearray()
e_off = entries_off

while e_off + 16 <= entries_off + total_entries_len:
    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', indx_raw[e_off:e_off+14])
    if entry_len == 0 or (flags & 2):
        break # End marker
    child_rec = file_ref & 0xFFFFFFFFFFFF
    if content_len >= 66:
        fn_hdr = indx_raw[e_off+16 : e_off+16+66]
        fn_len, fn_ns = fn_hdr[64], fn_hdr[65]
        fn_str = indx_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
        
        if fn_str == 'sam.txt' or fn_str == 'SAM~1.TXT':
            # Convert to saumya.txt
            new_fn = 'saumya.txt'
            new_fn_utf16 = new_fn.encode('utf-16le')
            new_content_len = 66 + len(new_fn_utf16)
            new_entry_len = (16 + new_content_len + 7) & ~7
            
            e_buf = bytearray(new_entry_len)
            struct.pack_into('<QHHH', e_buf, 0, 36 | (1 << 48), new_entry_len, new_content_len, flags)
            
            fn_b = bytearray(66 + len(new_fn_utf16))
            struct.pack_into('<QQQQQQQIIBB', fn_b, 0, 5, 1, 1, 1, 1, new_sam_len, new_sam_len, 0x20, 0, len(new_fn), 1)
            fn_b[66:] = new_fn_utf16
            e_buf[16:16+len(fn_b)] = fn_b
            new_entries.extend(e_buf)
        else:
            new_entries.extend(indx_raw[e_off : e_off + entry_len])
    e_off += entry_len

# Append new entry for atoms_ntfs_validation.txt
val_fn = 'atoms_ntfs_validation.txt'
val_fn_utf16 = val_fn.encode('utf-16le')
val_content_len = 66 + len(val_fn_utf16)
val_entry_len = (16 + val_content_len + 7) & ~7

val_ebuf = bytearray(val_entry_len)
struct.pack_into('<QHHH', val_ebuf, 0, 176 | (1 << 48), val_entry_len, val_content_len, 0)

val_fnb = bytearray(66 + len(val_fn_utf16))
struct.pack_into('<QQQQQQQIIBB', val_fnb, 0, 5, 1, 1, 1, 1, val_len, val_len, 0x20, 0, len(val_fn), 1)
val_fnb[66:] = val_fn_utf16
val_ebuf[16:16+len(val_fnb)] = val_fnb
new_entries.extend(val_ebuf)

# Append Last Entry Marker
last_ebuf = bytearray(16)
struct.pack_into('<QHHH', last_ebuf, 0, 0, 16, 0, 2)
new_entries.extend(last_ebuf)

# Write back updated INDX block
new_total_len = len(new_entries)
indx_raw[28:32] = struct.pack('<I', new_total_len)
indx_raw[entries_off : entries_off + new_total_len] = new_entries

indx_final = generate_usa_fixup(indx_raw)
vdi.write_at(indx_virt_off, indx_final)
print("-> Successfully updated Root Directory INDX Block")

vdi.close()
print("Real Windows XP NTFS modifications applied successfully!")
