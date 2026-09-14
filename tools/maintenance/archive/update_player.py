import os

path = 'kernel/audio/session/audio_player.c'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('#include "kernel/audio/drivers/ac97/ac97_playback.h"', '#include "kernel/audio/hal/audio_hal.h"')
content = content.replace('ac97_playback_init()', 'audio_hal_get_active_driver() != NULL')
content = content.replace('ac97_playback_prepare()', '// HAL handles preparation during start')
content = content.replace('ac97_playback_start()', 'audio_hal_start_stream(44100, 2, 16)')
content = content.replace('ac97_playback_stop()', 'audio_hal_stop_stream()')
content = content.replace('ac97_playback_shutdown()', 'audio_hal_shutdown()')
content = content.replace('ac97_playback_update()', 'audio_hal_update_pointers(0)')

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
