import struct
import sys
import os

def parse_mp4(filepath):
    print("=" * 70)
    print(f"FORENSIC AUDIT: {filepath}")
    print("=" * 70)
    if not os.path.exists(filepath):
        print(f"FILE NOT FOUND: {filepath}")
        return None

    size = os.path.getsize(filepath)
    print(f"File Size: {size:,} bytes ({size / (1024*1024):.2f} MB)")

    with open(filepath, "rb") as f:
        data = f.read(min(size, 50 * 1024 * 1024)) # read up to 50MB for headers/moov

    # Look for moov box
    moov_pos = data.find(b"moov")
    if moov_pos == -1:
        print("ERROR: moov box not found!")
        return None
    print(f"moov box found at offset {moov_pos - 4}")

    # Look for avcC box
    avcc_pos = data.find(b"avcC")
    if avcc_pos == -1:
        print("ERROR: avcC box not found!")
        return None

    box_size = struct.unpack(">I", data[avcc_pos - 4:avcc_pos])[0]
    print(f"avcC box found at offset {avcc_pos - 4}, box size = {box_size}")
    avcc_data = data[avcc_pos + 4 : avcc_pos - 4 + box_size]

    version = avcc_data[0]
    profile_idc = avcc_data[1]
    profile_compat = avcc_data[2]
    level_idc = avcc_data[3]
    nal_len_size = (avcc_data[4] & 0x03) + 1
    num_sps = avcc_data[5] & 0x1F

    profiles = {
        66: "Baseline Profile",
        77: "Main Profile",
        88: "Extended Profile",
        100: "High Profile",
        110: "High 10 Profile",
        122: "High 4:2:2 Profile",
        244: "High 4:4:4 Predictive Profile"
    }
    profile_str = profiles.get(profile_idc, f"Profile {profile_idc}")

    print(f"H.264 Profile: {profile_str} (profile_idc={profile_idc})")
    print(f"Profile Compatibility: 0x{profile_compat:02X}")
    print(f"Level IDC: {level_idc} (Level {level_idc / 10.0})")
    print(f"NAL Length Size: {nal_len_size} bytes")
    print(f"Num SPS: {num_sps}")

    offset = 6
    sps_list = []
    for i in range(num_sps):
        sps_len = struct.unpack(">H", avcc_data[offset:offset+2])[0]
        offset += 2
        sps = avcc_data[offset:offset+sps_len]
        offset += sps_len
        sps_list.append(sps)
        print(f"  SPS[{i}]: len={sps_len}, hex={sps.hex()[:32]}...")

    num_pps = avcc_data[offset]
    offset += 1
    print(f"Num PPS: {num_pps}")
    pps_list = []
    for i in range(num_pps):
        pps_len = struct.unpack(">H", avcc_data[offset:offset+2])[0]
        offset += 2
        pps = avcc_data[offset:offset+pps_len]
        offset += pps_len
        pps_list.append(pps)
        print(f"  PPS[{i}]: len={pps_len}, hex={pps.hex()[:32]}...")

    # Bitstream parser for PPS
    # Exp-Golomb reader
    class BitReader:
        def __init__(self, b):
            self.bytes = b
            self.bitpos = 0
            self.total_bits = len(b) * 8
        def get_bit(self):
            if self.bitpos >= self.total_bits:
                return 0
            b = self.bytes[self.bitpos // 8]
            shift = 7 - (self.bitpos % 8)
            self.bitpos += 1
            return (b >> shift) & 1
        def get_ue(self):
            zeros = 0
            while self.bitpos < self.total_bits and self.get_bit() == 0:
                zeros += 1
            if self.bitpos >= self.total_bits:
                return 0
            val = 0
            for _ in range(zeros):
                val = (val << 1) | self.get_bit()
            return (1 << zeros) - 1 + val

    cabac_enabled = False
    for i, pps in enumerate(pps_list):
        # PPS starts with NAL header (1 byte)
        nal_unit_type = pps[0] & 0x1F
        reader = BitReader(pps[1:])
        pic_parameter_set_id = reader.get_ue()
        seq_parameter_set_id = reader.get_ue()
        entropy_coding_mode_flag = reader.get_bit()
        bottom_field_pic_order_in_frame_present_flag = reader.get_bit()
        num_slice_groups_minus1 = reader.get_ue()

        print(f"  PPS[{i}] Detailed Audit:")
        print(f"    pic_parameter_set_id: {pic_parameter_set_id}")
        print(f"    seq_parameter_set_id: {seq_parameter_set_id}")
        print(f"    entropy_coding_mode_flag: {entropy_coding_mode_flag} "
              f"({'1 = CABAC (Context-Adaptive Binary Arithmetic Coding)' if entropy_coding_mode_flag else '0 = CAVLC'})")

        if entropy_coding_mode_flag == 1:
            cabac_enabled = True

    print("-" * 70)
    print("ATOMS OS H264BSD DECODER COMPATIBILITY AUDIT:")
    print("h264bsd specification constraint:")
    print("  In third_party/media/h264/src/h264bsd_pic_param_set.c:")
    print("    tmp = h264bsdGetBits(pStrmData, 1); // entropy_coding_mode_flag")
    print("    if (tmp) { EPRINT(\"entropy_coding_mode_flag\"); return(HANTRO_NOK); }")
    print("-" * 70)
    if cabac_enabled:
        print("VERDICT ON THIS FILE:")
        print("  [FAIL] FATAL INCOMPATIBILITY: entropy_coding_mode_flag == 1 (CABAC)")
        print("  -> h264bsdDecodePicParamSet() RETURNS HANTRO_NOK (H264BSD_PARAM_SET_ERROR)")
        print("  -> PPS is REJECTED by h264bsd!")
        print("  -> All subsequent video slice packets FAIL with H264BSD_PARAM_SET_ERROR")
        print("  -> h264bsdNextOutputPicture() returns NULL for 100% of frames")
        print("  -> No texture uploaded -> CANVAS REMAINS SOLID BLACK")
    else:
        print("VERDICT ON THIS FILE:")
        print("  [PASS] FULLY COMPATIBLE: entropy_coding_mode_flag == 0 (CAVLC Baseline)")
        print("  -> h264bsdDecodePicParamSet() SUCCEEDS (H264BSD_RDY)")
        print("  -> Video frames decode cleanly into YUV420P planar buffers")

    return {
        "profile": profile_str,
        "cabac": cabac_enabled
    }

if __name__ == "__main__":
    files = [
        "E:/Dolby_Vision_AtmosHDR.mp4",
        "d:/Signatures_OS/build/TEST.MP4",
        "d:/Signatures_OS/build/DOLBY_BASELINE.MP4"
    ]
    for f in files:
        if os.path.exists(f):
            parse_mp4(f)
