import os

# Fix ac97_playback.c
playback_path = 'kernel/audio/drivers/ac97/ac97_playback.c'
with open(playback_path, 'r') as f:
    pb_data = f.read()
if '#include "kernel/audio/core/audio_core.h"' not in pb_data:
    pb_data = pb_data.replace('#include "kernel/audio/drivers/ac97/ac97_playback.h"', '#include "kernel/audio/drivers/ac97/ac97_playback.h"\n#include "kernel/audio/core/audio_core.h"')
with open(playback_path, 'w') as f:
    f.write(pb_data)

# Fix audio_player.c
player_path = 'kernel/audio/session/audio_player.c'
with open(player_path, 'r') as f:
    ap_data = f.read()

# Add extern uint64_t timer_get_ticks(void); to the top of the file if not present
if 'extern uint64_t timer_get_ticks(void);' not in ap_data[:500]:
    ap_data = ap_data.replace('#pragma pack(push, 1)', 'extern uint64_t timer_get_ticks(void);\n#pragma pack(push, 1)')

with open(player_path, 'w') as f:
    f.write(ap_data)

print("Fixed.")
