import os

def fix_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    if 'g_kernel_screen_width' not in content:
        return False

    # Remove extern declarations
    import re
    content = re.sub(r'extern\s+uint32_t\s+g_kernel_screen_width;', '', content)
    content = re.sub(r'extern\s+uint32_t\s+g_kernel_screen_height;', '', content)
    
    # Ensure agdae.h is included
    if '#include "kernel/display/agdae/agdae.h"' not in content:
        idx = content.find('#include')
        if idx != -1:
            end_idx = content.find('\n', idx)
            content = content[:end_idx+1] + '#include "kernel/display/agdae/agdae.h"\n' + content[end_idx+1:]
        else:
            content = '#include "kernel/display/agdae/agdae.h"\n' + content

    content = content.replace('g_kernel_screen_width', '(uint32_t)AGDAE_GetMetrics()->desktop_rect.width')
    content = content.replace('g_kernel_screen_height', '(uint32_t)AGDAE_GetMetrics()->desktop_rect.height')

    with open(filepath, 'w') as f:
        f.write(content)
    
    return True

files = [
    'd:/Signatures_OS/kernel/wm/surface/surface.c',
    'd:/Signatures_OS/kernel/wm/compositor/compositor_damage.c',
    'd:/Signatures_OS/kernel/wm/bwe/src/bwe_window.c',
    'd:/Signatures_OS/kernel/wm/bwe/renderer/bwe_compositor.c',
    'd:/Signatures_OS/kernel/ui/start_menu.c',
    'd:/Signatures_OS/kernel/media/bopawn/wallpaper/wallpaper_manager.c',
    'd:/Signatures_OS/kernel/shell/desktop_shell/desktop_shell.c'
]

for f in files:
    if os.path.exists(f):
        if fix_file(f):
            print(f"Fixed {f}")
