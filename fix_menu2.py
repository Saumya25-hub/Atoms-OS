import os
import re

path = 'kernel/kernel.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the previous injection
content = re.sub(r'\s*// ==========================================================\s*// ROOT CAUSE INVESTIGATION: DIRECT DIAGNOSTIC MODE\s*// ==========================================================.*?(?=// Mount root filesystem)', '', content, flags=re.DOTALL)
content = re.sub(r'audio_diagnostic_mode\(\); // NEVER RETURNS\s*// ==========================================================\s*', '', content, flags=re.DOTALL)

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
    content = content.replace('vfs_mount_legacy("/", 1, "fat32");\n      display_print("Root FS mounted on BD 1\\n");\n    } else {\n      display_print("WARN: No partition found for Root FS.\\n");\n    }', 'vfs_mount_legacy("/", 1, "fat32");\n      display_print("Root FS mounted on BD 1\\n");\n    } else {\n      display_print("WARN: No partition found for Root FS.\\n");\n    }\n' + diagnostic_injection)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
