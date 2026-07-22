import os
import sys
import math
from PIL import Image, ImageFont, ImageDraw

# Input Font Definitions from Windows system fonts or fallbacks
FONT_CONFIGS = [
    {
        "role": "UI_REGULAR",
        "name": "SegoeUI-13-Regular",
        "font_path": "C:/Windows/Fonts/segoeui.ttf",
        "alt_path": "C:/Windows/Fonts/arial.ttf",
        "size": 13,
        "line_height": 16,
        "glyph_size": 16
    },
    {
        "role": "UI_MEDIUM",
        "name": "SegoeUI-13-Medium",
        "font_path": "C:/Windows/Fonts/segoeuisl.ttf",
        "alt_path": "C:/Windows/Fonts/segoeui.ttf",
        "size": 13,
        "line_height": 16,
        "glyph_size": 16
    },
    {
        "role": "UI_BOLD",
        "name": "SegoeUI-13-Bold",
        "font_path": "C:/Windows/Fonts/segoeuib.ttf",
        "alt_path": "C:/Windows/Fonts/arialbd.ttf",
        "size": 13,
        "line_height": 16,
        "glyph_size": 16
    },
    {
        "role": "CAPTION",
        "name": "SegoeUI-11-Caption",
        "font_path": "C:/Windows/Fonts/segoeui.ttf",
        "alt_path": "C:/Windows/Fonts/arial.ttf",
        "size": 11,
        "line_height": 14,
        "glyph_size": 14
    },
    {
        "role": "TITLE",
        "name": "SegoeUI-16-Title",
        "font_path": "C:/Windows/Fonts/segoeuib.ttf",
        "alt_path": "C:/Windows/Fonts/arialbd.ttf",
        "size": 16,
        "line_height": 20,
        "glyph_size": 20
    },
    {
        "role": "MONO",
        "name": "Consolas-13-Mono",
        "font_path": "C:/Windows/Fonts/consola.ttf",
        "alt_path": "C:/Windows/Fonts/CascadiaMono.ttf",
        "size": 13,
        "line_height": 16,
        "glyph_size": 16
    }
]

def load_font(primary, alt, size):
    if os.path.exists(primary):
        return ImageFont.truetype(primary, size)
    elif os.path.exists(alt):
        return ImageFont.truetype(alt, size)
    else:
        return ImageFont.load_default()

def build_bofont_asset(cfg):
    font = load_font(cfg["font_path"], cfg["alt_path"], cfg["size"])
    cols = 16
    rows = 8
    
    # 1. First pass: Measure maximum render bounds of d.text((3, 3), c) across all codepoints 32..126
    max_x2, max_y2 = 0, 0
    for cp in range(32, 127):
        c = chr(cp)
        big = Image.new("L", (64, 64), 0)
        d = ImageDraw.Draw(big)
        d.text((3, 3), c, font=font, fill=255)
        box = big.getbbox()
        if box:
            if box[2] > max_x2: max_x2 = box[2]
            if box[3] > max_y2: max_y2 = box[3]

    cell_w = max_x2 + 3
    cell_h = max_y2 + 3
    atlas_w = cols * cell_w
    atlas_h = rows * cell_h
    
    atlas_img = Image.new("L", (atlas_w, atlas_h), 0)
    glyphs_meta = []
    
    try:
        ascent, descent = font.getmetrics()
    except Exception:
        ascent = int(cfg["size"] * 0.8)
        descent = cfg["size"] - ascent
        
    line_height = cfg["line_height"]
    clipped_glyphs = 0
    
    for cp in range(128):
        c = chr(cp)
        col = cp % cols
        row = cp // cols
        cx = col * cell_w
        cy = row * cell_h
        
        if 32 <= cp <= 126:
            big = Image.new("L", (cell_w, cell_h), 0)
            d = ImageDraw.Draw(big)
            d.text((3, 3), c, font=font, fill=255)
            box = big.getbbox()
            
            if box:
                x1, y1, x2, y2 = box
                w = max(1, x2 - x1)
                h = max(1, y2 - y1)
                gx = cx + x1
                gy = cy + y1
                bearing_x = x1 - 3
                bearing_y = y1 - 3
                
                try:
                    adv = int(round(font.getlength(c)))
                    if adv <= 0: adv = w
                except Exception:
                    adv = w
                    
                g_crop = big.crop((x1, y1, x2, y2))
                atlas_img.paste(g_crop, (gx, gy))
                
                if x1 < 1 or y1 < 1 or gx + w >= cx + cell_w - 1 or gy + h >= cy + cell_h - 1:
                    clipped_glyphs += 1
            else:
                w, h = 1, 1
                gx, gy = cx + 2, cy + 2
                bearing_x, bearing_y = 0, 0
                try:
                    adv = int(round(font.getlength(c)))
                    if adv <= 0: adv = cfg["glyph_size"] // 2
                except Exception:
                    adv = cfg["glyph_size"] // 2
                
            u1 = gx / float(atlas_w)
            v1 = gy / float(atlas_h)
            u2 = (gx + w) / float(atlas_w)
            v2 = (gy + h) / float(atlas_h)
            
            glyphs_meta.append({
                "cp": cp,
                "advance_x": adv,
                "bearing_x": bearing_x,
                "bearing_y": bearing_y,
                "width": w,
                "height": h,
                "atlas_x": gx,
                "atlas_y": gy,
                "u1": u1, "v1": v1, "u2": u2, "v2": v2
            })
        else:
            w = 8
            h = line_height
            adv = cfg["glyph_size"] if cp not in (10, 13) else 0
            gx = cx + 2
            gy = cy + 2
            
            g_img = Image.new("L", (w, h), 0)
            if cp == 127:
                g_draw = ImageDraw.Draw(g_img)
                g_draw.rectangle([0, 0, w - 1, h - 1], outline=255)
                
            atlas_img.paste(g_img, (gx, gy))
            
            u1 = gx / float(atlas_w)
            v1 = gy / float(atlas_h)
            u2 = (gx + w) / float(atlas_w)
            v2 = (gy + h) / float(atlas_h)
            
            glyphs_meta.append({
                "cp": cp,
                "advance_x": adv,
                "bearing_x": 0,
                "bearing_y": 0,
                "width": w,
                "height": h,
                "atlas_x": gx,
                "atlas_y": gy,
                "u1": u1, "v1": v1, "u2": u2, "v2": v2
            })
            
    # Automated Atlas Validation Step
    contamination_pixels = 0
    for r in range(rows):
        for c in range(cols):
            cell_x = c * cell_w
            cell_y = r * cell_h
            # Verify 2-pixel transparent gutter around every cell
            for x in range(cell_x, cell_x + cell_w):
                if atlas_img.getpixel((x, cell_y)) > 0 or atlas_img.getpixel((x, cell_y + cell_h - 1)) > 0:
                    contamination_pixels += 1
            for y in range(cell_y, cell_y + cell_h):
                if atlas_img.getpixel((cell_x, y)) > 0 or atlas_img.getpixel((cell_x + cell_w - 1, y)) > 0:
                    contamination_pixels += 1
                    
    status = "PASS" if (contamination_pixels == 0 and clipped_glyphs == 0) else "FAIL"
    
    print("[BOFONT GENERATOR]")
    print(f"Role: {cfg['role']}")
    print("Glyphs: 95")
    print(f"Atlas: {atlas_w}x{atlas_h}")
    print(f"Cross-cell contamination: {contamination_pixels}")
    print(f"Clipped glyphs: {clipped_glyphs}")
    print(f"Status: {status}\n")

    atlas_bytes = list(atlas_img.tobytes())
    return {
        "cfg": cfg,
        "ascent": ascent,
        "descent": descent,
        "line_height": line_height,
        "atlas_w": atlas_w,
        "atlas_h": atlas_h,
        "glyphs": glyphs_meta,
        "bytes": atlas_bytes
    }

