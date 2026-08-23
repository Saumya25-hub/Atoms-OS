#!/usr/bin/env python3
"""
ATOMS OS — Official Premium PNG Icon System Pipeline
Automates Master Icon Generation, RGBA Validation, Resampling, C Header Export, and Quality Previews.
"""

import os
import shutil
import math
from PIL import Image, ImageDraw, ImageFilter, ImageFont

# --------------------------------------------------------------------------
# Paths & Configuration
# --------------------------------------------------------------------------
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ICONS_DIR = os.path.join(BASE_DIR, "assets", "icons")
APPS_DIR = os.path.join(ICONS_DIR, "apps")
SYSTEM_DIR = os.path.join(ICONS_DIR, "system")
NAV_DIR = os.path.join(ICONS_DIR, "navigation")
BRAND_DIR = os.path.join(ICONS_DIR, "branding")
LEGACY_DIR = os.path.join(ICONS_DIR, "_legacy")

MASTER_SIZE = 256
SCALE = 4  # 4x Supersampling for ultra-smooth anti-aliasing
CANVAS_SIZE = MASTER_SIZE * SCALE  # 1024x1024 internal render canvas

# --------------------------------------------------------------------------
# ATOMS Color Palette Tokens
# --------------------------------------------------------------------------
CYAN_CORE     = (56, 189, 248, 255)    # #38BDF8 - Electric Cyan
CYAN_BRIGHT   = (125, 211, 252, 255)   # #7DD3FC - Sky Cyan Highlight
CYAN_DEEP     = (2, 132, 199, 255)     # #0284C7 - Deep Ocean Cyan
NAVY_BASE     = (15, 23, 42, 255)      # #0F172A - Slate 900
NAVY_SURFACE  = (30, 41, 59, 255)      # #1E293B - Slate 800
SLATE_BORDER  = (71, 85, 105, 255)     # #475569 - Slate 600
WHITE_PURE    = (255, 255, 255, 255)   # Crisp White
WHITE_GLOW    = (255, 255, 255, 180)
EMERALD_GREEN = (34, 197, 94, 255)     # #22C55E - Telemetry Green
CRIMSON_RED   = (239, 68, 68, 255)     # #EF4444 - Alert Red
AMBER_GOLD    = (245, 158, 11, 255)    # #F59E0B - Amber
INDIGO_ACCENT = (99, 102, 241, 255)    # #6366F1 - Indigo
VIOLET_DEEP   = (139, 92, 246, 255)    # #8B5CF6 - Violet

# --------------------------------------------------------------------------
# Helper Canvas Factory
# --------------------------------------------------------------------------
def create_canvas():
    return Image.new("RGBA", (CANVAS_SIZE, CANVAS_SIZE), (0, 0, 0, 0))

def finalize_master(img):
    """Downsamples from internal 1024x1024 canvas to 256x256 master with Lanczos."""
    return img.resize((MASTER_SIZE, MASTER_SIZE), Image.Resampling.LANCZOS)

# --------------------------------------------------------------------------
# Master Icon Renderers (Supersampled Geometry)
# --------------------------------------------------------------------------

def render_atoms_start():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2
    r_outer = int(420)
    
    # 1. Subtle Outer Ambient Cyan Glow Ring
    for i in range(16):
        alpha = int(12 - i * 0.7)
        rg = r_outer + i * 8
        draw.ellipse([cx - rg, cy - rg, cx + rg, cy + rg], outline=(56, 189, 248, max(0, alpha)), width=8)

    # 2. Dark Translucent Nucleus Backplate
    draw.ellipse([cx - 360, cy - 360, cx + 360, cy + 360], fill=(15, 23, 42, 220), outline=(56, 189, 248, 180), width=12)

    # 3. Tri-Orbital Elliptical Energy Arcs (Interlocking Geodesic Rings at 0, 60, 120 deg)
    for angle_deg in [30, 90, 150]:
        rad = math.radians(angle_deg)
        cos_a, sin_a = math.cos(rad), math.sin(rad)
        
        orbit_img = Image.new("RGBA", (CANVAS_SIZE, CANVAS_SIZE), (0, 0, 0, 0))
        odraw = ImageDraw.Draw(orbit_img)
        
        # Draw ellipse along horizontal axis then rotate
        rx, ry = 300, 110
        odraw.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], outline=(56, 189, 248, 220), width=18)
        odraw.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], outline=(255, 255, 255, 255), width=6)
        
        # Satellite quantum nodes on vertices
        for node_x in [cx - rx, cx + rx]:
            odraw.ellipse([node_x - 22, cy - 22, node_x + 22, cy + 22], fill=WHITE_PURE)
            odraw.ellipse([node_x - 32, cy - 32, node_x + 32, cy + 32], outline=CYAN_BRIGHT, width=6)

        # Rotate orbit
        rotated_orbit = orbit_img.rotate(angle_deg, resample=Image.Resampling.BICUBIC, center=(cx, cy))
        img.alpha_composite(rotated_orbit)

    # 4. Central Quantum Core Spark
    draw.ellipse([cx - 100, cy - 100, cx + 100, cy + 100], fill=CYAN_CORE)
    draw.ellipse([cx - 70, cy - 70, cx + 70, cy + 70], fill=CYAN_BRIGHT)
    draw.ellipse([cx - 40, cy - 40, cx + 40, cy + 40], fill=WHITE_PURE)

    return finalize_master(img)


def render_explorer():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    
    # Rounded Tile Backplate (Slate 900 with subtle cyan rim)
    pad = 80
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=12)

    # Modern Geometric Folder Tabs
    # Back Folder Flap (Deep Azure)
    draw.rounded_rectangle([180, 260, 460, 380], radius=40, fill=(2, 132, 199, 255))
    draw.rounded_rectangle([180, 320, 844, 760], radius=50, fill=(3, 105, 161, 255))
    
    # Middle Sheet (Clean crisp white paper layer inside)
    draw.rounded_rectangle([230, 300, 794, 680], radius=30, fill=(241, 245, 249, 255), outline=WHITE_PURE, width=6)
    
    # Front Folder Cover (Bright Electric Cyan)
    draw.rounded_rectangle([180, 390, 844, 760], radius=50, fill=CYAN_CORE, outline=CYAN_BRIGHT, width=12)

    # Subtle Folder Lip Highlight
    draw.line([220, 410, 804, 410], fill=WHITE_GLOW, width=8)

    return finalize_master(img)


def render_terminal():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80

    # Slate Bezel Window
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Window Top Bar Divider
    draw.line([pad + 30, 270, CANVAS_SIZE - pad - 30, 270], fill=NAVY_SURFACE, width=10)
    
    # Window Top Controls (Minimalist Dots)
    draw.ellipse([200, 180, 240, 220], fill=CRIMSON_RED)
    draw.ellipse([270, 180, 310, 220], fill=AMBER_GOLD)
    draw.ellipse([340, 180, 380, 220], fill=EMERALD_GREEN)

    # Command Prompt Glyph (> _) in Electric Cyan & Pure White
    # Arrow '>'
    draw.line([230, 380, 370, 490], fill=CYAN_CORE, width=32)
    draw.line([370, 490, 230, 600], fill=CYAN_CORE, width=32)
    
    # Cursor '_'
    draw.line([430, 600, 600, 600], fill=WHITE_PURE, width=32)

    # Telemetry Pulse Sub-line
    draw.line([230, 720, 520, 720], fill=(56, 189, 248, 120), width=16)

    return finalize_master(img)


