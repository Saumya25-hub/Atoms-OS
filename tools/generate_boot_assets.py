import os
import sys
from PIL import Image

def qoi_encode(img):
    img = img.convert('RGB')
    if img.size != (1920, 1080):
        img = img.resize((1920, 1080), Image.Resampling.LANCZOS)
    raw = img.tobytes()
    w, h = 1920, 1080
    
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
    base_dir = r"D:\Signatures_OS"
    icons_dir = os.path.join(base_dir, "BOOT(OS-ICO)")
    wallpapers_dir = os.path.join(base_dir, "BOOT-WALLAPPERS")
    wp_bin_dir = os.path.join(base_dir, "build", "wallpapers")
    os.makedirs(wp_bin_dir, exist_ok=True)

    out_h = os.path.join(base_dir, "kernel", "services", "wallpaper", "boot_assets.h")
    out_c = os.path.join(base_dir, "kernel", "services", "wallpaper", "boot_assets.c")
    out_asm = os.path.join(base_dir, "kernel", "services", "wallpaper", "boot_assets_data.asm")

    files_to_embed = [
        ("g_boot_ico_lock_png", "BOOT(OS-ICO)/lock.png", os.path.join(icons_dir, "lock.png")),
        ("g_boot_ico_ethernet_png", "BOOT(OS-ICO)/ethernet-port.png", os.path.join(icons_dir, "ethernet-port.png")),
        ("g_boot_ico_chat_png", "BOOT(OS-ICO)/chat.png", os.path.join(icons_dir, "chat.png")),
        ("g_boot_ico_user_png", "BOOT(OS-ICO)/user.png", os.path.join(icons_dir, "user.png")),
    ]

    wp_qoi_info = []
    # Embed 3 pristine 1080p Full HD wallpapers (safe <=9.6MB memory footprint for all UEFI firmware)
    target_indices = [1, 2, 7]
    for idx, wp_num in enumerate(target_indices):
        wp_path = os.path.join(wallpapers_dir, f"{wp_num}.png")
        if not os.path.exists(wp_path):
            wp_path = os.path.join(wallpapers_dir, f"W{wp_num}.png")
        if not os.path.exists(wp_path):
            wp_path = os.path.join(wallpapers_dir, "1.png")

        print(f"[BOOT ASSET GENERATOR] Loading wallpaper {idx+1} (file {wp_num}.png) from {wp_path}...")
        wp_img = Image.open(wp_path)
        wp_qoi = qoi_encode(wp_img)
        bin_filename = f"wp_{idx}.qoi"
        bin_path = os.path.join(wp_bin_dir, bin_filename)
        with open(bin_path, "wb") as f_bin:
            f_bin.write(wp_qoi)
        rel_bin_path = f"build/wallpapers/{bin_filename}"
        print(f"[BOOT ASSET GENERATOR] Wallpaper {idx+1} 1080p QOI size: {len(wp_qoi)} bytes ({len(wp_qoi)/1024/1024:.2f} MB)")
        wp_qoi_info.append((f"g_boot_wallpaper_qoi_{idx}", rel_bin_path, len(wp_qoi)))

    # Write boot_assets.h
    with open(out_h, "w", encoding="utf-8") as f_h:
        f_h.write("#ifndef BOOT_ASSETS_H\n#define BOOT_ASSETS_H\n\n#include <stdint.h>\n#include <stddef.h>\n\n")
        for var_name, rel_path, filepath in files_to_embed:
            if os.path.exists(filepath):
                size = os.path.getsize(filepath)
                f_h.write(f"extern const uint8_t {var_name}[{size}];\n")
                f_h.write(f"extern const uint32_t {var_name}_size;\n\n")

        f_h.write(f"#define BOOT_WALLPAPERS_COUNT {len(wp_qoi_info)}\n\n")
        for var_name, rel_path, size in wp_qoi_info:
            f_h.write(f"extern const uint8_t {var_name}[{size}];\n")
            f_h.write(f"extern const uint32_t {var_name}_size;\n\n")

        f_h.write("extern const uint8_t* const g_boot_wallpapers_qoi[BOOT_WALLPAPERS_COUNT];\n")
        f_h.write("extern const uint32_t g_boot_wallpapers_qoi_sizes[BOOT_WALLPAPERS_COUNT];\n\n")
        
        f_h.write(f"extern const uint8_t g_boot_wallpaper_qoi[{wp_qoi_info[0][2]}];\n")
        f_h.write(f"extern const uint32_t g_boot_wallpaper_qoi_size;\n\n")
        f_h.write("#endif // BOOT_ASSETS_H\n")

    # Write boot_assets_data.asm (assembled with NASM in milliseconds)
    with open(out_asm, "w", encoding="utf-8") as f_asm:
        f_asm.write("; Auto-generated by generate_boot_assets.py\n")
        f_asm.write("section .rodata\n\n")
        
        for var_name, rel_path, filepath in files_to_embed:
            f_asm.write(f"global {var_name}\n")
        for var_name, rel_path, size in wp_qoi_info:
            f_asm.write(f"global {var_name}\n")
        f_asm.write("global g_boot_wallpaper_qoi\n\n")

        for var_name, rel_path, filepath in files_to_embed:
            f_asm.write(f"{var_name}:\n")
            f_asm.write(f"    incbin '{rel_path}'\n\n")

        for var_name, rel_path, size in wp_qoi_info:
            f_asm.write(f"{var_name}:\n")
            f_asm.write(f"    incbin '{rel_path}'\n\n")

        f_asm.write("g_boot_wallpaper_qoi:\n")
        f_asm.write(f"    incbin '{wp_qoi_info[0][1]}'\n\n")

    # Write boot_assets.c (clean, compact metadata)
    with open(out_c, "w", encoding="utf-8") as f_c:
        f_c.write('#include "boot_assets.h"\n\n')
        for var_name, rel_path, filepath in files_to_embed:
            if os.path.exists(filepath):
                size = os.path.getsize(filepath)
                f_c.write(f"const uint32_t {var_name}_size = {size};\n")

        f_c.write("\n")
        for var_name, rel_path, size in wp_qoi_info:
            f_c.write(f"const uint32_t {var_name}_size = {size};\n")

        f_c.write("\nconst uint8_t* const g_boot_wallpapers_qoi[BOOT_WALLPAPERS_COUNT] = {\n")
        for var_name, rel_path, size in wp_qoi_info:
            f_c.write(f"    {var_name},\n")
        f_c.write("};\n\n")

        f_c.write("const uint32_t g_boot_wallpapers_qoi_sizes[BOOT_WALLPAPERS_COUNT] = {\n")
        for var_name, rel_path, size in wp_qoi_info:
            f_c.write(f"    {size},\n")
        f_c.write("};\n\n")

        f_c.write(f"const uint32_t g_boot_wallpaper_qoi_size = {wp_qoi_info[0][2]};\n")

    print("[BOOT ASSET GENERATOR] Successfully generated boot_assets.h, boot_assets_data.asm, and boot_assets.c with 1080p QOI wallpapers!")

if __name__ == "__main__":
    main()
