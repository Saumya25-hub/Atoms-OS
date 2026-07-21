import re
with open('kernel/audio/session/audio_player.c', 'r') as f:
    data = f.read()

# Fix the duplicate extern void error
data = data.replace('extern void             extern uint64_t timer_get_ticks(void);', 'extern uint64_t timer_get_ticks(void);')

with open('kernel/audio/session/audio_player.c', 'w') as f:
    f.write(data)
