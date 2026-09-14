import os
import io
from PIL import Image

def main():
    avi_path = r"d:\Signatures_OS\TEST-VIDEO\DOLBY.avi"
    out_dir = r"D:\BOSPECTRA_TEST"
    os.makedirs(out_dir, exist_ok=True)
    out_yuv = os.path.join(out_dir, "frame_0001.yuv")

    print(f"Reading AVI file: {avi_path}")
    with open(avi_path, "rb") as f:
        data = f.read()

    # Find first JPEG chunk SOI (0xFFD8) and EOI (0xFFD9)
    soi_idx = data.find(b"\xFF\xD8")
    if soi_idx == -1:
        print("ERROR: No JPEG SOI marker found in AVI file")
        return

    eoi_idx = data.find(b"\xFF\xD9", soi_idx)
    if eoi_idx == -1:
        print("ERROR: No JPEG EOI marker found")
        return

    jpeg_bytes = data[soi_idx : eoi_idx + 2]
    print(f"Extracted JPEG frame 1: {len(jpeg_bytes)} bytes")

    # Load with PIL
    img = Image.open(io.BytesIO(jpeg_bytes))
    w, h = img.size
    print(f"Decoded JPEG Dimensions: {w}x{h}, Mode: {img.mode}")

    # Convert to YCbCr (Y, Cb, Cr planes)
    img_ycbcr = img.convert("YCbCr")
    y_img, cb_img, cr_img = img_ycbcr.split()

    # Subsample Cb and Cr to 4:2:0
    cb_sub = cb_img.resize((w // 2, h // 2), Image.Resampling.BILINEAR)
    cr_sub = cr_img.resize((w // 2, h // 2), Image.Resampling.BILINEAR)

    y_bytes = y_img.tobytes()
    cb_bytes = cb_sub.tobytes()
    cr_bytes = cr_sub.tobytes()

    y_size = len(y_bytes)
    cb_size = len(cb_bytes)
    cr_size = len(cr_bytes)

    # Write raw contiguous YUV420P: Y followed by Cb followed by Cr
    with open(out_yuv, "wb") as f:
        f.write(y_bytes)
        f.write(cb_bytes)
        f.write(cr_bytes)

    print("\n========== YUV DUMP ==========")
    print(f"Width : {w}")
    print(f"Height: {h}")
    print(f"Pixel Format: YUV420P")
    print(f"Y Pitch : {w}")
    print(f"Cb Pitch: {w // 2}")
    print(f"Cr Pitch: {w // 2}")
    print(f"Y Size : {y_size}")
    print(f"Cb Size: {cb_size}")
    print(f"Cr Size: {cr_size}")
    print(f"Output: {out_yuv}")
    print("==============================\n")
    print(f"SUCCESS: {out_yuv} is created and ready to open in VLC!")

if __name__ == "__main__":
    main()
