import os

# 1. audio_player.c
player_path = 'kernel/audio/session/audio_player.c'
with open(player_path, 'r') as f:
    player_data = f.read()

# Add audio_core.h at the top
player_data = player_data.replace('#include "kernel/audio/session/audio_player.h"', '#include "kernel/audio/session/audio_player.h"\n#include "kernel/audio/core/audio_core.h"')

fmt_check = '''    if (fmt.audio_format != 1 || fmt.num_channels != 2 || fmt.sample_rate != 48000 || fmt.bits_per_sample != 16) {'''
fmt_log = '''    
    display_print("\\nAUDIO_FORMAT:\\n");
    display_print("channels="); display_print_dec(fmt.num_channels); display_print("\\n");
    display_print("sample_rate="); display_print_dec(fmt.sample_rate); display_print("\\n");
    display_print("bits_per_sample="); display_print_dec(fmt.bits_per_sample); display_print("\\n");
    display_print("block_align="); display_print_dec(fmt.block_align); display_print("\\n");
    display_print("data_offset="); display_print_dec(data_offset); display_print("\\n");
    display_print("data_size="); display_print_dec(data_size); display_print("\\n");
    display_print("data_size % block_align="); display_print_dec(data_size % fmt.block_align); display_print("\\n");
'''
player_data = player_data.replace(fmt_check, fmt_log + '\n' + fmt_check)

loop_check = '''    if (g_audio_session.bytes_played >= g_audio_session.data_size) {
        extern int vfs_seek(int fd, uint64_t offset, int whence);
        vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
        g_audio_session.bytes_played = 0;
    }'''
loop_log = '''    if (g_audio_session.bytes_played >= g_audio_session.data_size) {
        extern int vfs_seek(int fd, uint64_t offset, int whence);
        struct AudioStream* s = audio_core_get_stream(g_audio_session.stream_id);
        display_print("\\n[AUDIO LOOP BOUNDARY]\\n");
        display_print("bytes_played="); display_print_dec(g_audio_session.bytes_played); display_print("\\n");
        display_print("data_size="); display_print_dec(g_audio_session.data_size); display_print("\\n");
        if (s && s->ring_buffer) {
            display_print("ring_head="); display_print_dec(s->ring_buffer->head); display_print("\\n");
            display_print("ring_tail="); display_print_dec(s->ring_buffer->tail); display_print("\\n");
            extern size_t audio_buffer_available(void*);
            display_print("ring_available="); display_print_dec(audio_buffer_available(s->ring_buffer)); display_print("\\n");
        }
        vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
        g_audio_session.bytes_played = 0;
    }'''
player_data = player_data.replace(loop_check, loop_log)

vfs_read_call = '''            int read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);'''
align_check = '''            int read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);
            if (read_bytes > 0 && (read_bytes % 4) != 0) {
                display_print("\\n[AUDIO ALIGNMENT VIOLATION] stage: VFS read\\n");
                display_print("requested:"); display_print_dec(to_read); display_print("\\n");
                display_print("actual:"); display_print_dec(read_bytes); display_print("\\n");
            }
'''
player_data = player_data.replace(vfs_read_call, align_check)

with open(player_path, 'w') as f:
    f.write(player_data)

# 2. audio_api.c (audio_stream_write)
api_path = 'kernel/audio/api/audio_api.c'
with open(api_path, 'r') as f:
    api_data = f.read()

# Add display.h at the top
api_data = api_data.replace('#include "kernel/audio/api/audio_api.h"', '#include "kernel/audio/api/audio_api.h"\n#include "kernel/drivers/display/display.h"')

stream_write_check = '''size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet) {'''
stream_write_log = '''size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet) {
    if (packet && packet->size_bytes % 4 != 0) {
        display_print("\\n[AUDIO ALIGNMENT VIOLATION] stage: audio_stream_write request\\n");
        display_print("actual:"); display_print_dec(packet->size_bytes); display_print("\\n");
    }
'''
api_data = api_data.replace(stream_write_check, stream_write_log)

with open(api_path, 'w') as f:
    f.write(api_data)

# 3. ac97_playback.c
playback_path = 'kernel/audio/drivers/ac97/ac97_playback.c'
with open(playback_path, 'r') as f:
    playback_data = f.read()

# Add display.h if not present
if '#include "kernel/drivers/display/display.h"' not in playback_data:
    playback_data = playback_data.replace('#include "kernel/audio/drivers/ac97/ac97_playback.h"', '#include "kernel/audio/drivers/ac97/ac97_playback.h"\n#include "kernel/drivers/display/display.h"')

wrap_log = '''    uint8_t civ = io_in8(g_nabm_base + AC97_NABM_PO_CIV);
    
    static uint8_t s_prev_civ = 0;
    if (s_prev_civ == 31 && civ == 0) {
        display_print("\\n[CIV WRAP 31->0]\\n");
    }
    s_prev_civ = civ;
'''
playback_data = playback_data.replace('uint8_t civ = io_in8(g_nabm_base + AC97_NABM_PO_CIV);', wrap_log)

with open(playback_path, 'w') as f:
    f.write(playback_data)

print("Patch applied.")
