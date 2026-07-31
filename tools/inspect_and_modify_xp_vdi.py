import os
import sys
import struct
import datetime

# ---------------------------------------------------------------------------
# VDI (VirtualBox Disk Image) Read/Write Driver
# ---------------------------------------------------------------------------
class VDIImage:
    def __init__(self, filepath):
        self.filepath = filepath
        self.f = open(filepath, 'r+b')
        self.f.seek(0x154)
        self.off_blocks, self.off_data = struct.unpack('<II', self.f.read(8))
        self.f.seek(0x170)
        self.disk_size = struct.unpack('<Q', self.f.read(8))[0]
        self.block_size = struct.unpack('<I', self.f.read(4))[0]
        self.block_extra = struct.unpack('<I', self.f.read(4))[0]
        self.total_blocks = struct.unpack('<I', self.f.read(4))[0]
        self.alloc_blocks = struct.unpack('<I', self.f.read(4))[0]

    def read_at(self, virt_offset, length):
        result = bytearray()
        remaining = length
        curr = virt_offset
        while remaining > 0:
            blk_idx = curr // self.block_size
            blk_off = curr % self.block_size
            to_read = min(remaining, self.block_size - blk_off)

            self.f.seek(self.off_blocks + blk_idx * 4)
            bat_entry = struct.unpack('<I', self.f.read(4))[0]

            if bat_entry == 0xFFFFFFFF:
                result.extend(b'\x00' * to_read)
            else:
                phys_off = self.off_data + bat_entry * (self.block_size + self.block_extra) + blk_off
                self.f.seek(phys_off)
                result.extend(self.f.read(to_read))

            curr += to_read
            remaining -= to_read
        return bytes(result)

    def write_at(self, virt_offset, data):
        remaining = len(data)
        curr = virt_offset
        data_off = 0
        while remaining > 0:
            blk_idx = curr // self.block_size
            blk_off = curr % self.block_size
            to_write = min(remaining, self.block_size - blk_off)

            self.f.seek(self.off_blocks + blk_idx * 4)
            bat_entry = struct.unpack('<I', self.f.read(4))[0]

            if bat_entry == 0xFFFFFFFF:
                # Allocate new block in VDI
                bat_entry = self.alloc_blocks
                self.alloc_blocks += 1
                # Update alloc_blocks in header
                self.f.seek(0x184)
                self.f.write(struct.pack('<I', self.alloc_blocks))
                # Update BAT entry
                self.f.seek(self.off_blocks + blk_idx * 4)
                self.f.write(struct.pack('<I', bat_entry))
                # Initialize new block with zeros
                phys_off = self.off_data + bat_entry * (self.block_size + self.block_extra)
                self.f.seek(phys_off)
                self.f.write(b'\x00' * self.block_size)

            phys_off = self.off_data + bat_entry * (self.block_size + self.block_extra) + blk_off
            self.f.seek(phys_off)
            self.f.write(data[data_off:data_off + to_write])

            curr += to_write
            data_off += to_write
            remaining -= to_write
        self.f.flush()

    def close(self):
        self.f.close()

# ---------------------------------------------------------------------------
# Helper Functions
# ---------------------------------------------------------------------------
def ntfs_time_to_datetime(ntfs_time):
    if ntfs_time == 0:
        return "N/A"
    try:
        # NTFS timestamp is 100-nanosecond intervals since Jan 1, 1601 UTC
        seconds = (ntfs_time - 116444736000000000) / 10000000.0
        dt = datetime.datetime.fromtimestamp(seconds, datetime.timezone.utc)
        return dt.strftime("%Y-%m-%d %H:%M:%S UTC")
    except Exception:
        return f"0x{ntfs_time:016x}"

