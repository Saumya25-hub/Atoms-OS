import os

# 1. ac97_playback.c
playback_path = 'kernel/audio/drivers/ac97/ac97_playback.c'
with open(playback_path, 'r') as f:
    pb_data = f.read()

# Strip out the forensic buffer and definitions
start_idx = pb_data.find('#define FORENSIC_BUF_SIZE 64')
if start_idx != -1:
    end_idx = pb_data.find('static Ac97PlaybackTelemetry g_pb_telemetry = {0};', start_idx)
    if end_idx != -1:
        # Actually in my previous patch I put it after g_pb_telemetry.
        pass

# It's safer to just re-checkout ac97_playback.c to the commit BEFORE my forensic changes, and then re-apply LVI fix!
