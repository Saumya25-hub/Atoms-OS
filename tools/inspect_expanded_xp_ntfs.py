import struct

raw_path = r'd:\Signatures_OS\build\winxp_ntfs_real.raw'

def decode_runlist(run_data):
    pos = 0
    runs = []
    current_lcn = 0
    current_vcn = 0
    
    while pos < len(run_data):
        header = run_data[pos]
        pos += 1
        if header == 0:
            break
        
        len_bytes = header & 0x0F
        off_bytes = (header >> 4) & 0x0F
        
        if len_bytes == 0 or len_bytes > 8 or off_bytes > 8 or pos + len_bytes + off_bytes > len(run_data):
            break
            
        run_len = 0
        for i in range(len_bytes):
            run_len |= run_data[pos] << (i * 8)
            pos += 1
            
        lcn_delta = 0
        if off_bytes > 0:
            for i in range(off_bytes):
                lcn_delta |= run_data[pos] << (i * 8)
                pos += 1
            if run_data[pos - 1] & 0x80:
                lcn_delta |= (-1 << (off_bytes * 8))
            current_lcn += lcn_delta
            runs.append({'vcn': current_vcn, 'lcn': current_lcn, 'length': run_len, 'sparse': False})
        else:
            runs.append({'vcn': current_vcn, 'lcn': None, 'length': run_len, 'sparse': True})
            
        current_vcn += run_len

    return runs

with open(raw_path, 'rb') as f:
    mbr = f.read(512)
    p1 = mbr[446:462]
    lba_start = struct.unpack('<I', p1[8:12])[0]
    p_type = p1[4]
    
    f.seek(lba_start * 512)
    boot = f.read(512)
    spc = boot[13]
    bps = struct.unpack('<H', boot[11:13])[0]
    mft_lcn = struct.unpack('<Q', boot[48:56])[0]
    mft_offset = (lba_start + mft_lcn * spc) * bps
    
    print(f"=== EXPANDED WINDOWS XP NTFS MFT RUNLIST AUDIT ===")
    print(f"Partition 1 Type: 0x{p_type:02X}, LBA Start: {lba_start}")
    print(f"Bytes/Sector: {bps}, Sectors/Cluster: {spc}, Cluster Size: {spc * bps} bytes")
    print(f"MFT LCN: {mft_lcn}, MFT Byte Offset: {mft_offset}\n")
    
    for i in range(512):
        f.seek(mft_offset + i * 1024)
        rec = f.read(1024)
        if rec[:4] != b'FILE':
            continue
            
        flags = struct.unpack('<H', rec[22:24])[0]
        in_use = bool(flags & 1)
        is_dir = bool(flags & 2)
        if not in_use:
            continue
            
        first_attr = struct.unpack('<H', rec[20:22])[0]
        off = first_attr
        filenames = []
        data_records = []
        
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
            elif a_type == 0x80: # DATA
                is_nonres = rec[off+8]
                if not is_nonres:
                    d_len = struct.unpack('<I', rec[off+16:off+20])[0]
                    d_off = struct.unpack('<H', rec[off+20:off+22])[0]
                    content = rec[off+d_off : off+d_off+min(d_len, 32)]
                    data_records.append({
                        'non_resident': False,
                        'size': d_len,
                        'runs': []
                    })
                else:
                    alloc_sz = struct.unpack('<Q', rec[off+40:off+48])[0]
                    data_sz = struct.unpack('<Q', rec[off+48:off+56])[0]
                    init_sz = struct.unpack('<Q', rec[off+56:off+64])[0]
                    run_off = struct.unpack('<H', rec[off+32:off+34])[0]
                    run_data = rec[off+run_off : off+a_len]
                    runs = decode_runlist(run_data)
                    data_records.append({
                        'non_resident': True,
                        'size': data_sz,
                        'alloc_size': alloc_sz,
                        'init_size': init_sz,
                        'runs': runs
                    })
            off += a_len
            
        if filenames:
            for fn in filenames:
                # Filter out system meta-files for clear view if desired
                print(f"Record {i:3d} | File: {fn:40s} | Dir: {is_dir}")
                for dr in data_records:
                    if not dr['non_resident']:
                        print(f"   -> RESIDENT DATA | Size: {dr['size']} bytes")
                    else:
                        r_count = len(dr['runs'])
                        ext_type = "MULTIPLE EXTENTS (FRAGMENTED)" if r_count > 1 else "SINGLE EXTENT (CONTIGUOUS)"
                        print(f"   -> NON-RESIDENT DATA | Size: {dr['size']} bytes | Alloc: {dr['alloc_size']} bytes | Runs: {r_count} [{ext_type}]")
                        for idx, r in enumerate(dr['runs']):
                            print(f"        Run #{idx+1}: VCN {r['vcn']} -> LCN {r['lcn']} ({r['length']} clusters = {r['length']*spc*bps} bytes)")
