import os

replacements = {
    '\"kernel/audio/audio_api.h\"': '\"kernel/audio/api/audio_api.h\"',
    '\"kernel/audio/audio_mixer.h\"': '\"kernel/audio/mixer/audio_mixer.h\"',
    '\"kernel/audio/audio_player.h\"': '\"kernel/audio/session/audio_player.h\"',
    '\"kernel/drivers/audio/ac97/ac97.h\"': '\"kernel/audio/drivers/ac97/ac97.h\"',
    '\"kernel/drivers/audio/ac97/ac97_playback.h\"': '\"kernel/audio/drivers/ac97/ac97_playback.h\"'
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