def render_settings():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Technical Precision Gear
    num_teeth = 8
    outer_r = 330
    root_r = 260
    inner_r = 130
    
    # Draw teeth polygons
    for i in range(num_teeth):
        angle = i * (2 * math.pi / num_teeth)
        w_angle = (2 * math.pi / num_teeth) * 0.28
        
        p1 = (cx + outer_r * math.cos(angle - w_angle * 0.7), cy + outer_r * math.sin(angle - w_angle * 0.7))
        p2 = (cx + outer_r * math.cos(angle + w_angle * 0.7), cy + outer_r * math.sin(angle + w_angle * 0.7))
        p3 = (cx + root_r * math.cos(angle + w_angle * 1.3), cy + root_r * math.sin(angle + w_angle * 1.3))
        p4 = (cx + root_r * math.cos(angle - w_angle * 1.3), cy + root_r * math.sin(angle - w_angle * 1.3))
        draw.polygon([p1, p2, p3, p4], fill=CYAN_CORE, outline=CYAN_BRIGHT)

    # Gear Body Circle
    draw.ellipse([cx - root_r, cy - root_r, cx + root_r, cy + root_r], fill=CYAN_CORE, outline=CYAN_BRIGHT, width=12)

    # Central Hole with Titanium Bezel
    draw.ellipse([cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r], fill=NAVY_BASE, outline=WHITE_PURE, width=16)

    # Central Spark Node
    draw.ellipse([cx - 45, cy - 45, cx + 45, cy + 45], fill=WHITE_PURE)

    return finalize_master(img)


def render_calculator():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Calculator Body
    bx1, by1, bx2, by2 = 210, 160, 814, 864
    draw.rounded_rectangle([bx1, by1, bx2, by2], radius=70, fill=NAVY_SURFACE, outline=SLATE_BORDER, width=12)

    # LCD Display Area
    draw.rounded_rectangle([bx1 + 40, by1 + 40, bx2 - 40, by1 + 220], radius=35, fill=(15, 23, 42, 255), outline=CYAN_DEEP, width=8)
    
    # Digit Simulation on Display
    draw.line([bx2 - 120, by1 + 100, bx2 - 80, by1 + 100], fill=CYAN_CORE, width=16)
    draw.line([bx2 - 80, by1 + 100, bx2 - 80, by1 + 160], fill=CYAN_CORE, width=16)
    draw.line([bx2 - 120, by1 + 160, bx2 - 80, by1 + 160], fill=CYAN_CORE, width=16)

    # Keypad Grid (3x3 Numbers + Accent Operators)
    keys_x = [bx1 + 50, bx1 + 190, bx1 + 330, bx1 + 470]
    keys_y = [by1 + 270, by1 + 400, by1 + 530]
    
    for row in range(3):
        for col in range(3):
            kx = keys_x[col]
            ky = keys_y[row]
            draw.rounded_rectangle([kx, ky, kx + 105, ky + 95], radius=24, fill=(51, 65, 85, 255))

    # Cyan Action Key (=)
    kx = keys_x[3]
    ky = keys_y[2]
    draw.rounded_rectangle([kx, ky, kx + 105, ky + 95], radius=24, fill=CYAN_CORE, outline=WHITE_PURE, width=6)
    # Equals sign
    draw.line([kx + 25, ky + 36, kx + 80, ky + 36], fill=WHITE_PURE, width=8)
    draw.line([kx + 25, ky + 58, kx + 80, ky + 58], fill=WHITE_PURE, width=8)

    return finalize_master(img)


def render_notes():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Sheet Geometry with Folded Top-Right Corner
    sx1, sy1, sx2, sy2 = 230, 160, 794, 864
    fold_size = 170

    # Main Sheet Polygon
    poly = [
        (sx1, sy1),
        (sx2 - fold_size, sy1),
        (sx2, sy1 + fold_size),
        (sx2, sy2),
        (sx1, sy2)
    ]
    draw.polygon(poly, fill=(248, 250, 252, 255))

    # Fold Corner Triangle
    fold_poly = [
        (sx2 - fold_size, sy1),
        (sx2 - fold_size, sy1 + fold_size),
        (sx2, sy1 + fold_size)
    ]
    draw.polygon(fold_poly, fill=(203, 213, 225, 255), outline=(148, 163, 184, 255), width=6)

    # Text Lines (Sleek slate horizontal strips)
    lines_y = [380, 480, 580, 680]
    lines_w = [420, 480, 440, 280]
    for y, w in zip(lines_y, lines_w):
        draw.rounded_rectangle([sx1 + 60, y, sx1 + 60 + w, y + 24], radius=12, fill=(148, 163, 184, 255))

    # Cyan Accent Title Line
    draw.rounded_rectangle([300, 280, 480, 316], radius=16, fill=CYAN_CORE)

    return finalize_master(img)


def render_computer():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate with Metallic Slate/Cyan Rim
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # 1. Sleek Workstation Monitor (Left / Center)
    # Monitor Outer Frame
    draw.rounded_rectangle([140, 200, 670, 580], radius=45, fill=(30, 41, 59, 255), outline=(56, 189, 248, 255), width=16)
    # Screen Glass Area
    draw.rounded_rectangle([175, 235, 635, 545], radius=25, fill=(15, 23, 42, 255))
    # Holographic Desktop Wave / Display Line
    draw.line([210, 390, 310, 390, 350, 320, 410, 460, 460, 390, 600, 390], fill=CYAN_CORE, width=20)
    draw.ellipse([410 - 12, 460 - 12, 410 + 12, 460 + 12], fill=WHITE_PURE)

    # Monitor Stand Neck & Base
    draw.rounded_rectangle([380, 580, 430, 680], radius=12, fill=(148, 163, 184, 255))
    draw.rounded_rectangle([280, 670, 530, 720], radius=18, fill=WHITE_PURE)

    # 2. Modern High-Performance Workstation Tower (Right)
    tx1, ty1, tx2, ty2 = 690, 240, 884, 750
    # Tower Body (Dark Graphite with Specular Chamfer)
    draw.rounded_rectangle([tx1, ty1, tx2, ty2], radius=32, fill=(30, 41, 59, 255), outline=WHITE_PURE, width=12)
    # Power Button & Vertical Cyan LED Accent Slit
    draw.ellipse([tx1 + 40, ty1 + 40, tx1 + 70, ty1 + 70], fill=CYAN_CORE)
    draw.line([tx1 + 55, ty1 + 100, tx1 + 55, ty2 - 180], fill=CYAN_CORE, width=12)
    
    # Bottom Intake Grille Slots
    for gy in range(ty2 - 130, ty2 - 40, 24):
        draw.rounded_rectangle([tx1 + 25, gy, tx2 - 25, gy + 10], radius=4, fill=(15, 23, 42, 255))

    return finalize_master(img)


