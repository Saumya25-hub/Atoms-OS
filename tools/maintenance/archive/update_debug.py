import os

path = 'kernel/audio/diagnostics/audio_debug.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

header = '#include "kernel/audio/hal/audio_hal.h"\n'
if header not in content:
    content = content.replace('#include "kernel/audio/diagnostics/audio_debug.h"', '#include "kernel/audio/diagnostics/audio_debug.h"\n' + header)

print_stats_hook = '''
void audio_debug_print_stats(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    display_print("--- Audio HAL Status ---\n");
    if (drv) {
        display_print("Active Driver: "); display_print(drv->name); display_print("\n");
        display_print("Capabilities: "); display_print_hex(drv->capabilities); display_print("\n");
    } else {
        display_print("Active Driver: [NONE DETECTED]\n");
    }
'''
if '--- Audio HAL Status ---' not in content:
    content = content.replace('void audio_debug_print_stats(void) {', print_stats_hook)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
