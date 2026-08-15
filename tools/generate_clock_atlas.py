import os
from PIL import Image, ImageDraw, ImageFont

def main():
    font_clock = ImageFont.truetype(r"C:\Windows\Fonts\arialn.ttf", 200)
    font_date = ImageFont.truetype(r"C:\Windows\Fonts\arialbd.ttf", 24)

    # 1. Clock Digits (76x160)
    CLOCK_W = 76
    CLOCK_H = 160
    COLON_W = 28

    clock_glyphs = {}
    for ch in '0123456789':
        dummy = Image.new('L', (300, 300), 0)
        d = ImageDraw.Draw(dummy)
        bb = d.textbbox((0, 0), ch, font=font_clock)
        w = bb[2] - bb[0]
        h = bb[3] - bb[1]
        
        img = Image.new('L', (CLOCK_W, CLOCK_H), 0)
        d2 = ImageDraw.Draw(img)
        tx = (CLOCK_W - w) // 2 - bb[0]
        ty = (CLOCK_H - h) // 2 - bb[1]
        d2.text((tx, ty), ch, fill=255, font=font_clock)
        clock_glyphs[ch] = img

    # Clock Colon
    img_colon = Image.new('L', (COLON_W, CLOCK_H), 0)
    d_c = ImageDraw.Draw(img_colon)
    d_c.ellipse([5, 40, 23, 58], fill=255)
    d_c.ellipse([5, 102, 23, 120], fill=255)
    clock_glyphs[':'] = img_colon

    # 2. Date Font (ASCII 32..126, Cell 20x26)
    DATE_W = 20
    DATE_H = 26
    date_glyphs = []
    date_widths = []

    for ascii_code in range(32, 127):
        ch = chr(ascii_code)
        dummy = Image.new('L', (100, 100), 0)
        d = ImageDraw.Draw(dummy)
        bb = d.textbbox((0, 0), ch, font=font_date)
        w = bb[2] - bb[0]
        h = bb[3] - bb[1]
        
        advance_w = max(w + 2, 8) if ch != ' ' else 8
        if advance_w > DATE_W:
            advance_w = DATE_W
        date_widths.append(advance_w)

        img = Image.new('L', (DATE_W, DATE_H), 0)
        d2 = ImageDraw.Draw(img)
        ty = 4 - bb[1] if bb[1] < 4 else 0
        tx = (advance_w - w) // 2 - bb[0] if w > 0 else 0
        if ch != ' ':
            d2.text((tx, ty), ch, fill=255, font=font_date)
        date_glyphs.append(img)

    # 3. 1:1 Native Resolution Icons (24x24 Lanczos Area-Averaged)
    ICON_SIZE = 24
    lock_512 = Image.open('BOOT(OS-ICO)/lock.png').convert('RGBA')
    eth_512 = Image.open('BOOT(OS-ICO)/ethernet-port.png').convert('RGBA')
    chat_512 = Image.open('BOOT(OS-ICO)/chat.png').convert('RGBA')

    lock_24 = lock_512.resize((ICON_SIZE, ICON_SIZE), Image.Resampling.LANCZOS)
    eth_24 = eth_512.resize((ICON_SIZE, ICON_SIZE), Image.Resampling.LANCZOS)
    chat_24 = chat_512.resize((ICON_SIZE, ICON_SIZE), Image.Resampling.LANCZOS)

    out_h = r"D:\Signatures_OS\kernel\shell\rook\pages\clock_atlas.h"
    out_c = r"D:\Signatures_OS\kernel\shell\rook\pages\clock_atlas.c"

    with open(out_h, 'w', encoding='utf-8') as f:
        f.write('#ifndef CLOCK_ATLAS_H\n')
        f.write('#define CLOCK_ATLAS_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'#define CLOCK_DIGIT_W {CLOCK_W}\n')
        f.write(f'#define CLOCK_DIGIT_H {CLOCK_H}\n')
        f.write(f'#define CLOCK_COLON_W {COLON_W}\n\n')
        f.write(f'#define DATE_FONT_W {DATE_W}\n')
        f.write(f'#define DATE_FONT_H {DATE_H}\n\n')
        f.write(f'#define NATIVE_ICON_SIZE {ICON_SIZE}\n\n')
        f.write('extern const uint8_t g_clock_digit_atlas[10][CLOCK_DIGIT_H * CLOCK_DIGIT_W];\n')
        f.write('extern const uint8_t g_clock_colon_atlas[CLOCK_DIGIT_H * CLOCK_COLON_W];\n\n')
        f.write('extern const uint8_t g_date_font_atlas[95][DATE_FONT_H * DATE_FONT_W];\n')
        f.write('extern const uint8_t g_date_font_widths[95];\n\n')
        f.write('extern const uint8_t g_lock_icon_atlas[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE];\n')
        f.write('extern const uint8_t g_ethernet_icon_atlas[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE];\n')
        f.write('extern const uint8_t g_chat_icon_atlas[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE];\n\n')
        f.write('#endif // CLOCK_ATLAS_H\n')

    with open(out_c, 'w', encoding='utf-8') as f:
        f.write('#include "clock_atlas.h"\n\n')
        
        # Clock Digits
        f.write(f'const uint8_t g_clock_digit_atlas[10][CLOCK_DIGIT_H * CLOCK_DIGIT_W] = {{\n')
        for d in range(10):
            ch = str(d)
            g = clock_glyphs[ch]
            data = list(g.getdata())
            f.write(f'  /* Digit {d} */\n  {{\n')
            for i, val in enumerate(data):
                f.write(f'0x{val:02X}, ')
                if (i + 1) % 16 == 0:
                    f.write('\n')
            f.write('\n  },\n')
        f.write('};\n\n')
        
        # Clock Colon
        g_col = clock_glyphs[':']
        data_col = list(g_col.getdata())
        f.write(f'const uint8_t g_clock_colon_atlas[CLOCK_DIGIT_H * CLOCK_COLON_W] = {{\n')
        for i, val in enumerate(data_col):
            f.write(f'0x{val:02X}, ')
            if (i + 1) % 16 == 0:
                f.write('\n')
        f.write('\n};\n\n')

        # Date Font Widths
        f.write('const uint8_t g_date_font_widths[95] = {\n  ')
        for i, w_val in enumerate(date_widths):
            f.write(f'{w_val}, ')
            if (i + 1) % 16 == 0:
                f.write('\n  ')
        f.write('\n};\n\n')

        # Date Font Atlas
        f.write(f'const uint8_t g_date_font_atlas[95][DATE_FONT_H * DATE_FONT_W] = {{\n')
        for idx, g in enumerate(date_glyphs):
            ch = chr(32 + idx)
            data = list(g.getdata())
            ch_desc = ch if ch != '\\' and ch != "'" else hex(ord(ch))
            f.write(f'  /* \'{ch_desc}\' (ASCII {32+idx}) */\n  {{\n')
            for i, val in enumerate(data):
                f.write(f'0x{val:02X}, ')
                if (i + 1) % 16 == 0:
                    f.write('\n')
            f.write('\n  },\n')
        f.write('};\n\n')

        # Native Icons
        for icon_name, img_obj in [('g_lock_icon_atlas', lock_24), ('g_ethernet_icon_atlas', eth_24), ('g_chat_icon_atlas', chat_24)]:
            f.write(f'const uint8_t {icon_name}[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE] = {{\n')
            for y in range(ICON_SIZE):
                for x in range(ICON_SIZE):
                    alpha_val = img_obj.getpixel((x, y))[3]
                    f.write(f'0x{alpha_val:02X}, ')
                f.write('\n')
            f.write('};\n\n')

    print("[SUCCESS] Generated unified Clock, Date & 1:1 Native Icon Atlas successfully!")

if __name__ == "__main__":
    main()