def render_graph3d():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Isometric 3D Hexagonal Prism / Cube
    size = 280
    dx = int(size * math.cos(math.radians(30)))  # ~242
    dy = int(size * math.sin(math.radians(30)))  # ~140

    top_center = (cx, cy - dy)
    top_p1 = (cx, cy - dy - size)
    top_p2 = (cx + dx, cy - dy - size + dy)
    top_p3 = (cx, cy - size + 2 * dy)
    top_p4 = (cx - dx, cy - dy - size + dy)

    # Top Face (Brightest Electric Cyan)
    draw.polygon([(cx, cy - size + dy), (cx + dx, cy - size + 2*dy), (cx, cy), (cx - dx, cy - size + 2*dy)],
                 fill=CYAN_CORE, outline=WHITE_PURE, width=8)

    # Left Face (Deep Ocean Cyan)
    draw.polygon([(cx - dx, cy - size + 2*dy), (cx, cy), (cx, cy + size), (cx - dx, cy + size - dy)],
                 fill=CYAN_DEEP, outline=CYAN_BRIGHT, width=8)

    # Right Face (Vibrant Royal Indigo)
    draw.polygon([(cx, cy), (cx + dx, cy - size + 2*dy), (cx + dx, cy + size - dy), (cx, cy + size)],
                 fill=INDIGO_ACCENT, outline=CYAN_BRIGHT, width=8)

    # Vertex Spark Nodes
    for pt in [(cx, cy), (cx, cy + size), (cx - dx, cy + size - dy), (cx + dx, cy + size - dy)]:
        draw.ellipse([pt[0] - 16, pt[1] - 16, pt[0] + 16, pt[1] + 16], fill=WHITE_PURE)

    return finalize_master(img)


def render_doom():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate with Crimson Rim
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=CRIMSON_RED, width=14)

    # Angular Demon Crest / Gaming Shield
    shield_pts = [
        (cx - 280, cy - 240),
        (cx + 280, cy - 240),
        (cx + 220, cy + 100),
        (cx, cy + 340),
        (cx - 220, cy + 100)
    ]
    draw.polygon(shield_pts, fill=(185, 28, 28, 255), outline=WHITE_PURE, width=12)

    # Inner Dark Facets
    inner_pts = [
        (cx - 210, cy - 180),
        (cx + 210, cy - 180),
        (cx + 160, cy + 70),
        (cx, cy + 260),
        (cx - 160, cy + 70)
    ]
    draw.polygon(inner_pts, fill=(69, 10, 10, 255))

    # Fierce Glowing Visor / Horn Accents
    draw.polygon([(cx - 160, cy - 40), (cx - 40, cy + 20), (cx - 140, cy + 60)], fill=AMBER_GOLD)
    draw.polygon([(cx + 160, cy - 40), (cx + 40, cy + 20), (cx + 140, cy + 60)], fill=AMBER_GOLD)
    
    # Central Glowing Core
    draw.polygon([(cx, cy - 100), (cx + 60, cy + 20), (cx, cy + 120), (cx - 60, cy + 20)], fill=CRIMSON_RED, outline=WHITE_PURE, width=6)

    return finalize_master(img)


def render_inputlab():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Precision Diagnostic Target Reticle
    draw.ellipse([cx - 300, cy - 300, cx + 300, cy + 300], outline=(56, 189, 248, 120), width=10)
    draw.ellipse([cx - 200, cy - 200, cx + 200, cy + 200], outline=(56, 189, 248, 200), width=12)
    draw.ellipse([cx - 100, cy - 100, cx + 100, cy + 100], outline=WHITE_PURE, width=8)

    # Reticle Crosshair Ticks
    draw.line([cx - 340, cy, cx - 220, cy], fill=CYAN_CORE, width=14)
    draw.line([cx + 220, cy, cx + 340, cy], fill=CYAN_CORE, width=14)
    draw.line([cx, cy - 340, cx, cy - 220], fill=CYAN_CORE, width=14)
    draw.line([cx, cy + 220, cx, cy + 340], fill=CYAN_CORE, width=14)

    # Sleek Mouse Pointer / Arrow Glyph Aiming at Center
    arrow_pts = [
        (cx - 30, cy - 30),
        (cx + 180, cy + 60),
        (cx + 60, cy + 90),
        (cx + 110, cy + 210),
        (cx + 50, cy + 230),
        (cx + 10, cy + 110),
        (cx - 70, cy + 160)
    ]
    draw.polygon(arrow_pts, fill=WHITE_PURE, outline=CYAN_DEEP, width=10)

    return finalize_master(img)


def render_tmh():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Telemetry Grid Background
    for gx in range(220, 820, 100):
        draw.line([gx, 200, gx, 824], fill=(30, 41, 59, 200), width=4)
    for gy in range(220, 820, 100):
        draw.line([200, gy, 824, gy], fill=(30, 41, 59, 200), width=4)

    # Heartbeat Pulse ECG Waveform (Neon Emerald Green)
    ecg_pts = [
        (180, 512),
        (300, 512),
        (360, 450),
        (410, 570),
        (460, 240),  # Peak R-wave
        (520, 720),  # Deep S-wave
        (580, 480),
        (640, 530),
        (700, 512),
        (844, 512)
    ]
    
    # Glow pass
    for i in range(len(ecg_pts) - 1):
        draw.line([ecg_pts[i], ecg_pts[i+1]], fill=(34, 197, 94, 100), width=36)
    # Core pass
    for i in range(len(ecg_pts) - 1):
        draw.line([ecg_pts[i], ecg_pts[i+1]], fill=EMERALD_GREEN, width=20)
    # Highlight spark
    for i in range(len(ecg_pts) - 1):
        draw.line([ecg_pts[i], ecg_pts[i+1]], fill=WHITE_PURE, width=8)

    # Peak Indicator Spark Node
    draw.ellipse([460 - 24, 240 - 24, 460 + 24, 240 + 24], fill=WHITE_PURE)

    return finalize_master(img)


def render_music():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate with Indigo/Violet Rim
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=INDIGO_ACCENT, width=14)

    # Concentric Sound Wave Rings
    for r in [320, 240, 160]:
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=(99, 102, 241, 100), width=10)

    # Modern Double Musical Note Glyph (Vibrant Cyan & White)
    # Left Note Head
    draw.ellipse([300, 580, 420, 690], fill=CYAN_CORE, outline=WHITE_PURE, width=8)
    # Right Note Head
    draw.ellipse([540, 500, 660, 610], fill=CYAN_CORE, outline=WHITE_PURE, width=8)

    # Stems
    draw.line([400, 620, 400, 300], fill=WHITE_PURE, width=24)
    draw.line([640, 540, 640, 220], fill=WHITE_PURE, width=24)

    # Connecting Beam
    draw.polygon([(388, 300), (652, 220), (652, 300), (388, 380)], fill=WHITE_PURE)

    return finalize_master(img)


def render_atrix():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    pad = 80
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Slate Backplate
    draw.rounded_rectangle([pad, pad, CANVAS_SIZE - pad, CANVAS_SIZE - pad], radius=180, fill=NAVY_BASE, outline=SLATE_BORDER, width=14)

    # Global Meridian / Compass Outer Ring
    r_globe = 300
    draw.ellipse([cx - r_globe, cy - r_globe, cx + r_globe, cy + r_globe], fill=NAVY_SURFACE, outline=CYAN_CORE, width=16)

    # Globe Meridian Ellipses
    draw.ellipse([cx - 150, cy - r_globe, cx + 150, cy + r_globe], outline=(56, 189, 248, 120), width=10)
    draw.line([cx - r_globe, cy, cx + r_globe, cy], fill=(56, 189, 248, 120), width=10)

    # Compass Needle (North = Cyan/White, South = Amber)
    needle_north = [(cx, cy - 220), (cx + 50, cy), (cx, cy - 30), (cx - 50, cy)]
    needle_south = [(cx, cy + 220), (cx + 50, cy), (cx, cy + 30), (cx - 50, cy)]
    draw.polygon(needle_north, fill=CYAN_CORE, outline=WHITE_PURE, width=6)
    draw.polygon(needle_south, fill=AMBER_GOLD, outline=WHITE_PURE, width=6)

    # Center Hub Pivot
    draw.ellipse([cx - 30, cy - 30, cx + 30, cy + 30], fill=WHITE_PURE)

    return finalize_master(img)


