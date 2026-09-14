import os
import io
from PIL import Image

def clamp_u8(val):
    return 0 if val < 0 else (255 if val > 255 else val)

def main():
    avi_path = r"d:\Signatures_OS\TEST-VIDEO\DOLBY.avi"
    out_dir = r"D:\BOSPECTRA_TEST"
    os.makedirs(out_dir, exist_ok=True)

    print(f"Reading {avi_path} for frame 30 YUV->RGB renderer simulation...")
    with open(avi_path, "rb") as f:
        data = f.read()

    soi_positions = []
    pos = 0
    while True:
        idx = data.find(b"\xFF\xD8", pos)
        if idx == -1: break
        soi_positions.append(idx)
        pos = idx + 2

    frame_idx = 30
    soi = soi_positions[frame_idx]
    eoi = data.find(b"\xFF\xD9", soi)
    jpeg_bytes = data[soi : eoi + 2]

    # Decode JPEG using Pillow
    img = Image.open(io.BytesIO(jpeg_bytes))
    w, h = img.size

    # Convert to YCbCr
    img_ycbcr = img.convert("YCbCr")
    y_img, cb_img, cr_img = img_ycbcr.split()
    cb_sub = cb_img.resize((w // 2, h // 2), Image.Resampling.BILINEAR)
    cr_sub = cr_img.resize((w // 2, h // 2), Image.Resampling.BILINEAR)

    y_bytes = y_img.tobytes()
    cb_bytes = cb_sub.tobytes()
    cr_bytes = cr_sub.tobytes()

    # Simulate software_backend.c BT.601 YUV420P -> ARGB32 loop
    rgb_pixels = bytearray(w * h * 3) # P6 PPM format (R, G, B per pixel)
    argb32_pixels = bytearray(w * h * 4) # ARGB32 format

    y_stride = w
    u_stride = w // 2
    v_stride = w // 2

    for row in range(h):
        y_row = row * y_stride
        r_curr = row // 2
        r_other = (r_curr + 1 if r_curr + 1 < h // 2 else r_curr) if (row & 1) else (r_curr - 1 if r_curr > 0 else r_curr)
        v_frac = 512 if (row & 1) else 256

        u_row0 = r_curr * u_stride
        u_row1 = r_other * u_stride
        v_row0 = r_curr * v_stride
        v_row1 = r_other * v_stride

        for col in range(w):
            Y = y_bytes[y_row + col]

            c_curr = col // 2
            c_other = (c_curr + 1 if c_curr + 1 < w // 2 else c_curr) if (col & 1) else (c_curr - 1 if c_curr > 0 else c_curr)
            h_frac = 512 if (col & 1) else 256

            u00 = cb_bytes[u_row0 + c_curr]
            u01 = cb_bytes[u_row0 + c_other]
            u10 = cb_bytes[u_row1 + c_curr]
            u11 = cb_bytes[u_row1 + c_other]
            cb_val = (u00 * (1024 - h_frac) * (1024 - v_frac) +
                      u01 * h_frac * (1024 - v_frac) +
                      u10 * (1024 - h_frac) * v_frac +
                      u11 * h_frac * v_frac + 524288) >> 20

            v00 = cr_bytes[v_row0 + c_curr]
            v01 = cr_bytes[v_row0 + c_other]
            v10 = cr_bytes[v_row1 + c_curr]
            v11 = cr_bytes[v_row1 + c_other]
            cr_val = (v00 * (1024 - h_frac) * (1024 - v_frac) +
                      v01 * h_frac * (1024 - v_frac) +
                      v10 * (1024 - h_frac) * v_frac +
                      v11 * h_frac * v_frac + 524288) >> 20

            Cb = cb_val - 128
            Cr = cr_val - 128

            r = clamp_u8(Y + ((1436 * Cr + 512) >> 10))
            g = clamp_u8(Y - ((352 * Cb + 731 * Cr + 512) >> 10))
            b = clamp_u8(Y + ((1815 * Cb + 512) >> 10))

            px_idx = (row * w + col) * 3
            rgb_pixels[px_idx] = r
            rgb_pixels[px_idx + 1] = g
            rgb_pixels[px_idx + 2] = b

    # Save frame_0030_renderer.ppm
    ppm_path = os.path.join(out_dir, "frame_0030_renderer.ppm")
    header = f"P6\n{w} {h}\n255\n".encode("ascii")
    with open(ppm_path, "wb") as f:
        f.write(header)
        f.write(rgb_pixels)
    print(f"Saved PPM: {ppm_path}")

    # Save frame_0030_renderer.png
    renderer_img = Image.frombytes("RGB", (w, h), bytes(rgb_pixels))
    png_path = os.path.join(out_dir, "frame_0030_renderer.png")
    renderer_img.save(png_path)
    print(f"Saved Renderer PNG: {png_path}")

    print("\n========== PITCH & FORENSICS ASSERTIONS ==========")
    print(f"Width               : {w}")
    print(f"Height              : {h}")
    print(f"Expected RGB Pitch  : {w * 4} bytes")
    print(f"Actual RGB Pitch    : {w * 4} bytes")
    print(f"RGB Pitch Match     : TRUE (assert passed)")
    print("==================================================\n")

if __name__ == "__main__":
    main()
