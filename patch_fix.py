import os

player_path = 'kernel/audio/session/audio_player.c'
with open(player_path, 'r') as f:
    player_data = f.read()

player_data = player_data.replace('extern size_t audio_buffer_available(void*);', '')

with open(player_path, 'w') as f:
    f.write(player_data)

print("Fixed.")
