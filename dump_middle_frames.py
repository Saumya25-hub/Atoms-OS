import os
import io
from PIL import Image

def main():
    avi_path = r"d:\Signatures_OS\TEST-VIDEO\DOLBY.avi"
    out_dir = r"D:\BOSPECTRA_TEST"
    os.makedirs(out_dir, exist_ok=True)

    print(f"Scanning AVI file for all JPEG frames: {avi_path}")
    with open(avi_path, "rb") as f:
        data = f.read()

    # Find all JPEG frames (SOI 0xFFD8 -> EOI 0xFFD9)
    soi_positions = []
    pos = 0
    while True:
        idx = data.find(b"\xFF\xD8", pos)
        if idx == -1:
            break
        soi_positions.append(idx)
        pos = idx + 2

    print(f"Total JPEG frames found in AVI: {len(soi_positions)}")
    if not soi_positions:
        print("ERROR: No JPEG frames found")
        return

    # Select multiple frames: e.g. frame 5, frame 30, frame 60, frame 100, middle frame
    frame_indices = [5, 15, 30, 60, len(soi_positions) // 2]
    frame_indices = sorted(list(set([idx for idx in frame_indices if idx < len(soi_positions)])))

    print(f"Extracting frames at indices: {frame_indices}")

    for idx_num in frame_indices:
        soi = soi_positions[idx_num]
        eoi = data.find(b"\xFF\xD9", soi)
        if eoi == -1:
            continue

        jpeg_bytes = data[soi : eoi + 2]
        img = Image.open(io.BytesIO(jpeg_bytes))
        w, h = img.size

        # Convert to PNG
        img_rgb = img.convert("RGB")
        out_png = os.path.join(out_dir, f"frame_{idx_num:04d}.png")
        img_rgb.save(out_png)

        # Convert to YUV420P
        img_ycbcr = img.convert("YCbCr")
        y_img, cb_img, cr_img = img_ycbcr.split()
        cb_sub = cb_img.resize((w // 2, h // 2), Image.Resampling.BILINEAR)
        cr_sub = cr_img.resize((w // 2, h // 2), Image.Resampling.BILINEAR)

        out_yuv = os.path.join(out_dir, f"frame_{idx_num:04d}.yuv")
        with open(out_yuv, "wb") as f:
            f.write(y_img.tobytes())
            f.write(cb_sub.tobytes())
            f.write(cr_sub.tobytes())

        # Also overwrite frame_0001.png with frame_0030.png if idx_num == 30
        if idx_num == 30 or idx_num == frame_indices[len(frame_indices)//2]:
            img_rgb.save(os.path.join(out_dir, "frame_0001.png"))
            with open(os.path.join(out_dir, "frame_0001.yuv"), "wb") as f:
                f.write(y_img.tobytes())
                f.write(cb_sub.tobytes())
                f.write(cr_sub.tobytes())

        print(f"Extracted Frame #{idx_num}: {w}x{h} -> {out_png}")

    print("\nSUCCESS: Middle video frames dumped to D:\\BOSPECTRA_TEST\\")

if __name__ == "__main__":
    main()
