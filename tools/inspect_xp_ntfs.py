import struct

raw_path = r'd:\Signatures_OS\build\winxp_ntfs_real.raw'
with open(raw_path, 'rb') as f:
    mbr = f.read(512)
    p1 = mbr[446:462]
    lba_start = struct.unpack('<I', p1[8:12])[0]
    print(f"Partition 1 LBA Start: {lba_start}")
    f.seek(lba_start * 512)
    boot = f.read(512)
    spc = boot[13]
    bps = struct.unpack('<H', boot[11:13])[0]
    mft_lcn = struct.unpack('<Q', boot[48:56])[0]
    mft_offset = (lba_start + mft_lcn * spc) * bps
    print(f"MFT LBA Byte Offset: {mft_offset} (LCN {mft_lcn})")

    for i in range(100):
        f.seek(mft_offset + i * 1024)
        rec = f.read(1024)
        if rec[:4] != b'FILE':
            continue
        
        # Parse attributes in FILE record
        first_attr = struct.unpack('<H', rec[20:22])[0]
        off = first_attr
        filenames = []
        data_payload = None
        while off < 1000:
            a_type = struct.unpack('<I', rec[off:off+4])[0]
            a_len = struct.unpack('<I', rec[off+4:off+8])[0]
            if a_type == 0xFFFFFFFF or a_len == 0:
                break
            if a_type == 0x30: # FILE_NAME
                res_off = struct.unpack('<H', rec[off+20:off+22])[0]
                fn_data = rec[off+res_off:]
                fn_len = fn_data[64]
                fn_name = fn_data[66 : 66 + fn_len*2].decode('utf-16le', errors='ignore')
                filenames.append(fn_name)
            if a_type == 0x80: # DATA
                is_nonres = rec[off+8]
                if not is_nonres:
                    d_len = struct.unpack('<I', rec[off+16:off+20])[0]
                    d_off = struct.unpack('<H', rec[off+20:off+22])[0]
                    data_payload = rec[off+d_off : off+d_off+d_len]
            off += a_len

        if filenames:
            print(f"Record {i:2d}: Names={filenames} | DATA={data_payload}")
