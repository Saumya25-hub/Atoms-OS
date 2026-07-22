import os
from PIL import Image

icons = [
    ("sys_wifi", "g_sys_wifi_rgba"),
    ("sys_wifi_off", "g_sys_wifi_off_rgba"),
    ("sys_vol_norm", "g_sys_vol_norm_rgba"),
    ("sys_vol_mute", "g_sys_vol_mute_rgba"),
    ("sys_bat_norm", "g_sys_bat_norm_rgba"),
    ("sys_bat_low", "g_sys_bat_low_rgba"),
    ("sys_bell_norm", "g_sys_bell_norm_rgba"),
    ("sys_bell_dot", "g_sys_bell_dot_rgba")
]

out_h = r"d:\Signatures_OS\kernel\ui\boasset\sys_icons_data.h"

with open(out_h, "w") as f:
    f.write("#ifndef KERNEL_BOASSET_SYS_ICONS_DATA_H\n")
    f.write("#define KERNEL_BOASSET_SYS_ICONS_DATA_H\n\n")
    f.write("#include <stdint.h>\n\n")

    for fname, varname in icons:
        img_path = os.path.join(r"d:\Signatures_OS\assets\icons", f"{fname}.png")
        img = Image.open(img_path).convert("RGBA")
        w, h = img.size
        pixels = list(img.getdata())

        f.write(f"// {fname}.png {w}x{h} RGBA\n")
        f.write(f"static const uint8_t {varname}[{w * h * 4}] = {{\n")

        line = "    "
        for i, (r, g, b, a) in enumerate(pixels):
            line += f"{b},{g},{r},{a},"
            if (i + 1) % 8 == 0:
                f.write(line + "\n")
                line = "    "
        if line.strip():
            f.write(line + "\n")
        f.write("};\n\n")

    f.write("#endif // KERNEL_BOASSET_SYS_ICONS_DATA_H\n")

print(f"Generated {out_h} successfully!")
