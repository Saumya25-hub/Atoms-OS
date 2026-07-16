import os

replacements = {
    '\"audio_api.h\"': '\"kernel/audio/api/audio_api.h\"',
    '\"audio_core.h\"': '\"kernel/audio/core/audio_core.h\"',
    '\"audio_buffer.h\"': '\"kernel/audio/streams/audio_buffer.h\"',
    '\"audio_stream.h\"': '\"kernel/audio/streams/audio_stream.h\"',
    '\"audio_mixer.h\"': '\"kernel/audio/mixer/audio_mixer.h\"',
    '\"audio_mix_math.h\"': '\"kernel/audio/mixer/audio_mix_math.h\"',
    '\"audio_volume.h\"': '\"kernel/audio/volume/audio_volume.h\"',
    '\"audio_player.h\"': '\"kernel/audio/session/audio_player.h\"',
    '\"audio_pcm.h\"': '\"kernel/audio/formats/audio_pcm.h\"',
    '\"audio_debug.h\"': '\"kernel/audio/diagnostics/audio_debug.h\"',
    '\"audio_forensic.h\"': '\"kernel/audio/forensic/audio_forensic.h\"',
    '\"../drivers/audio/ac97/ac97_playback.h\"': '\"kernel/audio/drivers/ac97/ac97_playback.h\"',
    '\"ac97.h\"': '\"kernel/audio/drivers/ac97/ac97.h\"',
    '\"ac97_codec.h\"': '\"kernel/audio/drivers/ac97/ac97_codec.h\"',
    '\"ac97_dma.h\"': '\"kernel/audio/drivers/ac97/ac97_dma.h\"',
    '\"ac97_playback.h\"': '\"kernel/audio/drivers/ac97/ac97_playback.h\"',
    '\"ac97_registers.h\"': '\"kernel/audio/drivers/ac97/ac97_registers.h\"',
    '\"ac97_bdl.h\"': '\"kernel/audio/drivers/ac97/ac97_bdl.h\"',
    '\"../audio/audio_core.h\"': '\"kernel/audio/core/audio_core.h\"'
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