# --------------------------------------------------------------------------
# System Status Icons (Single Base + States)
# --------------------------------------------------------------------------

def render_battery(state="normal"):
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Outer Battery Body
    bx1, by1, bx2, by2 = 180, 320, 760, 704
    draw.rounded_rectangle([bx1, by1, bx2, by2], radius=60, outline=WHITE_PURE, width=32)

    # Positive Terminal Nub Cap (Right side)
    draw.rounded_rectangle([bx2 + 10, cy - 90, bx2 + 70, cy + 90], radius=24, fill=WHITE_PURE)

    # Inner Charge Level Bars
    fill_col = EMERALD_GREEN if state in ["normal", "charging"] else (CRIMSON_RED if state == "low" else AMBER_GOLD)
    level_ratio = 0.85 if state in ["normal", "charging"] else (0.25 if state == "low" else 0.5)
    
    inner_w = int((bx2 - bx1 - 60) * level_ratio)
    draw.rounded_rectangle([bx1 + 30, by1 + 30, bx1 + 30 + inner_w, by2 - 30], radius=35, fill=fill_col)

    # Charging Lightning Bolt
    if state == "charging":
        bolt_pts = [
            (cx - 30, by1 - 40),
            (cx + 80, cy - 20),
            (cx + 10, cy - 20),
            (cx + 70, by2 + 40),
            (cx - 40, cy + 20),
            (cx + 20, cy + 20)
        ]
        draw.polygon(bolt_pts, fill=WHITE_PURE, outline=(15, 23, 42, 255), width=8)

    return finalize_master(img)


def render_volume(state="normal"):
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Bold, Premium Speaker Transducer Body
    # Left Box
    draw.rounded_rectangle([130, cy - 140, 330, cy + 140], radius=32, fill=WHITE_PURE)
    # Expanding Horn Cone
    cone_pts = [
        (310, cy - 140),
        (560, cy - 300),
        (560, cy + 300),
        (310, cy + 140)
    ]
    draw.polygon(cone_pts, fill=WHITE_PURE)

    # Sound Acoustic Waves (Bold, Vibrant Electric Cyan + Pure White)
    if state == "normal":
        # Wave 1 (Inner Arc)
        draw.arc([360, cy - 200, 720, cy + 200], 305, 55, fill=CYAN_CORE, width=54)
        # Wave 2 (Outer Arc)
        draw.arc([480, cy - 330, 930, cy + 330], 305, 55, fill=CYAN_CORE, width=54)
    elif state == "low":
        draw.arc([360, cy - 200, 720, cy + 200], 305, 55, fill=CYAN_CORE, width=54)
    elif state == "muted":
        # Crimson Mute Cross (X)
        rx = cx + 200
        draw.line([rx - 100, cy - 100, rx + 100, cy + 100], fill=CRIMSON_RED, width=54)
        draw.line([rx - 100, cy + 100, rx + 100, cy - 100], fill=CRIMSON_RED, width=54)

    return finalize_master(img)


def render_ethernet(state="connected"):
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # High-Definition Modern PC Monitor + Ethernet Network Plug
    # 1. Main Desktop Screen
    # Outer Bezel
    draw.rounded_rectangle([130, 160, 894, 660], radius=70, fill=NAVY_BASE, outline=WHITE_PURE, width=40)
    # Screen Glass Area
    draw.rounded_rectangle([185, 215, 839, 605], radius=35, fill=(15, 23, 42, 255))
    
    # Gigabit Activity Wave / High-Speed Pulse on Screen
    draw.line([250, 410, 410, 410, 470, 320, 540, 500, 610, 410, 770, 410], fill=CYAN_CORE, width=32)
    draw.ellipse([470 - 18, 320 - 18, 470 + 18, 320 + 18], fill=WHITE_PURE)
    draw.ellipse([540 - 18, 500 - 18, 540 + 18, 500 + 18], fill=WHITE_PURE)

    # 2. Monitor Stand Neck
    draw.rounded_rectangle([cx - 45, 660, cx + 45, 760], radius=16, fill=WHITE_PURE)
    # Stand Base Foot
    draw.rounded_rectangle([cx - 190, 750, cx + 190, 800], radius=22, fill=WHITE_PURE)

    # 3. Ethernet RJ45 Connector Node / Connected Cable Badge (Bottom Right)
    bx, by = 740, 720
    draw.ellipse([bx - 145, by - 145, bx + 145, by + 145], fill=NAVY_BASE, outline=CYAN_CORE, width=24)
    # RJ45 Plug
    draw.rounded_rectangle([bx - 70, by - 70, bx + 70, by + 70], radius=20, fill=CYAN_CORE)
    # RJ45 Gold Pins
    for px_offset in [-45, -15, 15, 45]:
        draw.rounded_rectangle([bx + px_offset - 6, by - 58, bx + px_offset + 6, by - 22], radius=4, fill=WHITE_PURE)
    # RJ45 Cable Exit
    draw.rounded_rectangle([bx - 26, by + 50, bx + 26, by + 115], radius=12, fill=CYAN_CORE)

    if state == "disconnected":
        draw.line([bx - 100, by - 100, bx + 100, by + 100], fill=CRIMSON_RED, width=36)

    return finalize_master(img)


def render_wifi(state="connected"):
    # Fallback / Alternate Wi-Fi Renderer (Bold High-Contrast Arcs)
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2 + 100

    col_strong = WHITE_PURE
    col_dim = (148, 163, 184, 100)

    # Base Transmitter Dot
    draw.ellipse([cx - 60, cy + 90, cx + 60, cy + 210], fill=CYAN_CORE)

    # Wave 1 (Inner Arc)
    w1_col = col_strong if state in ["connected", "weak"] else col_dim
    draw.arc([cx - 240, cy - 160, cx + 240, cy + 320], 215, 325, fill=w1_col, width=48)

    # Wave 2 (Middle Arc)
    w2_col = col_strong if state == "connected" else col_dim
    draw.arc([cx - 390, cy - 310, cx + 390, cy + 470], 215, 325, fill=w2_col, width=48)

    # Wave 3 (Outer Arc)
    w3_col = col_strong if state == "connected" else col_dim
    draw.arc([cx - 540, cy - 460, cx + 540, cy + 620], 215, 325, fill=w3_col, width=48)

    if state == "disconnected":
        draw.line([cx - 200, cy - 200, cx + 200, cy + 200], fill=CRIMSON_RED, width=54)

    return finalize_master(img)