def apply_usa_fixup(buf):
    rec = bytearray(buf)
    usa_off = struct.unpack('<H', rec[4:6])[0]
    usa_cnt = struct.unpack('<H', rec[6:8])[0]
    if usa_cnt <= 1:
        return bytes(rec)
    usn = rec[usa_off:usa_off+2]
    for i in range(1, usa_cnt):
        sector_trailer_off = i * 512 - 2
        rec[sector_trailer_off:sector_trailer_off+2] = rec[usa_off + i*2 : usa_off + i*2 + 2]
    return bytes(rec)

def generate_usa_fixup(buf):
    rec = bytearray(buf)
    usa_off = struct.unpack('<H', rec[4:6])[0]
    usa_cnt = struct.unpack('<H', rec[6:8])[0]
    if usa_cnt <= 1:
        return bytes(rec)
    
    usn = struct.unpack('<H', rec[usa_off:usa_off+2])[0]
    usn = (usn + 1) & 0xFFFF
    if usn == 0: usn = 1
    rec[usa_off:usa_off+2] = struct.pack('<H', usn)

    for i in range(1, usa_cnt):
        sector_trailer_off = i * 512 - 2
        orig_val = rec[sector_trailer_off:sector_trailer_off+2]
        rec[usa_off + i*2 : usa_off + i*2 + 2] = orig_val
        rec[sector_trailer_off:sector_trailer_off+2] = struct.pack('<H', usn)
    return bytes(rec)

