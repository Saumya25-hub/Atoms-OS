import os
import sys
from PIL import Image

def qoi_encode(img):
    img = img.convert('RGB')
    if img.size != (960, 540):
        img = img.resize((960, 540), Image.Resampling.LANCZOS)
    raw = img.tobytes()
    w, h = 960, 540
    
    bytes_out = bytearray(b'qoif')
    bytes_out.extend(w.to_bytes(4, 'big'))
    bytes_out.extend(h.to_bytes(4, 'big'))
    bytes_out.append(3) # RGB
    bytes_out.append(0) # sRGB
    
    index = [(0, 0, 0, 255)] * 64
    px_prev = (0, 0, 0, 255)
    run = 0
    
    for i in range(0, len(raw), 3):
        px = (raw[i], raw[i+1], raw[i+2], 255)
        
        if px == px_prev:
            run += 1
            if run == 62:
                bytes_out.append(0xC0 | (run - 1))
                run = 0
        else:
            if run > 0:
                bytes_out.append(0xC0 | (run - 1))
                run = 0
            
            idx = (px[0] * 3 + px[1] * 5 + px[2] * 7 + px[3] * 11) % 64
            if index[idx] == px:
                bytes_out.append(0x00 | idx)
            else:
                index[idx] = px
                
                vr = (px[0] - px_prev[0] + 256) % 256
                vg = (px[1] - px_prev[1] + 256) % 256
                vb = (px[2] - px_prev[2] + 256) % 256
                
                vr_s = vr - 256 if vr > 127 else vr
                vg_s = vg - 256 if vg > 127 else vg
                vb_s = vb - 256 if vb > 127 else vb
                
                vg_r = vr_s - vg_s
                vg_b = vb_s - vg_s
                
                if -2 <= vr_s <= 1 and -2 <= vg_s <= 1 and -2 <= vb_s <= 1:
                    bytes_out.append(0x40 | ((vr_s + 2) << 4) | ((vg_s + 2) << 2) | (vb_s + 2))
                elif -32 <= vg_s <= 31 and -8 <= vg_r <= 7 and -8 <= vg_b <= 7:
                    bytes_out.append(0x80 | (vg_s + 32))
                    bytes_out.append(((vg_r + 8) << 4) | (vg_b + 8))
                else:
                    bytes_out.append(0xFE)
                    bytes_out.append(px[0])
                    bytes_out.append(px[1])
                    bytes_out.append(px[2])
                    
            px_prev = px
            
    if run > 0:
        bytes_out.append(0xC0 | (run - 1))
        
    bytes_out.extend(b'\x00\x00\x00\x00\x00\x00\x00\x01')
    return bytes(bytes_out)

def main():
    icons_dir = r"D:\Signatures_OS\BOOT(OS-ICO)"
    wallpapers_dir = r"D:\Signatures_OS\BOOT-WALLAPPERS"
    out_h = r"D:\Signatures_OS\kernel\services\wallpaper\boot_assets.h"
    out_c = r"D:\Signatures_OS\kernel\services\wallpaper\boot_assets.c"

    files_to_embed = [
        ("g_boot_ico_lock_png", os.path.join(icons_dir, "lock.png")),
        ("g_boot_ico_ethernet_png", os.path.join(icons_dir, "ethernet-port.png")),
        ("g_boot_ico_chat_png", os.path.join(icons_dir, "chat.png")),
        ("g_boot_ico_user_png", os.path.join(icons_dir, "user.png")),
    ]

    wp_path = os.path.join(wallpapers_dir, "1.png")
    if not os.path.exists(wp_path):
        wp_path = os.path.join(wallpapers_dir, "W1.png")

    print(f"[BOOT ASSET GENERATOR] Loading wallpaper from {wp_path}...")
    wp_img = Image.open(wp_path)
    wp_qoi = qoi_encode(wp_img)
    print(f"[BOOT ASSET GENERATOR] Wallpaper QOI encoded size: {len(wp_qoi)} bytes ({len(wp_qoi)/1024/1024:.2f} MB)")

    with open(out_h, "w", encoding="utf-8") as f_h:
        f_h.write("#ifndef BOOT_ASSETS_H\n#define BOOT_ASSETS_H\n\n#include <stdint.h>\n#include <stddef.h>\n\n")
        for var_name, filepath in files_to_embed:
            if os.path.exists(filepath):
                size = os.path.getsize(filepath)
                f_h.write(f"extern const uint8_t {var_name}[{size}];\n")
                f_h.write(f"extern const uint32_t {var_name}_size;\n\n")
        f_h.write(f"extern const uint8_t g_boot_wallpaper_qoi[{len(wp_qoi)}];\n")
        f_h.write(f"extern const uint32_t g_boot_wallpaper_qoi_size;\n\n")
        f_h.write("#endif // BOOT_ASSETS_H\n")

    with open(out_c, "w", encoding="utf-8") as f_c:
        f_c.write('#include "boot_assets.h"\n\n')
        for var_name, filepath in files_to_embed:
            if not os.path.exists(filepath):
                print(f"Warning: File not found: {filepath}")
                continue
            with open(filepath, "rb") as f_in:
                data = f_in.read()
            size = len(data)
            f_c.write(f"const uint8_t {var_name}[{size}] = {{\n")
            for i, byte in enumerate(data):
                f_c.write(f"0x{byte:02X}, ")
                if (i + 1) % 16 == 0:
                    f_c.write("\n")
            f_c.write("\n};\n")
            f_c.write(f"const uint32_t {var_name}_size = {size};\n\n")

        f_c.write(f"const uint8_t g_boot_wallpaper_qoi[{len(wp_qoi)}] = {{\n")
        for i, byte in enumerate(wp_qoi):
            f_c.write(f"0x{byte:02X}, ")
            if (i + 1) % 16 == 0:
                f_c.write("\n")
        f_c.write("\n};\n")
        f_c.write(f"const uint32_t g_boot_wallpaper_qoi_size = {len(wp_qoi)};\n\n")

    print("[BOOT ASSET GENERATOR] Successfully generated boot_assets.h and boot_assets.c with clean QOI wallpaper!")

if __name__ == "__main__":
    main()