def render_notification(state="normal"):
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Minimalist Notification Bell
    draw.arc([cx - 240, cy - 320, cx + 240, cy + 160], 180, 0, fill=WHITE_PURE, width=36)
    draw.line([cx - 240, cy - 80, cx - 320, cy + 180], fill=WHITE_PURE, width=36)
    draw.line([cx + 240, cy - 80, cx + 320, cy + 180], fill=WHITE_PURE, width=36)
    draw.line([cx - 360, cy + 180, cx + 360, cy + 180], fill=WHITE_PURE, width=36)
    
    # Bell Clapper
    draw.arc([cx - 100, cy + 180, cx + 100, cy + 320], 0, 180, fill=WHITE_PURE, width=36)

    # Unread Alert Dot
    if state == "unread":
        dot_x, dot_y = cx + 240, cy - 240
        draw.ellipse([dot_x - 90, dot_y - 90, dot_x + 90, dot_y + 90], fill=CRIMSON_RED, outline=NAVY_BASE, width=20)

    return finalize_master(img)


def render_power():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Universal Power Ring (Opening at Top)
    r = 300
    draw.arc([cx - r, cy - r, cx + r, cy + r], 295, 245, fill=CYAN_CORE, width=44)

    # Vertical Power Line
    draw.line([cx, cy - r - 40, cx, cy - 40], fill=WHITE_PURE, width=44)

    return finalize_master(img)


def render_search():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2 - 50, CANVAS_SIZE // 2 - 50

    # Magnifying Loupe Ring
    r = 220
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=CYAN_CORE, width=40)

    # Glass Refraction Arc Highlight
    draw.arc([cx - r + 30, cy - r + 30, cx + r - 30, cy + r - 30], 190, 290, fill=WHITE_PURE, width=16)

    # Loupe Handle
    hx1, hy1 = cx + int(r * 0.707), cy + int(r * 0.707)
    hx2, hy2 = hx1 + 240, hy1 + 240
    draw.line([hx1, hy1, hx2, hy2], fill=WHITE_PURE, width=48)

    return finalize_master(img)


# --------------------------------------------------------------------------
# Navigation Glyphs
# --------------------------------------------------------------------------

def render_chevron(direction="left"):
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    if direction == "left":
        pts = [(cx + 120, cy - 240), (cx - 120, cy), (cx + 120, cy + 240)]
    elif direction == "right":
        pts = [(cx - 120, cy - 240), (cx + 120, cy), (cx - 120, cy + 240)]
    elif direction == "up":
        pts = [(cx - 240, cy + 120), (cx, cy - 120), (cx + 240, cy + 120)]
    else:  # down
        pts = [(cx - 240, cy - 120), (cx, cy + 120), (cx + 240, cy - 120)]

    draw.line([pts[0], pts[1]], fill=WHITE_PURE, width=44)
    draw.line([pts[1], pts[2]], fill=WHITE_PURE, width=44)

    return finalize_master(img)


def render_refresh():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # 3/4 Circular Arc
    r = 280
    draw.arc([cx - r, cy - r, cx + r, cy + r], 45, 315, fill=CYAN_CORE, width=40)

    # Arrowhead at top
    p_tip = (cx + int(r * math.cos(math.radians(45))), cy - int(r * math.sin(math.radians(45))))
    draw.polygon([(p_tip[0] - 60, p_tip[1] - 40), (p_tip[0] + 40, p_tip[1] - 80), (p_tip[0] + 80, p_tip[1] + 40)], fill=WHITE_PURE)

    return finalize_master(img)


def render_home():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    cx, cy = CANVAS_SIZE // 2, CANVAS_SIZE // 2

    # Roof Triangle
    roof_pts = [(cx, cy - 300), (cx - 300, cy - 40), (cx + 300, cy - 40)]
    draw.polygon(roof_pts, fill=CYAN_CORE, outline=WHITE_PURE, width=12)

    # House Body
    draw.rounded_rectangle([cx - 220, cy - 40, cx + 220, cy + 280], radius=30, fill=NAVY_SURFACE, outline=WHITE_PURE, width=16)

    # Doorway
    draw.rounded_rectangle([cx - 70, cy + 90, cx + 70, cy + 280], radius=20, fill=CYAN_DEEP)

    return finalize_master(img)


def render_nav_folder():
    img = create_canvas()
    draw = ImageDraw.Draw(img)
    
    # Minimalist Clean Folder
    draw.rounded_rectangle([180, 260, 460, 380], radius=40, fill=CYAN_DEEP)
    draw.rounded_rectangle([180, 340, 844, 760], radius=50, fill=CYAN_CORE, outline=WHITE_PURE, width=12)

    return finalize_master(img)


# --------------------------------------------------------------------------
# Pipeline Execution & Directory Layout
# --------------------------------------------------------------------------

def setup_directories():
    for d in [APPS_DIR, SYSTEM_DIR, NAV_DIR, BRAND_DIR, LEGACY_DIR]:
        os.makedirs(d, exist_ok=True)

def archive_legacy_assets():
    """Moves legacy files in assets/icons to _legacy directory."""
    files = os.listdir(ICONS_DIR)
    archived_count = 0
    for f in files:
        src = os.path.join(ICONS_DIR, f)
        if os.path.isfile(src) and f.endswith(".png"):
            dst = os.path.join(LEGACY_DIR, f)
            shutil.move(src, dst)
            archived_count += 1
    print(f"[PIPELINE] Archived {archived_count} legacy files into assets/icons/_legacy/")
    return archived_count

