import re
with open('kernel/audio/session/audio_player.c', 'r') as f:
    data = f.read()

data = re.sub(r'^\s*extern uint64_t timer_get_ticks\(void\);\n?', '', data, flags=re.MULTILINE)
data = re.sub(r'^\s*extern int vfs_seek\([^\)]+\);\n?', '', data, flags=re.MULTILINE)
data = re.sub(r'^\s*extern void audio_forensic_reset\(void\);\n?', '', data, flags=re.MULTILINE)
data = re.sub(r'^\s*extern void audio_realtime_worker_start\(void\);\n?', '', data, flags=re.MULTILINE)
data = re.sub(r'^\s*extern void audio_realtime_worker_stop\(void\);\n?', '', data, flags=re.MULTILINE)

# Also fix the weird alignment prints:
data = re.sub(r'display_print\("actual:"\); display_print_dec\(read_bytes\); display_print\("\\n"\);\s*\}', '}', data, flags=re.DOTALL)

with open('kernel/audio/session/audio_player.c', 'w') as f:
    f.write(data)
