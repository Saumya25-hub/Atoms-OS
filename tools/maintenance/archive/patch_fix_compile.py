import os

player_path = 'kernel/audio/session/audio_player.c'
with open(player_path, 'r') as f:
    ap_data = f.read()

ap_data = ap_data.replace('g_audio_session.format.block_align', 'audio_pcm_bytes_per_frame(&g_audio_session.format)')

with open(player_path, 'w') as f:
    f.write(ap_data)

print("Fix applied.")