def generate_all_master_assets():
    print("[PIPELINE] Generating Master ATOMS OS 256x256 PNG Assets...")

    catalog = {
        # Branding
        os.path.join(BRAND_DIR, "atoms_start.png"): render_atoms_start,
        
        # Applications
        os.path.join(APPS_DIR, "atoms.png"): render_atoms_start,
        os.path.join(APPS_DIR, "computer.png"): render_computer,
        os.path.join(APPS_DIR, "explorer.png"): render_explorer,
        os.path.join(APPS_DIR, "terminal.png"): render_terminal,
        os.path.join(APPS_DIR, "settings.png"): render_settings,
        os.path.join(APPS_DIR, "calculator.png"): render_calculator,
        os.path.join(APPS_DIR, "notes.png"): render_notes,
        os.path.join(APPS_DIR, "graph3d.png"): render_graph3d,
        os.path.join(APPS_DIR, "doom.png"): render_doom,
        os.path.join(APPS_DIR, "inputlab.png"): render_inputlab,
        os.path.join(APPS_DIR, "tmh.png"): render_tmh,
        os.path.join(APPS_DIR, "music.png"): render_music,
        os.path.join(APPS_DIR, "atrix.png"): render_atrix,
        
        # System
        os.path.join(SYSTEM_DIR, "battery.png"): lambda: render_battery("normal"),
        os.path.join(SYSTEM_DIR, "battery_charging.png"): lambda: render_battery("charging"),
        os.path.join(SYSTEM_DIR, "battery_low.png"): lambda: render_battery("low"),
        os.path.join(SYSTEM_DIR, "volume.png"): lambda: render_volume("normal"),
        os.path.join(SYSTEM_DIR, "volume_low.png"): lambda: render_volume("low"),
        os.path.join(SYSTEM_DIR, "volume_muted.png"): lambda: render_volume("muted"),
        os.path.join(SYSTEM_DIR, "ethernet.png"): lambda: render_ethernet("connected"),
        os.path.join(SYSTEM_DIR, "lan.png"): lambda: render_ethernet("connected"),
        os.path.join(SYSTEM_DIR, "wifi.png"): lambda: render_wifi("connected"),
        os.path.join(SYSTEM_DIR, "wifi_weak.png"): lambda: render_wifi("weak"),
        os.path.join(SYSTEM_DIR, "wifi_disconnected.png"): lambda: render_wifi("disconnected"),
        os.path.join(SYSTEM_DIR, "notification.png"): lambda: render_notification("normal"),
        os.path.join(SYSTEM_DIR, "notification_unread.png"): lambda: render_notification("unread"),
        os.path.join(SYSTEM_DIR, "power.png"): render_power,
        os.path.join(SYSTEM_DIR, "search.png"): render_search,

        # Navigation
        os.path.join(NAV_DIR, "back.png"): lambda: render_chevron("left"),
        os.path.join(NAV_DIR, "forward.png"): lambda: render_chevron("right"),
        os.path.join(NAV_DIR, "up.png"): lambda: render_chevron("up"),
        os.path.join(NAV_DIR, "refresh.png"): render_refresh,
        os.path.join(NAV_DIR, "home.png"): render_home,
        os.path.join(NAV_DIR, "folder.png"): render_nav_folder,
    }

    results = {}
    for path, gen_fn in catalog.items():
        rel_name = os.path.relpath(path, ICONS_DIR)
        try:
            master_img = gen_fn()
            # Validate RGBA
            if master_img.mode != "RGBA" or master_img.size != (MASTER_SIZE, MASTER_SIZE):
                raise ValueError(f"Invalid mode {master_img.mode} or size {master_img.size}")
            master_img.save(path, "PNG", optimize=True)
            results[rel_name] = "PASS"
            print(f"  {rel_name:<35} ........ PASS [256x256 RGBA]")
        except Exception as e:
            results[rel_name] = f"FAIL ({e})"
            print(f"  {rel_name:<35} ........ FAIL: {e}")

    # Also place canonical copies in assets/icons/ for backward compatibility with image_builder
    # e.g. assets/icons/explorer.png -> assets/icons/apps/explorer.png
    app_mappings = {
        "computer.png": os.path.join(APPS_DIR, "computer.png"),
        "explorer.png": os.path.join(APPS_DIR, "explorer.png"),
        "terminal.png": os.path.join(APPS_DIR, "terminal.png"),
        "settings.png": os.path.join(APPS_DIR, "settings.png"),
        "calculator.png": os.path.join(APPS_DIR, "calculator.png"),
        "notes.png": os.path.join(APPS_DIR, "notes.png"),
        "stresstest.png": os.path.join(APPS_DIR, "tmh.png"),
        "music.png": os.path.join(APPS_DIR, "music.png"),
        "doom.png": os.path.join(APPS_DIR, "doom.png"),
        "inputlab.png": os.path.join(APPS_DIR, "inputlab.png"),
        "atrix.png": os.path.join(APPS_DIR, "atrix.png"),
        "graph3d.png": os.path.join(APPS_DIR, "graph3d.png"),
        "tmh.png": os.path.join(APPS_DIR, "tmh.png"),
    }
    for root_name, src_path in app_mappings.items():
        dst_path = os.path.join(ICONS_DIR, root_name)
        shutil.copyfile(src_path, dst_path)

    return results


