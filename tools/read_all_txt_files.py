import struct

raw_path = r'd:\Signatures_OS\build\winxp_ntfs_real.raw'

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

    print("=== ALL TXT FILES READ FROM WINDOWS XP DISK ===")
    for i in range(1024):
        f.seek(mft_offset + i * 1024)
        rec = f.read(1024)
        if rec[:4] != b'FILE': continue
        flags = struct.unpack('<H', rec[22:24])[0]
        if not (flags & 1) or (flags & 2): continue
        
        off = struct.unpack('<H', rec[20:22])[0]
        names = []
        data_text = None
        data_size = 0
        while off < 1000:
            a_type = struct.unpack('<I', rec[off:off+4])[0]
            a_len = struct.unpack('<I', rec[off+4:off+8])[0]
            if a_type == 0xFFFFFFFF or a_len == 0: break
            if a_type == 0x30:
                res_off = struct.unpack('<H', rec[off+20:off+22])[0]
                fn_data = rec[off+res_off:]
                fn_len = fn_data[64]
                names.append(fn_data[66:66+fn_len*2].decode('utf-16le', errors='ignore'))
            elif a_type == 0x80:
                is_nonres = rec[off+8]
                if not is_nonres:
                    d_len = struct.unpack('<I', rec[off+16:off+20])[0]
                    d_off = struct.unpack('<H', rec[off+20:off+22])[0]
                    content = rec[off+d_off : off+d_off+d_len]
                    data_size = d_len
                    try:
                        data_text = content.decode('utf-8', errors='ignore').strip()
                    except:
                        data_text = str(content)
            off += a_len
            
        if any(fn.lower().endswith('.txt') for fn in names):
            print(f"Record {i:3d} | Names: {names} | Size: {data_size} B | Content: \"{data_text}\"")
