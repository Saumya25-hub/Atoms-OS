import os
from PIL import Image

def main():
    yuv_path = r"D:\BOSPECTRA_TEST\frame_0001.yuv"
    png_path = r"D:\BOSPECTRA_TEST\frame_0001.png"
    bmp_path = r"D:\BOSPECTRA_TEST\frame_0001.bmp"

    w, h = 640, 360
    y_size = w * h
    uv_size = (w // 2) * (h // 2)

    if not os.path.exists(yuv_path):
        print("ERROR: frame_0001.yuv not found")
        return

    with open(yuv_path, "rb") as f:
        y_bytes = f.read(y_size)
        cb_bytes = f.read(uv_size)
        cr_bytes = f.read(uv_size)

    # Reconstruct Y, Cb, Cr planes as Pillow Images
    y_img = Image.frombytes("L", (w, h), y_bytes)
    cb_img = Image.frombytes("L", (w // 2, h // 2), cb_bytes).resize((w, h), Image.Resampling.BILINEAR)
    cr_img = Image.frombytes("L", (w // 2, h // 2), cr_bytes).resize((w, h), Image.Resampling.BILINEAR)

    # Merge to YCbCr and convert to RGB
    ycbcr_img = Image.merge("YCbCr", (y_img, cb_img, cr_img))
    rgb_img = ycbcr_img.convert("RGB")

    # Save as PNG and BMP
    rgb_img.save(png_path)
    rgb_img.save(bmp_path)

    print(f"[HOST] Converted raw YUV to PNG: {png_path}")
    print(f"[HOST] Converted raw YUV to BMP: {bmp_path}")
    print("SUCCESS: You can now double click frame_0001.png directly in Windows File Explorer!")

if __name__ == "__main__":
    main()