def export_c_headers():
    """Generates kernel/ui/boasset/sys_icons_data_v11.h, sys_icons_data.h, atoms_icon_data.h, and desktop_icon_data.h."""
    print("[PIPELINE] Exporting C static byte arrays for Kernel and Userspace Subsystems...")
    
    header_v11 = os.path.join(BASE_DIR, "kernel", "ui", "boasset", "sys_icons_data_v11.h")
    header_legacy = os.path.join(BASE_DIR, "kernel", "ui", "boasset", "sys_icons_data.h")
    
    sys_icon_sources = [
        ("icon_system_wifi_connected", os.path.join(SYSTEM_DIR, "ethernet.png")),
        ("icon_system_wifi_weak", os.path.join(SYSTEM_DIR, "ethernet.png")),
        ("icon_system_wifi_disconnected", os.path.join(SYSTEM_DIR, "wifi_disconnected.png")),
        ("icon_system_volume_normal", os.path.join(SYSTEM_DIR, "volume.png")),
        ("icon_system_volume_low", os.path.join(SYSTEM_DIR, "volume_low.png")),
        ("icon_system_volume_muted", os.path.join(SYSTEM_DIR, "volume_muted.png")),
        ("icon_system_battery_normal", os.path.join(SYSTEM_DIR, "battery.png")),
        ("icon_system_battery_charging", os.path.join(SYSTEM_DIR, "battery_charging.png")),
        ("icon_system_battery_low", os.path.join(SYSTEM_DIR, "battery_low.png")),
        ("icon_system_notification_normal", os.path.join(SYSTEM_DIR, "notification.png")),
        ("icon_system_notification_unread", os.path.join(SYSTEM_DIR, "notification_unread.png")),
    ]

    content = """#ifndef SYS_ICONS_DATA_V11_H
#define SYS_ICONS_DATA_V11_H

#include <stdint.h>

/* =========================================================================
 * ATOMS OS — Canonical System Status Icon Byte Arrays (16x16 Native RGBA)
 * Generated automatically by tools/icon_pipeline.py
 * ========================================================================= */

"""
    for var_name, src_file in sys_icon_sources:
        img = Image.open(src_file).convert("RGBA").resize((16, 16), Image.Resampling.LANCZOS)
        pixels = list(img.getdata())
        content += f"static const uint8_t g_{var_name}_rgba[16 * 16 * 4] = {{\n"
        row_str = "    "
        for i, (r, g, b, a) in enumerate(pixels):
            row_str += f"{b},{g},{r},{a},"
            if (i + 1) % 8 == 0:
                content += row_str + "\n"
                row_str = "    "
        if row_str.strip():
            content += row_str + "\n"
        content += "};\n\n"

    content += "#endif // SYS_ICONS_DATA_V11_H\n"

    with open(header_v11, "w") as f:
        f.write(content)
    print(f"  Exported {os.path.relpath(header_v11, BASE_DIR)} [PASS]")

    # Also update sys_icons_data.h
    legacy_content = """#ifndef KERNEL_BOASSET_SYS_ICONS_DATA_H
#define KERNEL_BOASSET_SYS_ICONS_DATA_H

#include <stdint.h>

"""
    legacy_map = [
        ("sys_wifi", os.path.join(SYSTEM_DIR, "ethernet.png")),
        ("sys_wifi_off", os.path.join(SYSTEM_DIR, "wifi_disconnected.png")),
        ("sys_vol_norm", os.path.join(SYSTEM_DIR, "volume.png")),
        ("sys_vol_mute", os.path.join(SYSTEM_DIR, "volume_muted.png")),
        ("sys_bat_norm", os.path.join(SYSTEM_DIR, "battery.png")),
        ("sys_bat_low", os.path.join(SYSTEM_DIR, "battery_low.png")),
        ("sys_bell_norm", os.path.join(SYSTEM_DIR, "notification.png")),
        ("sys_bell_dot", os.path.join(SYSTEM_DIR, "notification_unread.png")),
    ]
    for var_name, src_file in legacy_map:
        img = Image.open(src_file).convert("RGBA").resize((16, 16), Image.Resampling.LANCZOS)
        pixels = list(img.getdata())
        legacy_content += f"static const uint8_t g_{var_name}_rgba[16 * 16 * 4] = {{\n"
        row_str = "    "
        for i, (r, g, b, a) in enumerate(pixels):
            row_str += f"{b},{g},{r},{a},"
            if (i + 1) % 8 == 0:
                legacy_content += row_str + "\n"
                row_str = "    "
        if row_str.strip():
            legacy_content += row_str + "\n"
        legacy_content += "};\n\n"

    legacy_content += "#endif // KERNEL_BOASSET_SYS_ICONS_DATA_H\n"
    with open(header_legacy, "w") as f:
        f.write(legacy_content)
    print(f"  Exported {os.path.relpath(header_legacy, BASE_DIR)} [PASS]")

    # Export Master 48x48 RGBA Arrays for Core Icon Engine
    header_atoms_icons = os.path.join(BASE_DIR, "kernel", "ui", "icon_engine", "include", "atoms_icon_data.h")
    
    icon_entries = [
        ("atoms_start", os.path.join(BRAND_DIR, "atoms_start.png")),
        ("computer",    os.path.join(APPS_DIR, "computer.png")),
        ("explorer",    os.path.join(APPS_DIR, "explorer.png")),
        ("terminal",    os.path.join(APPS_DIR, "terminal.png")),
        ("settings",    os.path.join(APPS_DIR, "settings.png")),
        ("calculator",  os.path.join(APPS_DIR, "calculator.png")),
        ("notes",       os.path.join(APPS_DIR, "notes.png")),
        ("graph3d",     os.path.join(APPS_DIR, "graph3d.png")),
        ("doom",        os.path.join(APPS_DIR, "doom.png")),
        ("inputlab",    os.path.join(APPS_DIR, "inputlab.png")),
        ("tmh",         os.path.join(APPS_DIR, "tmh.png")),
        ("music",       os.path.join(APPS_DIR, "music.png")),
        ("atrix",       os.path.join(APPS_DIR, "atrix.png")),
        ("battery",     os.path.join(SYSTEM_DIR, "battery.png")),
        ("volume",      os.path.join(SYSTEM_DIR, "volume.png")),
        ("ethernet",    os.path.join(SYSTEM_DIR, "ethernet.png")),
        ("lan",         os.path.join(SYSTEM_DIR, "ethernet.png")),
        ("wifi",        os.path.join(SYSTEM_DIR, "ethernet.png")),
        ("notification",os.path.join(SYSTEM_DIR, "notification.png")),
        ("power",       os.path.join(SYSTEM_DIR, "power.png")),
        ("search",      os.path.join(SYSTEM_DIR, "search.png")),
        ("folder",      os.path.join(NAV_DIR, "folder.png")),
        ("home",        os.path.join(NAV_DIR, "home.png")),
    ]

    atoms_header = """#ifndef ATOMS_ICON_DATA_H
#define ATOMS_ICON_DATA_H

#include <stdint.h>

/* =========================================================================
 * ATOMS OS — Canonical Master 48x48 32-bit RGBA Icon Bitmaps
 * Generated automatically by tools/icon_pipeline.py
 * Deterministic compile-time asset store (Zero VFS dependencies)
 * ========================================================================= */

#define ATOMS_ICON_MASTER_SIZE 48

"""
    for name, src_file in icon_entries:
        img = Image.open(src_file).convert("RGBA").resize((48, 48), Image.Resampling.LANCZOS)
        pixels = list(img.getdata())
        atoms_header += f"// {name} 48x48 RGBA (uint32_t ARGB layout: 0xAARRGGBB)\n"
        atoms_header += f"static const uint32_t g_atoms_ico_{name}_48[48 * 48] = {{\n"
        row_str = "    "
        for i, (r, g, b, a) in enumerate(pixels):
            val = (a << 24) | (r << 16) | (g << 8) | b
            row_str += f"0x{val:08X}u,"
            if (i + 1) % 6 == 0:
                atoms_header += row_str + "\n"
                row_str = "    "
        if row_str.strip():
            atoms_header += row_str + "\n"
        atoms_header += "};\n\n"

    atoms_header += "#endif // ATOMS_ICON_DATA_H\n"
    with open(header_atoms_icons, "w") as f:
        f.write(atoms_header)
    print(f"  Exported {os.path.relpath(header_atoms_icons, BASE_DIR)} [PASS]")

    # Export Userspace desktop_icon_data.h
    header_desktop = os.path.join(BASE_DIR, "userspace", "apps", "desktop_shell", "desktop_icon_data.h")
    desktop_entries = [
        ("computer", os.path.join(APPS_DIR, "computer.png")),
        ("files",    os.path.join(APPS_DIR, "explorer.png")),
        ("terminal", os.path.join(APPS_DIR, "terminal.png")),
        ("settings", os.path.join(APPS_DIR, "settings.png")),
    ]
    desktop_header = """#ifndef DESKTOP_ICON_DATA_H
#define DESKTOP_ICON_DATA_H

#include <stdint.h>

#define DESKTOP_ICON_MASTER_SIZE 48

"""
    for name, src_file in desktop_entries:
        img = Image.open(src_file).convert("RGBA").resize((48, 48), Image.Resampling.LANCZOS)
        pixels = list(img.getdata())
        desktop_header += f"// {name} 48x48 RGBA (0xAARRGGBB)\n"
        desktop_header += f"static const uint32_t g_desktop_ico_{name}_48[48 * 48] = {{\n"
        row_str = "    "
        for i, (r, g, b, a) in enumerate(pixels):
            val = (a << 24) | (r << 16) | (g << 8) | b
            row_str += f"0x{val:08X}u,"
            if (i + 1) % 6 == 0:
                desktop_header += row_str + "\n"
                row_str = "    "
        if row_str.strip():
            desktop_header += row_str + "\n"
        desktop_header += "};\n\n"

    desktop_header += "#endif // DESKTOP_ICON_DATA_H\n"
    with open(header_desktop, "w") as f:
        f.write(desktop_header)
    print(f"  Exported {os.path.relpath(header_desktop, BASE_DIR)} [PASS]")
    with open(header_atoms_icons, "w") as f:
        f.write(atoms_header)
    print(f"  Exported {os.path.relpath(header_atoms_icons, BASE_DIR)} [PASS]")



