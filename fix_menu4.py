import os
import re

path = 'kernel/kernel.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

diagnostic_injection = """
  // ==========================================================
  // ROOT CAUSE INVESTIGATION: DIRECT DIAGNOSTIC MODE
  // ==========================================================
  extern void audio_init(void);
  audio_init();
  extern void audio_mixer_init(void);
  audio_mixer_init();
  extern void audio_hal_init(void);
  audio_hal_init();
  
  extern void audio_diagnostic_mode(void);
  audio_diagnostic_mode(); // NEVER RETURNS
  // ==========================================================
"""

content = content.replace('display_print("[VFS] WARNING: No block devices found!\\n");\n  }', 'display_print("[VFS] WARNING: No block devices found!\\n");\n  }\n\n' + diagnostic_injection)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
