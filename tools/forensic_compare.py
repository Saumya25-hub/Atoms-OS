#!/usr/bin/env python3
"""
BOSPECTRA FORENSIC DIFFERENTIAL COMPARISON TOOL
Name: forensic_compare.py

Compares native BOSPECTRA decoded frames (Pipeline A) vs Reference Decoded frames (Pipeline B).
Calculates CRC32, pixel deltas, and generates diff.png / rgb_diff.png.
"""

import sys
import os
import zlib
import struct

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

def calculate_crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF

def compare_buffers(buf_a: bytes, buf_b: bytes, width: int, height: int, bpp: int, out_diff_name: str):
    if len(buf_a) != len(buf_b):
        print(f"[!] Buffer size mismatch: Frame A={len(buf_a)} bytes, Frame B={len(buf_b)} bytes")
        min_len = min(len(buf_a), len(buf_b))
        buf_a = buf_a[:min_len]
        buf_b = buf_b[:min_len]

    crc_a = calculate_crc32(buf_a)
    crc_b = calculate_crc32(buf_b)

    total_pixels = width * height
    bytes_per_pixel = bpp

    max_delta = 0
    total_delta = 0
    divergent_pixels = 0

    diff_bytes = bytearray(total_pixels * (3 if bpp == 1 else 4))

    for i in range(min(total_pixels, len(buf_a) // bytes_per_pixel)):
        if bytes_per_pixel == 1:
            p_a = buf_a[i]
            p_b = buf_b[i]
            delta = abs(p_a - p_b)
            if delta > 0:
                divergent_pixels += 1
            if delta > max_delta:
                max_delta = delta
            total_delta += delta

            # High-contrast visual diff mapping (boosted 8x for visibility)
            vis = min(255, delta * 8)
            idx = i * 3
            diff_bytes[idx]     = vis  # Red
            diff_bytes[idx + 1] = vis  # Green
            diff_bytes[idx + 2] = vis  # Blue
        else:
            # 4-byte ARGB32 pixels
            idx = i * 4
            r_a, g_a, b_a = buf_a[idx+1], buf_a[idx+2], buf_a[idx+3]
            r_b, g_b, b_b = buf_b[idx+1], buf_b[idx+2], buf_b[idx+3]

            delta_r = abs(r_a - r_b)
            delta_g = abs(g_a - g_b)
            delta_b = abs(b_a - b_b)
            delta = max(delta_r, delta_g, delta_b)

            if delta > 0:
                divergent_pixels += 1
            if delta > max_delta:
                max_delta = delta
            total_delta += (delta_r + delta_g + delta_b) // 3

            vis_r = min(255, delta_r * 8)
            vis_g = min(255, delta_g * 8)
            vis_b = min(255, delta_b * 8)

            diff_bytes[idx]     = 255    # Alpha
            diff_bytes[idx + 1] = vis_r  # Red
            diff_bytes[idx + 2] = vis_g  # Green
            diff_bytes[idx + 3] = vis_b  # Blue

    avg_delta = total_delta / max(1, total_pixels)
    pct_diff = (divergent_pixels / max(1, total_pixels)) * 100.0

    print("=========================================================")
    print(f" FORENSIC COMPARISON: {out_diff_name}")
    print("=========================================================")
    print(f" Resolution             : {width} x {height} ({bpp * 8}-bit)")
    print(f" Pipeline A CRC32       : 0x{crc_a:08X}")
    print(f" Pipeline B (Ref) CRC32 : 0x{crc_b:08X}")
    print(f" Match Status           : {'🟢 IDENTICAL' if crc_a == crc_b else '🔴 DIVERGENT'}")
    print(f" Max Pixel Delta        : {max_delta} / 255")
    print(f" Average Pixel Delta    : {avg_delta:.4f}")
    print(f" Divergent Pixels       : {divergent_pixels} / {total_pixels} ({pct_diff:.2f}%)")
    print("=========================================================")

    # Write diff image
    if HAS_PIL:
        mode = "RGB" if bpp == 1 else "RGBA"
        img = Image.frombytes(mode, (width, height), bytes(diff_bytes))
        img.save(out_diff_name)
        print(f"[+] Saved visual diff image: {out_diff_name}")
    else:
        # Fallback PPM format
        ppm_name = out_diff_name.replace(".png", ".ppm")
        with open(ppm_name, "wb") as f:
            f.write(f"P6\n{width} {height}\n255\n".encode('ascii'))
            if bpp == 1:
                f.write(bytes(diff_bytes))
            else:
                rgb_only = bytearray(width * height * 3)
                for i in range(total_pixels):
                    rgb_only[i*3]     = diff_bytes[i*4 + 1]
                    rgb_only[i*3 + 1] = diff_bytes[i*4 + 2]
                    rgb_only[i*3 + 2] = diff_bytes[i*4 + 3]
                f.write(bytes(rgb_only))
        print(f"[+] Saved visual diff PPM image: {ppm_name}")

def main():
    if len(sys.argv) < 6:
        print("Usage: python forensic_compare.py <file_a> <file_b> <width> <height> <bpp:1|4> [out_diff.png]")
        sys.exit(1)

    file_a = sys.argv[1]
    file_b = sys.argv[2]
    w = int(sys.argv[3])
    h = int(sys.argv[4])
    bpp = int(sys.argv[5])
    out_name = sys.argv[6] if len(sys.argv) >= 7 else ("diff.png" if bpp == 1 else "rgb_diff.png")

    if not os.path.exists(file_a) or not os.path.exists(file_b):
        print(f"[!] Input files not found: {file_a}, {file_b}")
        sys.exit(1)

    with open(file_a, "rb") as fa, open(file_b, "rb") as fb:
        buf_a = fa.read()
        buf_b = fb.read()

    compare_buffers(buf_a, buf_b, w, h, bpp, out_name)

if __name__ == "__main__":
    main()