def generate_c_code(assets):
    header_path = "kernel/ui/bofont/bofont_assets.h"
    source_path = "kernel/ui/bofont/bofont_assets.c"
    
    with open(header_path, "w") as f:
        f.write("/* AUTO-GENERATED BY tools/generate_bofont.py — DO NOT HAND EDIT */\n")
        f.write("#ifndef KERNEL_BOFONT_ASSETS_H\n#define KERNEL_BOFONT_ASSETS_H\n\n")
        f.write("#include \"font_types.h\"\n\n")
        
        for a in assets:
            role = a["cfg"]["role"]
            f.write(f"extern const BOFontAsset g_bofont_asset_{role.lower()};\n")
            
        f.write("\n#endif // KERNEL_BOFONT_ASSETS_H\n")
        
    with open(source_path, "w") as f:
        f.write("/* AUTO-GENERATED BY tools/generate_bofont.py — DO NOT HAND EDIT */\n")
        f.write("#include \"bofont_assets.h\"\n\n")
        
        for a in assets:
            role = a["cfg"]["role"]
            name = a["cfg"]["name"]
            var_prefix = f"g_bofont_data_{role.lower()}"
            
            f.write(f"static const uint8_t {var_prefix}_atlas[{len(a['bytes'])}] = {{\n")
            for i in range(0, len(a['bytes']), 16):
                chunk = a['bytes'][i:i+16]
                hex_str = ", ".join([f"0x{b:02X}" for b in chunk])
                f.write(f"    {hex_str},\n")
            f.write("};\n\n")
            
            f.write(f"static const BOGlyphMeta {var_prefix}_glyphs[128] = {{\n")
            for g in a["glyphs"]:
                f.write(f"    {{ .codepoint = {g['cp']}, .advance_x = {g['advance_x']}, .bearing_x = {g['bearing_x']}, .bearing_y = {g['bearing_y']}, .width = {g['width']}, .height = {g['height']}, .atlas_x = {g['atlas_x']}, .atlas_y = {g['atlas_y']}, .u1 = {g['u1']:.6f}f, .v1 = {g['v1']:.6f}f, .u2 = {g['u2']:.6f}f, .v2 = {g['v2']:.6f}f }},\n")
            f.write("};\n\n")
            
            f.write(f"const BOFontAsset g_bofont_asset_{role.lower()} = {{\n")
            f.write(f"    .name = \"{name}\",\n")
            f.write(f"    .glyph_size = {a['cfg']['glyph_size']},\n")
            f.write(f"    .line_height = {a['line_height']},\n")
            f.write(f"    .ascender = {a['ascent']},\n")
            f.write(f"    .descender = {a['descent']},\n")
            f.write(f"    .atlas_w = {a['atlas_w']},\n")
            f.write(f"    .atlas_h = {a['atlas_h']},\n")
            f.write(f"    .atlas_data = {var_prefix}_atlas,\n")
            f.write(f"    .atlas_data_size = sizeof({var_prefix}_atlas),\n")
            f.write(f"    .glyphs = {var_prefix}_glyphs\n")
            f.write("};\n\n")

def main():
    print("[BOFONT GENERATOR] Rasterizing TTF fonts into 8-bit Alpha atlases...")
    assets = []
    for cfg in FONT_CONFIGS:
        asset = build_bofont_asset(cfg)
        assets.append(asset)
        
    generate_c_code(assets)
    print("[BOFONT GENERATOR] Successfully created bofont_assets.h and bofont_assets.c!")

if __name__ == "__main__":
    main()
