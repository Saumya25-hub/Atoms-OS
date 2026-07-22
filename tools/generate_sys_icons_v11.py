import os
from PIL import Image, ImageDraw

def create_wifi_icon(state, size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size / 16.0
    cx, cy = size / 2.0, size / 2.0 + 1 * s

    white = (255, 255, 255, 255)
    weak_col = (255, 255, 255, 120)
    disc_red = (239, 68, 68, 255)

    if state == "disconnected":
        draw.arc([cx - 5 * s, cy - 5 * s, cx + 5 * s, cy + 5 * s], 215, 325, fill=weak_col, width=max(1, int(1.5 * s)))
        draw.line([cx, cy - 2 * s, cx, cy + 1 * s], fill=disc_red, width=max(1, int(1.5 * s)))
        draw.ellipse([cx - 1 * s, cy + 2 * s, cx + 1 * s, cy + 4 * s], fill=disc_red)
        return img

    w = max(1, int(1.5 * s))
    col3 = white
    col2 = white if state == "connected" else weak_col
    col1 = white

    # Outer arc
    draw.arc([cx - 6 * s, cy - 6 * s, cx + 6 * s, cy + 6 * s], 215, 325, fill=col3, width=w)
    # Middle arc
    draw.arc([cx - 3.8 * s, cy - 3.8 * s, cx + 3.8 * s, cy + 3.8 * s], 215, 325, fill=col2, width=w)
    # Dot
    r = 1.0 * s
    draw.ellipse([cx - r, cy + 2 * s - r, cx + r, cy + 2 * s + r], fill=white)
    return img

def create_volume_icon(state, size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size / 16.0
    cx, cy = size / 2.0, size / 2.0

    white = (255, 255, 255, 255)
    mute_red = (239, 68, 68, 255)

    # Speaker Body
    w = max(1, int(1.5 * s))
    pts = [(cx - 5 * s, cy - 2 * s), (cx - 2 * s, cy - 2 * s), (cx + 1 * s, cy - 5 * s), (cx + 1 * s, cy + 5 * s), (cx - 2 * s, cy + 2 * s), (cx - 5 * s, cy + 2 * s)]
    draw.polygon(pts, fill=white if state != "muted" else (220, 220, 220, 200))

    if state == "muted":
        rx = cx + 4 * s
        draw.line([rx - 2 * s, cy - 2 * s, rx + 2 * s, cy + 2 * s], fill=mute_red, width=w)
        draw.line([rx - 2 * s, cy + 2 * s, rx + 2 * s, cy - 2 * s], fill=mute_red, width=w)
    elif state == "low":
        draw.arc([cx - 2 * s, cy - 3.5 * s, cx + 4.5 * s, cy + 3.5 * s], 305, 55, fill=white, width=w)
    else: # normal
        draw.arc([cx - 2 * s, cy - 3.5 * s, cx + 4.5 * s, cy + 3.5 * s], 305, 55, fill=white, width=w)
        draw.arc([cx - 3 * s, cy - 6 * s, cx + 7 * s, cy + 6 * s], 305, 55, fill=white, width=w)
    return img

def create_battery_icon(state, size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size / 16.0
    cx, cy = size / 2.0, size / 2.0

    white = (255, 255, 255, 255)
    green = (34, 197, 94, 255)
    red = (239, 68, 68, 255)

    bw, bh = 10 * s, 6 * s
    bx, by = cx - 5.5 * s, cy - 3 * s
    w = max(1, int(1.0 * s))

    # Outer outline
    draw.rounded_rectangle([bx, by, bx + bw, by + bh], radius=1.5*s, outline=white, width=w)
    # Right nub cap
    draw.line([bx + bw + 1.2*s, cy - 1.5*s, bx + bw + 1.2*s, cy + 1.5*s], fill=white, width=w)

    # Inner Fill
    fill_col = red if state == "low" else green
    fill_w = 0.3 * (bw - 2*s) if state == "low" else 0.8 * (bw - 2*s)
    draw.rectangle([bx + 1.5*s, by + 1.5*s, bx + 1.5*s + fill_w, by + bh - 1.5*s], fill=fill_col)

    if state == "charging":
        bolt = [(cx - 0.5*s, by + 1*s), (cx + 1*s, cy - 0.5*s), (cx, cy - 0.5*s), (cx + 1*s, by + bh - 1*s), (cx - 0.5*s, cy + 0.5*s), (cx, cy + 0.5*s)]
        draw.polygon(bolt, fill=(255, 255, 255, 255))
    return img

def create_notification_icon(state, size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size / 16.0
    cx, cy = size / 2.0, size / 2.0

    white = (255, 255, 255, 255)
    red = (239, 68, 68, 255)
    w = max(1, int(1.0 * s))

    draw.arc([cx - 3.5 * s, cy - 5.5 * s, cx + 3.5 * s, cy + 1 * s], 180, 0, fill=white, width=w)
    draw.line([cx - 3.5 * s, cy - 2 * s, cx - 4.5 * s, cy + 2 * s], fill=white, width=w)
    draw.line([cx + 3.5 * s, cy - 2 * s, cx + 4.5 * s, cy + 2 * s], fill=white, width=w)
    draw.line([cx - 5.5 * s, cy + 2 * s, cx + 5.5 * s, cy + 2 * s], fill=white, width=w)
    draw.arc([cx - 1.5 * s, cy + 2 * s, cx + 1.5 * s, cy + 4.5 * s], 0, 180, fill=white, width=w)

    if state == "unread":
        rx, ry = cx + 3.5 * s, cy - 4 * s
        r = 1.8 * s
        draw.ellipse([rx - r, ry - r, rx + r, ry + r], fill=red, outline=(15, 23, 42, 255), width=1)
    return img

def generate_all():
    os.makedirs("assets/icons", exist_ok=True)

    icons = [
        ("icon_system_wifi_connected", lambda sz: create_wifi_icon("connected", sz)),
        ("icon_system_wifi_weak", lambda sz: create_wifi_icon("weak", sz)),
        ("icon_system_wifi_disconnected", lambda sz: create_wifi_icon("disconnected", sz)),
        ("icon_system_volume_normal", lambda sz: create_volume_icon("normal", sz)),
        ("icon_system_volume_low", lambda sz: create_volume_icon("low", sz)),
        ("icon_system_volume_muted", lambda sz: create_volume_icon("muted", sz)),
        ("icon_system_battery_normal", lambda sz: create_battery_icon("normal", sz)),
        ("icon_system_battery_charging", lambda sz: create_battery_icon("charging", sz)),
        ("icon_system_battery_low", lambda sz: create_battery_icon("low", sz)),
        ("icon_system_notification_normal", lambda sz: create_notification_icon("normal", sz)),
        ("icon_system_notification_unread", lambda sz: create_notification_icon("unread", sz)),
    ]

    header_content = """#ifndef SYS_ICONS_DATA_V11_H
#define SYS_ICONS_DATA_V11_H

#include <stdint.h>

"""

    for name, gen_fn in icons:
        img_32 = gen_fn(32)
        img_32.save(f"assets/icons/{name}.png")

        img_16 = gen_fn(16)
        img_16.save(f"assets/icons/{name}_16.png")

        for sz in [20, 24]:
            img_sz = gen_fn(sz)
            img_sz.save(f"assets/icons/{name}_{sz}.png")

        # Create C array for 16x16 native variant
        pixels_16 = list(img_16.getdata())
        array_name = f"g_{name}_rgba"
        header_content += f"static const uint8_t {array_name}[16 * 16 * 4] = {{\n"
        row_str = "    "
        for i, (r, g, b, a) in enumerate(pixels_16):
            row_str += f"{b}, {g}, {r}, {a}, "
            if (i + 1) % 4 == 0:
                header_content += row_str + "\n"
                row_str = "    "
        if row_str.strip():
            header_content += row_str + "\n"
        header_content += "};\n\n"

    header_content += "#endif // SYS_ICONS_DATA_V11_H\n"

    with open("kernel/ui/boasset/sys_icons_data_v11.h", "w") as f:
        f.write(header_content)

    print("Successfully generated all 11 transparent PNG icons (native 16x16) and sys_icons_data_v11.h!")

if __name__ == "__main__":
    generate_all()