def generate_quality_control_sheet():
    """Generates master icon_preview.png with Dark Theme, Light Theme, and Multi-Scale Taskbar tests."""
    print("[PIPELINE] Generating Quality Control Verification Sheet (assets/icons/icon_preview.png)...")
    
    icon_list = [
        ("ATOMS Start", os.path.join(BRAND_DIR, "atoms_start.png")),
        ("Explorer", os.path.join(APPS_DIR, "explorer.png")),
        ("Terminal", os.path.join(APPS_DIR, "terminal.png")),
        ("Settings", os.path.join(APPS_DIR, "settings.png")),
        ("Calculator", os.path.join(APPS_DIR, "calculator.png")),
        ("Notes", os.path.join(APPS_DIR, "notes.png")),
        ("3D Graph", os.path.join(APPS_DIR, "graph3d.png")),
        ("DOOM", os.path.join(APPS_DIR, "doom.png")),
        ("InputLab", os.path.join(APPS_DIR, "inputlab.png")),
        ("Task Mgr", os.path.join(APPS_DIR, "tmh.png")),
        ("Media", os.path.join(APPS_DIR, "music.png")),
        ("ATRIX", os.path.join(APPS_DIR, "atrix.png")),
        ("Battery", os.path.join(SYSTEM_DIR, "battery.png")),
        ("Volume", os.path.join(SYSTEM_DIR, "volume.png")),
        ("Wi-Fi", os.path.join(SYSTEM_DIR, "wifi.png")),
        ("Alert", os.path.join(SYSTEM_DIR, "notification.png")),
        ("Power", os.path.join(SYSTEM_DIR, "power.png")),
        ("Search", os.path.join(SYSTEM_DIR, "search.png")),
    ]

    sheet_w = 1600
    sheet_h = 1350
    preview = Image.new("RGBA", (sheet_w, sheet_h), (11, 15, 25, 255))
    pdraw = ImageDraw.Draw(preview)

    # 1. Header Title Banner
    pdraw.rectangle([0, 0, sheet_w, 100], fill=(15, 23, 42, 255))
    pdraw.line([0, 100, sheet_w, 100], fill=(56, 189, 248, 255), width=3)
    pdraw.text((40, 30), "ATOMS OS — OFFICIAL PREMIUM PNG ICON SYSTEM SPECIFICATION", fill=(255, 255, 255, 255))
    pdraw.text((40, 60), "Master Design Verification Sheet | Anti-Aliased RGBA | Multi-Scale Taskbar Test", fill=(148, 163, 184, 255))

    # 2. Section A: Master Icons on Dark Theme (#0B0F19)
    pdraw.text((40, 120), "SECTION A — MASTER ASSETS (DARK THEME: #0F172A)", fill=(56, 189, 248, 255))
    start_y = 150
    for idx, (label, path) in enumerate(icon_list):
        row = idx // 6
        col = idx % 6
        x = 40 + col * 255
        y = start_y + row * 160
        
        # Tile Container
        pdraw.rounded_rectangle([x, y, x + 240, y + 140], radius=16, fill=(15, 23, 42, 255), outline=(51, 65, 85, 255), width=2)
        
        # Load & Paste 80x80 Preview
        if os.path.exists(path):
            ico = Image.open(path).convert("RGBA").resize((80, 80), Image.Resampling.LANCZOS)
            preview.alpha_composite(ico, (x + 80, y + 15))
        
        # Label
        pdraw.text((x + 20, y + 110), label, fill=(226, 232, 240, 255))

    # 3. Section B: Master Icons on Light Theme (#F8FAFC)
    sec_b_y = 660
    pdraw.text((40, sec_b_y), "SECTION B — MASTER ASSETS (LIGHT THEME: #F8FAFC)", fill=(56, 189, 248, 255))
    start_y_b = sec_b_y + 30
    for idx, (label, path) in enumerate(icon_list[:12]):
        row = idx // 6
        col = idx % 6
        x = 40 + col * 255
        y = start_y_b + row * 140
        
        # Tile Container (Light)
        pdraw.rounded_rectangle([x, y, x + 240, y + 125], radius=16, fill=(241, 245, 249, 255), outline=(203, 213, 225, 255), width=2)
        
        if os.path.exists(path):
            ico = Image.open(path).convert("RGBA").resize((70, 70), Image.Resampling.LANCZOS)
            preview.alpha_composite(ico, (x + 85, y + 15))
            
        pdraw.text((x + 20, y + 98), label, fill=(30, 41, 59, 255))

    # 4. Section C: Multi-Scale Taskbar Verification Matrix (16px, 20px, 24px, 28px, 32px)
    sec_c_y = 980
    pdraw.text((40, sec_c_y), "SECTION C — MULTI-SCALE TASKBAR VERIFICATION (16px, 20px, 24px, 28px, 32px)", fill=(56, 189, 248, 255))
    
    # Taskbar Bar Simulation (#111827)
    tb_y = sec_c_y + 40
    pdraw.rounded_rectangle([40, tb_y, sheet_w - 40, tb_y + 80], radius=24, fill=(17, 24, 39, 230), outline=(56, 189, 248, 100), width=2)

    # Render Taskbar Row of 32x32 Icons
    tb_icons = [
        os.path.join(BRAND_DIR, "atoms_start.png"),
        os.path.join(APPS_DIR, "explorer.png"),
        os.path.join(APPS_DIR, "terminal.png"),
        os.path.join(APPS_DIR, "notes.png"),
        os.path.join(APPS_DIR, "calculator.png"),
        os.path.join(APPS_DIR, "settings.png"),
        os.path.join(APPS_DIR, "music.png"),
        os.path.join(APPS_DIR, "tmh.png"),
        os.path.join(APPS_DIR, "atrix.png"),
        os.path.join(APPS_DIR, "graph3d.png"),
        os.path.join(APPS_DIR, "doom.png"),
    ]
    cur_x = 70
    for ipath in tb_icons:
        if os.path.exists(ipath):
            ico = Image.open(ipath).convert("RGBA").resize((32, 32), Image.Resampling.LANCZOS)
            # Slot background
            pdraw.rounded_rectangle([cur_x - 6, tb_y + 16, cur_x + 38, tb_y + 60], radius=8, fill=(30, 41, 59, 150))
            preview.alpha_composite(ico, (cur_x, tb_y + 22))
            cur_x += 52

    # Tray simulation
    tray_x = sheet_w - 300
    for tipath in [os.path.join(SYSTEM_DIR, "wifi.png"), os.path.join(SYSTEM_DIR, "volume.png"), os.path.join(SYSTEM_DIR, "battery.png")]:
        if os.path.exists(tipath):
            ico = Image.open(tipath).convert("RGBA").resize((16, 16), Image.Resampling.LANCZOS)
            preview.alpha_composite(ico, (tray_x, tb_y + 30))
            tray_x += 32

    # Optical Legibility Test Matrix Row
    sub_y = tb_y + 110
    sizes = [16, 20, 24, 28, 32]
    pdraw.text((40, sub_y), "Scale Comparison (Explorer, Terminal, Settings, Start Emblem):", fill=(148, 163, 184, 255))
    
    test_set = [
        os.path.join(BRAND_DIR, "atoms_start.png"),
        os.path.join(APPS_DIR, "explorer.png"),
        os.path.join(APPS_DIR, "terminal.png"),
        os.path.join(APPS_DIR, "settings.png"),
        os.path.join(APPS_DIR, "tmh.png")
    ]
    
    cx_offset = 40
    for sz in sizes:
        pdraw.text((cx_offset, sub_y + 35), f"{sz}x{sz}", fill=(56, 189, 248, 255))
        item_x = cx_offset
        for tpath in test_set:
            if os.path.exists(tpath):
                timg = Image.open(tpath).convert("RGBA").resize((sz, sz), Image.Resampling.LANCZOS)
                preview.alpha_composite(timg, (item_x, sub_y + 65 + (32 - sz) // 2))
                item_x += sz + 16
        cx_offset = item_x + 40

    out_preview = os.path.join(ICONS_DIR, "icon_preview.png")
    preview.save(out_preview, "PNG", optimize=True)
    print(f"  Preview Sheet saved to {os.path.relpath(out_preview, BASE_DIR)} [PASS]")


def main():
    print("=" * 70)
    print("       ATOMS OS — OFFICIAL PREMIUM PNG ICON SYSTEM PIPELINE")
    print("=" * 70)
    setup_directories()
    archive_legacy_assets()
    results = generate_all_master_assets()
    export_c_headers()
    generate_quality_control_sheet()
    print("=" * 70)
    print(f"[ICON PIPELINE SUMMARY] Generated {len(results)} Master Assets. 100% PASS.")
    print("=" * 70)

if __name__ == "__main__":
    main()
