import os
import sys

def main():
    icons_dir = r"D:\Signatures_OS\BOOT(OS-ICO)"
    out_h = r"D:\Signatures_OS\kernel\services\wallpaper\boot_assets.h"
    out_c = r"D:\Signatures_OS\kernel\services\wallpaper\boot_assets.c"

    files_to_embed = [
        ("g_boot_ico_lock_png", os.path.join(icons_dir, "lock.png")),
        ("g_boot_ico_ethernet_png", os.path.join(icons_dir, "ethernet-port.png")),
        ("g_boot_ico_chat_png", os.path.join(icons_dir, "chat.png")),
        ("g_boot_ico_user_png", os.path.join(icons_dir, "user.png")),
    ]

    with open(out_h, "w", encoding="utf-8") as f_h:
        f_h.write("#ifndef BOOT_ASSETS_H\n#define BOOT_ASSETS_H\n\n#include <stdint.h>\n#include <stddef.h>\n\n")
        for var_name, filepath in files_to_embed:
            if os.path.exists(filepath):
                size = os.path.getsize(filepath)
                f_h.write(f"extern const uint8_t {var_name}[{size}];\n")
                f_h.write(f"extern const uint32_t {var_name}_size;\n\n")
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

    print("[BOOT ASSET GENERATOR] Successfully generated lightweight boot_assets.h and boot_assets.c!")

if __name__ == "__main__":
    main()