# ---------------------------------------------------------------------------
# NTFS Volume Forensic Inspector
# ---------------------------------------------------------------------------
class NTFSForensicInspector:
    def __init__(self, vdi_path):
        self.vdi = VDIImage(vdi_path)
        mbr = self.vdi.read_at(0, 512)
        part1 = mbr[446:462]
        _, _, ptype, _, self.lba_start, self.sector_count = struct.unpack('<B3sB3sII', part1)
        
        self.part_byte_off = self.lba_start * 512
        bpb = self.vdi.read_at(self.part_byte_off, 512)
        
        self.oem_id = bpb[3:11].decode('ascii', 'replace')
        self.bytes_per_sector = struct.unpack('<H', bpb[0x0B:0x0D])[0]
        self.sectors_per_cluster = bpb[0x0D]
        self.bytes_per_cluster = self.bytes_per_sector * self.sectors_per_cluster
        self.mft_lcn = struct.unpack('<Q', bpb[0x30:0x38])[0]
        self.mft_mirr_lcn = struct.unpack('<Q', bpb[0x38:0x40])[0]
        
        encoded_rec_sz = struct.unpack('<b', bpb[0x40:0x41])[0]
        if encoded_rec_sz < 0:
            self.file_record_size = 1 << (-encoded_rec_sz)
        else:
            self.file_record_size = encoded_rec_sz * self.bytes_per_cluster
            
        self.volume_serial = struct.unpack('<Q', bpb[0x48:0x50])[0]
        self.mft_byte_off = self.part_byte_off + self.mft_lcn * self.bytes_per_cluster

    def read_mft_record_raw(self, rec_num):
        raw_off = self.mft_byte_off + rec_num * self.file_record_size
        raw_buf = self.vdi.read_at(raw_off, self.file_record_size)
        if raw_buf[:4] != b'FILE':
            return None
        return apply_usa_fixup(raw_buf)

    def parse_mft_record(self, rec_num):
        raw = self.read_mft_record_raw(rec_num)
        if not raw: return None
        
        flags = struct.unpack('<H', raw[22:24])[0]
        bytes_in_use = struct.unpack('<I', raw[24:28])[0]
        seq_num = struct.unpack('<H', raw[16:18])[0]
        
        record_info = {
            'rec_num': rec_num,
            'flags': flags,
            'is_dir': bool(flags & 2),
            'is_in_use': bool(flags & 1),
            'bytes_in_use': bytes_in_use,
            'seq_num': seq_num,
            'filename': None,
            'parent_rec': 0,
            'file_size': 0,
            'timestamps': {},
            'data_resident': True,
            'runlist': [],
            'ads': [],
            'reparse_tag': None
        }

        first_attr_off = struct.unpack('<H', raw[20:22])[0]
        off = first_attr_off
        while off + 8 <= bytes_in_use:
            attr_type, attr_len, non_resident, name_len, name_off = struct.unpack('<IIBBB', raw[off:off+11])
            if attr_type == 0xFFFFFFFF or attr_len == 0:
                break
                
            if attr_type == 0x10: # $STANDARD_INFORMATION
                val_off = struct.unpack('<H', raw[off+20:off+22])[0]
                si = raw[off+val_off : off+val_off+32]
                cr, mo, mft_c, ac = struct.unpack('<QQQQ', si[:32])
                record_info['timestamps'] = {
                    'creation': ntfs_time_to_datetime(cr),
                    'modification': ntfs_time_to_datetime(mo),
                    'mft_change': ntfs_time_to_datetime(mft_c),
                    'access': ntfs_time_to_datetime(ac)
                }
            elif attr_type == 0x30: # $FILE_NAME
                val_off = struct.unpack('<H', raw[off+20:off+22])[0]
                fn_hdr = raw[off+val_off : off+val_off+66]
                p_ref, cr, mo, mft_c, ac, alloc_sz, real_sz, fflags, rtag, fn_len, fn_ns = struct.unpack('<QQQQQQQIIBB', fn_hdr[:66])
                p_rec = p_ref & 0xFFFFFFFFFFFF
                fn_bytes = raw[off+val_off+66 : off+val_off+66 + fn_len*2]
                fn_str = fn_bytes.decode('utf-16le', errors='replace')
                if fn_ns != 2 or record_info['filename'] is None:
                    record_info['filename'] = fn_str
                    record_info['parent_rec'] = p_rec
                    record_info['file_size'] = real_sz
            elif attr_type == 0x80: # $DATA
                if name_len > 0:
                    stream_name_bytes = raw[off+name_off : off+name_off + name_len*2]
                    record_info['ads'].append(stream_name_bytes.decode('utf-16le', errors='replace'))
                else:
                    record_info['data_resident'] = not non_resident
                    if non_resident:
                        record_info['file_size'] = struct.unpack('<Q', raw[off+48:off+56])[0]
            elif attr_type == 0xC0: # $REPARSE_POINT
                val_off = struct.unpack('<H', raw[off+20:off+22])[0]
                rtag = struct.unpack('<I', raw[off+val_off:off+val_off+4])[0]
                record_info['reparse_tag'] = f"0x{rtag:08x}"
                
            off += attr_len
            
        return record_info

    def get_total_mft_records(self):
        # MFT record 0 contains $MFT record itself
        mft_0 = self.parse_mft_record(0)
        return mft_0['file_size'] // self.file_record_size

    def enumerate_directory(self, dir_rec_num=5, current_path=""):
        items = []
        dir_info = self.parse_mft_record(dir_rec_num)
        if not dir_info or not dir_info['is_dir']: return items

        raw = self.read_mft_record_raw(dir_rec_num)
        off = struct.unpack('<H', raw[20:22])[0]
        
        entries = []
        while off + 8 <= dir_info['bytes_in_use']:
            attr_type, attr_len = struct.unpack('<II', raw[off:off+8])
            if attr_type == 0xFFFFFFFF or attr_len == 0: break
            
            if attr_type == 0x90: # $INDEX_ROOT
                val_off = struct.unpack('<H', raw[off+20:off+22])[0]
                ir_raw = raw[off+val_off:]
                entries_off = struct.unpack('<I', ir_raw[16:20])[0] + 16
                e_off = entries_off
                while e_off + 16 <= len(ir_raw):
                    file_ref, entry_len, content_len, flags = struct.unpack('<QHHH', ir_raw[e_off:e_off+14])
                    if entry_len == 0 or (flags & 2): break # Last entry marker
                    child_rec = file_ref & 0xFFFFFFFFFFFF
                    if content_len >= 66:
                        fn_hdr = ir_raw[e_off+16 : e_off+16+66]
                        p_ref, cr, mo, mft_c, ac, alloc_sz, real_sz, fflags, rtag, fn_len, fn_ns = struct.unpack('<QQQQQQQIIBB', fn_hdr[:66])
                        fn_str = ir_raw[e_off+16+66 : e_off+16+66+fn_len*2].decode('utf-16le', errors='replace')
                        if fn_ns != 2 and fn_str not in ('.', '..'):
                            entries.append((child_rec, fn_str, bool(fflags & 0x10), real_sz))
                    e_off += entry_len
            off += attr_len

        for child_rec, fn_str, is_dir, sz in entries:
            full_path = f"{current_path}/{fn_str}" if current_path else f"/{fn_str}"
            child_info = self.parse_mft_record(child_rec)
            items.append((full_path, child_rec, child_info))
            if is_dir and child_rec != dir_rec_num:
                sub_items = self.enumerate_directory(child_rec, full_path)
                items.extend(sub_items)
                
        return items

    def close(self):
        self.vdi.close()

