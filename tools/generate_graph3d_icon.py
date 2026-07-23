import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_futuristic_floating_3d_icon():
    size = 64
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    # --- Layer 1: Ambient Glow Canvas ---
    glow_layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow_layer)
    # Soft cyan/violet aura in center
    glow_draw.ellipse([12, 10, 52, 50], fill=(56, 189, 248, 45))
    glow_layer = glow_layer.filter(ImageFilter.GaussianBlur(radius=6))
    img.alpha_composite(glow_layer)

    # --- Layer 2: Outer Squircle Tile ---
    tile_layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    tile_draw = ImageDraw.Draw(tile_layer)
    x0, y0, x1, y1 = 3, 3, 60, 60
    
    # Dark futuristic tile with subtle gradient
    for y in range(y0, y1 + 1):
        t = (y - y0) / float(y1 - y0)
        r = int(15 * (1 - t) + 10 * t)
        g = int(23 * (1 - t) + 15 * t)
        b = int(42 * (1 - t) + 30 * t)
        tile_draw.line([(x0, y), (x1, y)], fill=(r, g, b, 230))

    # Squircle Mask (Radius 12)
    mask = Image.new("L", (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle([x0, y0, x1, y1], radius=12, fill=255)
    tile_layer.putalpha(mask)

    # Border
    tile_border = ImageDraw.Draw(tile_layer)
    tile_border.rounded_rectangle([x0, y0, x1, y1], radius=12, outline=(56, 189, 248, 140), width=1)
    img.alpha_composite(tile_layer)

    # --- Layer 3: Soft Ground Drop Shadow (Creates Floating Illusion) ---
    shadow_layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    shadow_draw = ImageDraw.Draw(shadow_layer)
    # Elliptical shadow underneath the floating 3D object
    shadow_draw.ellipse([14, 46, 50, 54], fill=(0, 0, 0, 140))
    shadow_layer = shadow_layer.filter(ImageFilter.GaussianBlur(radius=3))
    img.alpha_composite(shadow_layer)

    # --- Layer 4: Floating 3D Geometric Gem / Cube Object ---
    # Center of Floating 3D Object (Lifted up to (32, 25) above the shadow)
    cx, cy = 32, 25
    sx = 16 # Half width X
    sy = 9  # Half height Y (perspective tilt)
    h  = 16 # Vertical height of 3D box

    # 3D Vertices
    v_top_mid   = (cx, cy - sy)
    v_top_right = (cx + sx, cy)
    v_top_left  = (cx - sx, cy)
    v_top_bot   = (cx, cy + sy)

    v_bot_left  = (cx - sx, cy + h)
    v_bot_mid   = (cx, cy + sy + h)
    v_bot_right = (cx + sx, cy + h)

    obj_draw = ImageDraw.Draw(img)

    # 4A. Left Face (Medium Cyan/Blue Shaded Face)
    left_poly = [v_top_left, v_top_bot, v_bot_mid, v_bot_left]
    obj_draw.polygon(left_poly, fill=(14, 116, 144, 240)) # Deep Cyan
    # Gradient/Highlight on Left Face
    for i in range(h):
        t = i / float(h)
        r = int(14 * (1 - t) + 2 * t)
        g = int(165 * (1 - t) + 132 * t)
        b = int(233 * (1 - t) + 199 * t)
        y_step = cy + int(sy * 0.5) + i
        obj_draw.line([(cx - sx + 2, y_step - sy), (cx - 2, y_step)], fill=(r, g, b, 60))

    # 4B. Right Face (Dark Indigo/Violet Shadow Face)
    right_poly = [v_top_bot, v_top_right, v_bot_right, v_bot_mid]
    obj_draw.polygon(right_poly, fill=(67, 56, 202, 240)) # Indigo/Violet

    # 4C. Top Light Face (Bright Cyan/Teal Specular Face)
    top_poly = [v_top_mid, v_top_right, v_top_bot, v_top_left]
    obj_draw.polygon(top_poly, fill=(56, 189, 248, 255)) # Bright Cyan Top

    # Sub-facet highlight on top face for metallic 3D feel
    sub_top = [(cx, cy - sy + 3), (cx + sx - 5, cy), (cx, cy + sy - 3), (cx - sx + 5, cy)]
    obj_draw.polygon(sub_top, fill=(186, 230, 253, 220))

    # 4D. Glowing 3D Wireframe Edges & Apex Points
    edge_color = (255, 255, 255, 230)
    cyan_glow  = (56, 189, 248, 255)

    # Outer Edges
    obj_draw.line([v_top_mid, v_top_left], fill=edge_color, width=2)
    obj_draw.line([v_top_mid, v_top_right], fill=edge_color, width=2)
    obj_draw.line([v_top_left, v_bot_left], fill=edge_color, width=2)
    obj_draw.line([v_top_right, v_bot_right], fill=edge_color, width=2)
    obj_draw.line([v_bot_left, v_bot_mid], fill=edge_color, width=2)
    obj_draw.line([v_bot_right, v_bot_mid], fill=edge_color, width=2)
    
    # Center Crease Edges
    obj_draw.line([v_top_left, v_top_bot], fill=cyan_glow, width=1)
    obj_draw.line([v_top_right, v_top_bot], fill=cyan_glow, width=1)
    obj_draw.line([v_top_bot, v_bot_mid], fill=edge_color, width=2)

    # 4E. Apex Node Points (Glow dots at vertices)
    for px, py in [v_top_mid, v_top_left, v_top_right, v_top_bot, v_bot_mid]:
        obj_draw.ellipse([px - 2, py - 2, px + 2, py + 2], fill=(255, 255, 255, 255))

    # Save output PNG
    out_dir = r"d:\Signatures_OS\assets\icons"
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "graph3d.png")
    img.save(out_path, "PNG")
    print(f"[OK] Generated futuristic floating 3D benchmark icon: {out_path}")

if __name__ == "__main__":
    create_futuristic_floating_3d_icon()
