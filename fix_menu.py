import os
import re

path = 'kernel/kernel.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the previously injected menu
menu_regex = re.compile(r'// ===.*?ROOT CAUSE INVESTIGATION BOOT MENU.*?// ===\n', re.DOTALL)
content = menu_regex.sub('', content)

# Remove the exact boot menu block I injected
content = re.sub(r'\s*// ==========================================================\s*// ROOT CAUSE INVESTIGATION BOOT MENU\s*// ==========================================================.*?(?=extern void BOF_BeginAtomicFrame)', '', content, flags=re.DOTALL)

# Now, find display_print("VFS OK\n"); and inject the hardcoded diagnostic jump
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

if "DIRECT DIAGNOSTIC MODE" not in content:
    content = content.replace('display_print("VFS OK\\n");', 'display_print("VFS OK\\n");\n' + diagnostic_injection)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
