import os
import math
from PIL import Image, ImageDraw

out_dir = r"d:\Signatures_OS\assets\icons"
os.makedirs(out_dir, exist_ok=True)

SIZE = 128
TARGET_SIZE = 32

def create_canvas():
    return Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))

def save_icon(img, filename):
    resized = img.resize((TARGET_SIZE, TARGET_SIZE), Image.Resampling.LANCZOS)
    path = os.path.join(out_dir, filename)
    resized.save(path, "PNG")
    print(f"Saved {path}")

# 1. Wi-Fi Connected (sys_wifi.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
cx, cy = 64, 88
draw.arc([cx - 48, cy - 48, cx + 48, cy + 48], 215, 325, fill=(56, 189, 248, 255), width=8)
draw.arc([cx - 32, cy - 32, cx + 32, cy + 32], 215, 325, fill=(56, 189, 248, 255), width=8)
draw.arc([cx - 16, cy - 16, cx + 16, cy + 16], 215, 325, fill=(56, 189, 248, 255), width=8)
draw.ellipse([cx - 6, cy - 6, cx + 6, cy + 6], fill=(56, 189, 248, 255))
save_icon(img, "sys_wifi.png")

# 2. Wi-Fi Disconnected (sys_wifi_off.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.arc([cx - 48, cy - 48, cx + 48, cy + 48], 215, 325, fill=(100, 116, 139, 180), width=8)
draw.arc([cx - 32, cy - 32, cx + 32, cy + 32], 215, 325, fill=(100, 116, 139, 180), width=8)
draw.arc([cx - 16, cy - 16, cx + 16, cy + 16], 215, 325, fill=(100, 116, 139, 180), width=8)
draw.ellipse([cx - 6, cy - 6, cx + 6, cy + 6], fill=(100, 116, 139, 180))
draw.line([24, 24, 104, 104], fill=(239, 68, 68, 240), width=8)
save_icon(img, "sys_wifi_off.png")

# 3. Volume Normal (sys_vol_norm.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.polygon([(24, 48), (44, 48), (68, 28), (68, 100), (44, 80), (24, 80)], fill=(241, 245, 249, 255))
draw.arc([40, 32, 96, 96], -45, 45, fill=(59, 130, 246, 255), width=8)
draw.arc([24, 16, 112, 112], -45, 45, fill=(59, 130, 246, 255), width=8)
save_icon(img, "sys_vol_norm.png")

# 4. Volume Muted (sys_vol_mute.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.polygon([(24, 48), (44, 48), (68, 28), (68, 100), (44, 80), (24, 80)], fill=(241, 245, 249, 255))
draw.line([80, 48, 108, 76], fill=(239, 68, 68, 255), width=8)
draw.line([108, 48, 80, 76], fill=(239, 68, 68, 255), width=8)
save_icon(img, "sys_vol_mute.png")

# 5. Battery Normal (sys_bat_norm.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.rounded_rectangle([16, 40, 104, 88], radius=12, outline=(241, 245, 249, 255), width=8)
draw.rounded_rectangle([104, 52, 114, 76], radius=4, fill=(241, 245, 249, 255))
draw.rounded_rectangle([26, 48, 92, 80], radius=6, fill=(34, 197, 94, 255))
save_icon(img, "sys_bat_norm.png")

# 6. Battery Low (sys_bat_low.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.rounded_rectangle([16, 40, 104, 88], radius=12, outline=(241, 245, 249, 255), width=8)
draw.rounded_rectangle([104, 52, 114, 76], radius=4, fill=(241, 245, 249, 255))
draw.rounded_rectangle([26, 48, 42, 80], radius=6, fill=(239, 68, 68, 255))
save_icon(img, "sys_bat_low.png")

# 7. Notification Bell (sys_bell_norm.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.ellipse([58, 20, 70, 32], outline=(241, 245, 249, 255), width=6)
draw.polygon([(64, 30), (88, 56), (92, 84), (36, 84), (40, 56)], fill=(241, 245, 249, 255))
draw.rounded_rectangle([30, 80, 98, 90], radius=4, fill=(241, 245, 249, 255))
draw.ellipse([56, 90, 72, 104], fill=(241, 245, 249, 255))
save_icon(img, "sys_bell_norm.png")

# 8. Notification Bell Dot (sys_bell_dot.png)
img = create_canvas()
draw = ImageDraw.Draw(img)
draw.ellipse([58, 20, 70, 32], outline=(241, 245, 249, 255), width=6)
draw.polygon([(64, 30), (88, 56), (92, 84), (36, 84), (40, 56)], fill=(241, 245, 249, 255))
draw.rounded_rectangle([30, 80, 98, 90], radius=4, fill=(241, 245, 249, 255))
draw.ellipse([56, 90, 72, 104], fill=(241, 245, 249, 255))
draw.ellipse([80, 20, 108, 48], fill=(239, 68, 68, 255), outline=(15, 23, 42, 255), width=4)
save_icon(img, "sys_bell_dot.png")

print("All system status icons generated successfully!")
