import urllib.request
import re

url = "https://raw.githubusercontent.com/dhepper/font8x8/master/font8x8_basic.h"
try:
    with urllib.request.urlopen(url) as response:
        content = response.read().decode('utf-8')
    
    # format: { 0x00, 0x00, 0x00, ... }
    lines = content.split('\n')
    font_data = {}
    
    current_char = 0
    for line in lines:
        hex_strs = re.findall(r'0x[0-9A-Fa-f]{2}', line)
        if len(hex_strs) == 8:
            font_data[current_char] = [int(h, 16) for h in hex_strs]
            current_char += 1
    
    with open("d:/Signatures_OS/bovisual/Text/font8x16.h", "w") as f:
        f.write("#ifndef BOVISUAL_FONT8X16_H\n#define BOVISUAL_FONT8X16_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("static const uint8_t g_font8x16_stub[256][16] = {\n")
        for i in range(256):
            if i in font_data:
                f.write("    { ")
                # Double the height (each 8-bit row becomes two rows)
                for byte_val in font_data[i]:
                    f.write(f"0x{byte_val:02X}, 0x{byte_val:02X}, ")
                f.write("},\n")
            else:
                f.write("    { 0 },\n")
        f.write("};\n\n")
        f.write("#endif // BOVISUAL_FONT8X16_H\n")
    print("Successfully generated font8x16.h")
except Exception as e:
    print(f"Error: {e}")
