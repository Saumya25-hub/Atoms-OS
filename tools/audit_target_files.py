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
        if header == 0: break
        len_bytes = header & 0x0F
        off_bytes = (header >> 4) & 0x0F
        if len_bytes == 0 or len_bytes > 8 or off_bytes > 8 or pos + len_bytes + off_bytes > len(run_data): break
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
            runs.append({'vcn': current_vcn, 'lcn': current_lcn, 'length': run_len})
        else:
            runs.append({'vcn': current_vcn, 'lcn': None, 'length': run_len})
        current_vcn += run_len
    return runs

with open(raw_path, 'rb') as f:
    mbr = f.read(512)
    p1 = mbr[446:462]
    lba_start = struct.unpack('<I', p1[8:12])[0]
    f.seek(lba_start * 512)
    boot = f.read(512)
    spc = boot[13]
    bps = struct.unpack('<H', boot[11:13])[0]
    mft_lcn = struct.unpack('<Q', boot[48:56])[0]
    mft_offset = (lba_start + mft_lcn * spc) * bps

    print("=== TARGET FILES RUNLIST FORENSIC REPORT ===")
    for i in range(1024):
        f.seek(mft_offset + i * 1024)
        rec = f.read(1024)
        if rec[:4] != b'FILE': continue
        flags = struct.unpack('<H', rec[22:24])[0]
        if not (flags & 1): continue
        
        first_attr = struct.unpack('<H', rec[20:22])[0]
        off = first_attr
        names = []
        data_records = []
        while off < 1000:
            a_type = struct.unpack('<I', rec[off:off+4])[0]
            a_len = struct.unpack('<I', rec[off+4:off+8])[0]
            if a_type == 0xFFFFFFFF or a_len == 0: break
            if a_type == 0x30:
                res_off = struct.unpack('<H', rec[off+20:off+22])[0]
                fn_data = rec[off+res_off:]
                fn_len = fn_data[64]
                names.append(fn_data[66 : 66 + fn_len*2].decode('utf-16le', errors='ignore'))
            elif a_type == 0x80:
                is_nonres = rec[off+8]
                if not is_nonres:
                    d_len = struct.unpack('<I', rec[off+16:off+20])[0]
                    data_records.append({'non_resident': False, 'size': d_len, 'runs': []})
                else:
                    data_sz = struct.unpack('<Q', rec[off+48:off+56])[0]
                    run_off = struct.unpack('<H', rec[off+32:off+34])[0]
                    runs = decode_runlist(rec[off+run_off : off+a_len])
                    data_records.append({'non_resident': True, 'size': data_sz, 'runs': runs})
            off += a_len
            
        for fn in names:
            upper_fn = fn.upper()
            if any(k in upper_fn for k in ['BIG1', 'FRAG', 'MEDIUM', 'LARGE', 'HUG', 'HELLO', 'OS KERNAL', 'TEST', 'MANY']):
                print(f"\n[FILE] Name: '{fn}' | MFT Record: {i}")
                for dr in data_records:
                    if not dr['non_resident']:
                        print(f"  Type       : RESIDENT")
                        print(f"  Size       : {dr['size']} bytes")
                        print(f"  Run Count  : 0")
                        print(f"  Extent     : N/A (Resident in MFT Record)")
                    else:
                        r_cnt = len(dr['runs'])
                        ext_type = "MULTIPLE EXTENTS (FRAGMENTED)" if r_cnt > 1 else "SINGLE EXTENT (CONTIGUOUS)"
                        print(f"  Type       : NON-RESIDENT")
                        print(f"  Size       : {dr['size']} bytes ({dr['size'] / 1024:.2f} KB / {dr['size'] / (1024*1024):.2f} MB)")
                        print(f"  Run Count  : {r_cnt}")
                        print(f"  Extent     : {ext_type}")
                        for idx, r in enumerate(dr['runs']):
                            print(f"    Run #{idx+1}: VCN {r['vcn']} -> LCN {r['lcn']} ({r['length']} clusters = {r['length']*spc*bps} bytes)")
