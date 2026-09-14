import struct
import sys

def parse_mp4_tracks(path):
    print(f"=== Inspecting {path} ===")
    with open(path, "rb") as f:
        data = f.read()

    pos = 0
    while pos + 8 <= len(data):
        sz, fourcc = struct.unpack(">I4s", data[pos:pos+8])
        fourcc_str = fourcc.decode("latin1", errors="ignore")
        if sz == 1:
            sz = struct.unpack(">Q", data[pos+8:pos+16])[0]
            hdr_len = 16
        elif sz == 0:
            sz = len(data) - pos
            hdr_len = 8
        else:
            hdr_len = 8
        
        if fourcc_str == "moov":
            print(f"Found moov at {pos}, size {sz}")
            # Scan for traks
            moov_data = data[pos+hdr_len:pos+sz]
            mpos = 0
            trak_id = 0
            while mpos + 8 <= len(moov_data):
                tsz, tfourcc = struct.unpack(">I4s", moov_data[mpos:mpos+8])
                tfourcc_str = tfourcc.decode("latin1", errors="ignore")
                if tfourcc_str == "trak":
                    trak_id += 1
                    trak_data = moov_data[mpos+8:mpos+tsz]
                    print(f"  Track #{trak_id}:")
                    # Search for handler type in hdlr
                    hpos = trak_data.find(b"hdlr")
                    if hpos != -1 and hpos + 16 <= len(trak_data):
                        htype = trak_data[hpos+8:hpos+12].decode("latin1", errors="ignore")
                        print(f"    Handler: {htype}")
                    # Search for stsd
                    spos = trak_data.find(b"stsd")
                    if spos != -1 and spos + 20 <= len(trak_data):
                        codec = trak_data[spos+16:spos+20].decode("latin1", errors="ignore")
                        print(f"    Codec FourCC: {codec}")
                if tsz == 0:
                    break
                mpos += tsz
            break
        if sz == 0:
            break
        pos += sz

if __name__ == "__main__":
    for p in ["TEST-VIDEO/test.mp4", "TEST-VIDEO/test1.mp4", "TEST-VIDEO/DolbyVision(720p).mp4"]:
        try:
            parse_mp4_tracks(p)
        except Exception as e:
            print(f"Error on {p}: {e}")
