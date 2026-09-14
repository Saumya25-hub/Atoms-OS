import os

path = 'kernel/audio/diagnostics/audio_debug.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('display_print("--- Audio HAL Status ---\n");', 'display_print("--- Audio HAL Status ---{\\n}");'.replace('{', '').replace('}', ''))
content = content.replace('display_print("Active Driver: "); display_print(drv->name); display_print("\n");', 'display_print("Active Driver: "); display_print(drv->name); display_print("{\\n}");'.replace('{', '').replace('}', ''))
content = content.replace('display_print("Capabilities: "); display_print_hex(drv->capabilities); display_print("\n");', 'display_print("Capabilities: "); display_print_hex(drv->capabilities); display_print("{\\n}");'.replace('{', '').replace('}', ''))
content = content.replace('display_print("Active Driver: [NONE DETECTED]\n");', 'display_print("Active Driver: [NONE DETECTED]{\\n}");'.replace('{', '').replace('}', ''))

# Actually, my previous python script literally injected literal newlines in the C strings.
# I need to clean it up.
import re
content = re.sub(r'display_print\("([^"]*)\n"\);', r'display_print("\1\\n");', content)
content = re.sub(r'display_print\("([^"]*)\n      \|', r'display_print("\1\\n");', content) # In case it got super weird

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
