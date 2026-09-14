import os
import re

replacements = {
    '\"kernel/audio/audio_mixer.h\"': '\"kernel/audio/mixer/audio_mixer.h\"',
    '\"kernel/audio/audio_player.h\"': '\"kernel/audio/session/audio_player.h\"',
    '\"../../display/display.h\"': '\"kernel/drivers/display/display.h\"',
    '\"../../../../arch/x86_64/io/port_io.h\"': '\"kernel/arch/x86_64/io/port_io.h\"'
}

for root, _, files in os.walk('kernel'):
    for file in files:
        if file.endswith('.c') or file.endswith('.h'):
            path = os.path.join(root, file)
            with open(path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            modified = False
            for old, new in replacements.items():
                if old in content:
                    content = content.replace(old, new)
                    modified = True
            
            if modified:
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(content)
                print(f'Updated {path}')
