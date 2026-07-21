import os

with open('kernel/audio/session/audio_player.c', 'r') as f:
    data = f.read()

# Fix the broken print and remove misplaced externs
data = data.replace('''display_print("\
extern uint64_t timer_get_ticks(void);
extern int vfs_seek(int fd, uint64_t offset, int whence);
extern void audio_forensic_reset(void);
extern void audio_realtime_worker_start(void);
extern void audio_realtime_worker_stop(void);
n");''', 'display_print("\\n");')

externs = '''
extern uint64_t timer_get_ticks(void);
extern int vfs_seek(int fd, uint64_t offset, int whence);
extern void audio_forensic_reset(void);
extern void audio_realtime_worker_start(void);
extern void audio_realtime_worker_stop(void);
'''

# insert properly right after the last include
data = data.replace('#include "kernel/core/lib/include/string.h"', '#include "kernel/core/lib/include/string.h"\n' + externs)

with open('kernel/audio/session/audio_player.c', 'w') as f:
    f.write(data)
