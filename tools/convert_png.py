import os
import sys

try:
    from PIL import Image
except ImportError:
    print("Pillow not found, installing via pip...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
    from PIL import Image

png_path = r"d:\Signatures_OS\photo\boot.png"
raw_path = r"d:\Signatures_OS\build\boot.raw"

if not os.path.exists(png_path):
    print(f"Error: {png_path} does not exist!")
    sys.exit(1)

print(f"Converting {png_path} to raw BGRA...")
img = Image.open(png_path).resize((1920, 1080)).convert("RGBA")

# VBE expects BGRA format: Blue, Green, Red, Alpha (0xAARRGGBB in little endian)
r, g, b, a = img.split()
bgra = Image.merge("RGBA", (b, g, r, a))

with open(raw_path, "wb") as f:
    f.write(bgra.tobytes())

print(f"Success! Saved raw BGRA image to {raw_path}")
