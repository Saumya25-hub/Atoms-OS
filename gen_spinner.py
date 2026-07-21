import math

frames = 8
size = 24
cx, cy = 11.5, 11.5
inner_r = 5.0
outer_r = 9.0
thickness_inner = 1.2
thickness_outer = 2.2 # black border

def dist_point_to_segment(px, py, x1, y1, x2, y2):
    vx, vy = x2 - x1, y2 - y1
    wx, wy = px - x1, py - y1
    
    c1 = wx * vx + wy * vy
    if c1 <= 0:
        return math.hypot(px - x1, py - y1)
        
    c2 = vx * vx + vy * vy
    if c2 <= c1:
        return math.hypot(px - x2, py - y2)
        
    b = c1 / c2
    pb_x = x1 + b * vx
    pb_y = y1 + b * vy
    return math.hypot(px - pb_x, py - pb_y)

with open('spinner_c.txt', 'w', encoding='utf-8') as f:
    f.write('    /* Busy Spinner (8 animated frames) */\n')
    f.write('    CursorThemeSprite* sb = &g_theme_sprites[CURSOR_SHAPE_BUSY];\n')
    f.write('    sb->width = 24;\n')
    f.write('    sb->height = 24;\n')
    f.write('    sb->hotspot_x = 12;\n')
    f.write('    sb->hotspot_y = 12;\n')
    f.write('    sb->frame_count = 8;\n')
    f.write('    sb->frame_interval_ms = 100;\n')
    f.write('    sb->is_animated = true;\n\n')
    
    f.write('    const char* spinner_frames[8][24] = {\n')
    
    for frame in range(frames):
        f.write('        {\n')
        for y in range(size):
            row = []
            for x in range(size):
                # distance to center
                dist_center = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if dist_center < 3.0:
                    row.append(' ')
                    continue
                
                c = ' '
                for i in range(frames):
                    angle = i * (360.0 / frames) - 90.0
                    rad = math.radians(angle)
                    
                    x1 = cx + inner_r * math.cos(rad)
                    y1 = cy + inner_r * math.sin(rad)
                    x2 = cx + outer_r * math.cos(rad)
                    y2 = cy + outer_r * math.sin(rad)
                    
                    d = dist_point_to_segment(x + 0.5, y + 0.5, x1, y1, x2, y2)
                    
                    if d <= thickness_outer:
                        if d <= thickness_inner:
                            # Inner color
                            if i == frame:
                                c = 'D' # Dark (active)
                            else:
                                if c != 'D':
                                    c = 'W' # Light
                        else:
                            # Shadow / Border
                            if c == ' ':
                                c = 'B' # Black
                row.append(c)
            f.write('            \"' + ''.join(row) + '\",\n')
        f.write('        },\n')
    f.write('    };\n\n')
    
    f.write('    for (uint32_t f = 0; f < 8; f++) {\n')
    f.write('        uint32_t* bmp = sb->bitmaps[f];\n')
    f.write('        for (uint32_t i = 0; i < 24 * 24; i++) bmp[i] = 0;\n')
    f.write('        for (uint32_t y = 0; y < 24; y++) {\n')
    f.write('            for (uint32_t x = 0; x < 24; x++) {\n')
    f.write('                char c = spinner_frames[f][y][x];\n')
    f.write('                uint32_t color = 0;\n')
    f.write('                if (c == \'B\') color = 0xFF000000;\n')
    f.write('                else if (c == \'W\') color = 0xFFCCCCCC; // Light grey\n')
    f.write('                else if (c == \'D\') color = 0xFF404040; // Dark grey\n')
    f.write('                if (color != 0) set_pixel(bmp, 24, x, y, color);\n')
    f.write('            }\n')
    f.write('        }\n')
    f.write('    }\n')