# ---------------------------------------------------------------------------
# Main Execution Strategy
# ---------------------------------------------------------------------------
if __name__ == '__main__':
    vdi_path = r'D:\Signatures_OS\NTFS-SYSTEM[TESTS-VDI]\XP TEST (NTFS SYSTEM ATOMS)_1.vdi'
    inspector = NTFSForensicInspector(vdi_path)
    
    print("========================================================")
    print(" STEP 1: FORENSIC INSPECTION REPORT (REAL WINDOWS XP NTFS)")
    print("========================================================")
    print(f"OEM Identifier       : {inspector.oem_id}")
    print(f"Bytes Per Sector     : {inspector.bytes_per_sector}")
    print(f"Sectors Per Cluster  : {inspector.sectors_per_cluster}")
    print(f"Cluster Size         : {inspector.bytes_per_cluster} bytes")
    print(f"Volume Serial Number : 0x{inspector.volume_serial:016X}")
    print(f"FILE Record Size     : {inspector.file_record_size} bytes")
    print(f"$MFT Start LCN       : {inspector.mft_lcn} (Byte Offset: {inspector.mft_byte_off})")
    print(f"MFT Record Count     : {inspector.get_total_mft_records()}")
    
    print("\n--------------------------------------------------------")
    print(" COMPLETE DIRECTORY TREE (BEFORE ANY MODIFICATIONS)")
    print("--------------------------------------------------------")
    tree_items = inspector.enumerate_directory(5, "")
    for path, rec_num, info in tree_items:
        if info:
            dir_tag = "[DIR] " if info['is_dir'] else "[FILE]"
            res_tag = "Resident" if info['data_resident'] else "Non-Resident"
            print(f"{dir_tag:7s} MFT #{rec_num:3d} | {path:35s} | Size: {info['file_size']:10d} bytes | {res_tag} | Modified: {info['timestamps'].get('modification', 'N/A')}")
        else:
            print(f"[UNKN] MFT #{rec_num:3d} | {path:35s}")
            
    print("\n========================================================")
    print(" STEP 2: VERIFY TEST FILES LOCATION")
    print("========================================================")
    target_names = ["ATOMS OS", "ATOMS-TEST", "FRAG", "FRAG2", "BIG1.BIN", "FRAGMENT60.BIN", "os details.txt", "sam.txt"]
    found_targets = {}
    for name in target_names:
        found = False
        for path, rec_num, info in tree_items:
            if path.endswith("/" + name) or path == "/" + name:
                found_targets[name] = (path, rec_num, info)
                print(f"[FOUND] {name:15s} -> Path: {path:30s} (MFT Record #{rec_num})")
                found = True
                break
        if not found:
            print(f"[NOT FOUND] {name}")
            
    inspector.close()
