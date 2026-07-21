import os

# 1. Fix Ring Producer/Consumer Memory Barriers
buffer_path = 'kernel/audio/streams/audio_buffer.c'
with open(buffer_path, 'r') as f:
    buf_data = f.read()

write_target = '''    buffer->head = (buffer->head + size) % buffer->capacity;
    return size;'''
write_fix = '''    __asm__ volatile("" ::: "memory");
    buffer->head = (buffer->head + size) % buffer->capacity;
    return size;'''
buf_data = buf_data.replace(write_target, write_fix)

read_target = '''    buffer->tail = (buffer->tail + size) % buffer->capacity;
    return size;'''
read_fix = '''    __asm__ volatile("" ::: "memory");
    buffer->tail = (buffer->tail + size) % buffer->capacity;
    return size;'''
buf_data = buf_data.replace(read_target, read_fix)

with open(buffer_path, 'w') as f:
    f.write(buf_data)

# 2. Fix AC97 LVI publication ordering barrier
ac97_path = 'kernel/audio/drivers/ac97/ac97_playback.c'
with open(ac97_path, 'r') as f:
    ac97_data = f.read()

lvi_target = '''    g_last_civ = civ;
    
    g_lvi = (civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
    io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);'''
lvi_fix = '''    g_last_civ = civ;
    
    __asm__ volatile("" ::: "memory"); // ENFORCE LVI ORDERING
    g_lvi = (civ + AC97_BDL_ENTRIES - 1) % AC97_BDL_ENTRIES;
    io_out8(g_nabm_base + AC97_NABM_PO_LVI, g_lvi);'''
ac97_data = ac97_data.replace(lvi_target, lvi_fix)

with open(ac97_path, 'w') as f:
    f.write(ac97_data)

# 3. Fix WAV block_align hardcoded % 4
player_path = 'kernel/audio/session/audio_player.c'
with open(player_path, 'r') as f:
    ap_data = f.read()

align_target = '''            if (read_bytes > 0 && (read_bytes % 4) != 0) {'''
align_fix = '''            if (read_bytes > 0 && g_audio_session.format.block_align > 0 && (read_bytes % g_audio_session.format.block_align) != 0) {'''
ap_data = ap_data.replace(align_target, align_fix)

align_target2 = '''            packet.frame_count = read_bytes / 4;'''
align_fix2 = '''            packet.frame_count = read_bytes / g_audio_session.format.block_align;'''
ap_data = ap_data.replace(align_target2, align_fix2)

align_target3 = '''        while (audio_stream_capacity(g_audio_session.stream_id) - audio_stream_available(g_audio_session.stream_id) >= 16384 + 4 &&'''
align_fix3 = '''        while (audio_stream_capacity(g_audio_session.stream_id) - audio_stream_available(g_audio_session.stream_id) >= 16384 + g_audio_session.format.block_align &&'''
ap_data = ap_data.replace(align_target3, align_fix3)

align_target4 = '''    if ((occupancy_pct < PRODUCER_HIGH_WATERMARK_PCT || g_audio_session.state == PLAYER_STATE_OPENED) && free_bytes >= PRODUCER_CHUNK_SIZE + 4) {'''
align_fix4 = '''    if ((occupancy_pct < PRODUCER_HIGH_WATERMARK_PCT || g_audio_session.state == PLAYER_STATE_OPENED) && free_bytes >= PRODUCER_CHUNK_SIZE + g_audio_session.format.block_align) {'''
ap_data = ap_data.replace(align_target4, align_fix4)

align_target5 = '''            if (free_bytes < PRODUCER_CHUNK_SIZE + 4) break;'''
align_fix5 = '''            if (free_bytes < PRODUCER_CHUNK_SIZE + g_audio_session.format.block_align) break;'''
ap_data = ap_data.replace(align_target5, align_fix5)

align_target6 = '''            uint32_t to_read = PRODUCER_CHUNK_SIZE;
            if (g_audio_session.bytes_played + to_read > g_audio_session.data_size) {
                to_read = g_audio_session.data_size - g_audio_session.bytes_played;
            }'''
align_fix6 = '''            uint32_t to_read = PRODUCER_CHUNK_SIZE;
            if (g_audio_session.bytes_played + to_read > g_audio_session.data_size) {
                to_read = g_audio_session.data_size - g_audio_session.bytes_played;
            }
            if (g_audio_session.format.block_align > 0) {
                to_read = to_read - (to_read % g_audio_session.format.block_align);
            }'''
ap_data = ap_data.replace(align_target6, align_fix6)

# Remove spammy CIV wrap log
ac97_spam = '''    static uint8_t s_prev_civ = 0;
    if (s_prev_civ == 31 && civ == 0) {
        display_print("\\n[CIV WRAP 31->0]\\n");
    }
    s_prev_civ = civ;'''
ac97_data = ac97_data.replace(ac97_spam, '')

with open(ac97_path, 'w') as f:
    f.write(ac97_data)
    
with open(player_path, 'w') as f:
    f.write(ap_data)

print("Fixes applied.")
