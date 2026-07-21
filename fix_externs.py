import os

with open('kernel/audio/session/audio_player.c', 'r') as f:
    data = f.read()

externs = '''
extern uint64_t timer_get_ticks(void);
extern int vfs_seek(int fd, uint64_t offset, int whence);
extern void audio_forensic_reset(void);
extern void audio_realtime_worker_start(void);
extern void audio_realtime_worker_stop(void);
'''

# insert after includes
include_end = data.rfind('#include')
newline = data.find('\\n', include_end) + 1

data = data[:newline] + externs + data[newline:]

with open('kernel/audio/session/audio_player.c', 'w') as f:
    f.write(data)
