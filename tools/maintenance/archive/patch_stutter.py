import os

# --- AC97 PLAYBACK PATCH ---
ac97_path = 'kernel/audio/drivers/ac97/ac97_playback.c'
with open(ac97_path, 'r') as f:
    pb_data = f.read()

# Add counters globally
counters = '''
static uint32_t g_CIVMaxJump = 0;
static uint32_t g_DescriptorsRecoveredAfterCIVJump = 0;
static uint32_t g_MixerShortFills = 0;
static uint64_t g_MixerSilenceInjectedBytes = 0;
static uint64_t g_AudioWorkerMaxGapUs = 0;
static uint64_t g_last_ac97_update_ticks = 0;
'''
if 'g_CIVMaxJump' not in pb_data:
    pb_data = pb_data.replace('static uint32_t g_tel_late_refills = 0;', 'static uint32_t g_tel_late_refills = 0;\n' + counters)

# Strip out the huge forensic dump loop
if 'if (!g_forensic_frozen) {' in pb_data:
    # We will replace the entire loop block with a cleaner one that has our telemetry
    old_loop_start = pb_data.find('    uint8_t idx = g_last_civ;')
    old_loop_end = pb_data.find('    g_last_civ = civ;', old_loop_start)
    
    clean_loop = '''    uint8_t idx = g_last_civ;
    uint32_t jump_size = (civ + AC97_BDL_ENTRIES - g_last_civ) % AC97_BDL_ENTRIES;
    if (jump_size > g_CIVMaxJump && jump_size < AC97_BDL_ENTRIES) {
        g_CIVMaxJump = jump_size;
    }
    if (jump_size > 1) {
        g_DescriptorsRecoveredAfterCIVJump += jump_size;
    }
    
    extern uint64_t timer_get_ticks(void);
    extern uint64_t step14_cycles_to_us(uint64_t);
    uint64_t now_update = timer_get_ticks();
    if (g_last_ac97_update_ticks != 0) {
        uint64_t gap = step14_cycles_to_us(now_update - g_last_ac97_update_ticks);
        if (gap > g_AudioWorkerMaxGapUs) {
            g_AudioWorkerMaxGapUs = gap;
        }
    }
    g_last_ac97_update_ticks = now_update;

    while (idx != civ) {
        uint8_t next_idx = (idx + 1) % AC97_BDL_ENTRIES;
        uint8_t* target = g_dma_mgr->pcm_buffer + (idx * chunk_bytes);
        
        size_t returned_bytes = audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        
        if (returned_bytes < chunk_bytes) {
            g_MixerShortFills++;
            size_t missing = chunk_bytes - returned_bytes;
            g_MixerSilenceInjectedBytes += missing;
            for (size_t i = returned_bytes; i < chunk_bytes; i++) {
                target[i] = 0;
            }
        }
        
        g_tel_rotations++;
        g_pb_telemetry.bytes_sent += chunk_bytes;
        g_pb_telemetry.frames_played += chunk_bytes / 4;
        g_tel_total_mixer_bytes += chunk_bytes;
        
        idx = next_idx;
    }
'''
    pb_data = pb_data[:old_loop_start] + clean_loop + pb_data[old_loop_end:]

# Inject diagnostic print in ac97_playback_status
status_end = pb_data.find('    display_print("========================================\\n");', pb_data.find('ac97_playback_status'))
if status_end != -1 and 'CIVMaxJump' not in pb_data[status_end-200:status_end]:
    new_status = '''    display_print("CIVMaxJump        : "); display_print_dec(g_CIVMaxJump); display_print("\\n");
    display_print("DescRecovered     : "); display_print_dec(g_DescriptorsRecoveredAfterCIVJump); display_print("\\n");
    display_print("MixerShortFills   : "); display_print_dec(g_MixerShortFills); display_print("\\n");
    display_print("MixerSilenceBytes : "); display_print_dec(g_MixerSilenceInjectedBytes); display_print("\\n");
    display_print("AudioWorkerMaxGap : "); display_print_dec(g_AudioWorkerMaxGapUs); display_print(" us\\n");
    extern void audio_player_print_telemetry(void);
    audio_player_print_telemetry();
'''
    pb_data = pb_data[:status_end] + new_status + pb_data[status_end:]

with open(ac97_path, 'w') as f:
    f.write(pb_data)

# --- AUDIO PLAYER PATCH ---
player_path = 'kernel/audio/session/audio_player.c'
with open(player_path, 'r') as f:
    ap_data = f.read()

# Add counters globally
counters = '''
static uint32_t g_AudioRingMinAvailable = 0xFFFFFFFF;
static uint32_t g_ProducerRefillCalls = 0;
static uint64_t g_ProducerBytesRead = 0;
static uint64_t g_ProducerMaxServiceGapMs = 0;
static uint64_t g_last_producer_ticks = 0;

void audio_player_print_telemetry(void) {
    display_print("RingMinAvailable  : "); display_print_dec(g_AudioRingMinAvailable); display_print("\\n");
    display_print("ProducerRefillCall: "); display_print_dec(g_ProducerRefillCalls); display_print("\\n");
    display_print("ProducerBytesRead : "); display_print_dec(g_ProducerBytesRead); display_print("\\n");
    display_print("ProdMaxServiceGap : "); display_print_dec(g_ProducerMaxServiceGapMs); display_print(" ms\\n");
}
'''
if 'g_AudioRingMinAvailable' not in ap_data:
    ap_data = ap_data.replace('static uint64_t g_last_read_time_ticks = 0;', 'static uint64_t g_last_read_time_ticks = 0;\n' + counters)

# Strip out display prints in audio_player_update
import re
# Remove the WAV Progress prints and AUDIO ALIGNMENT VIOLATION and audio_forensic_record
lines = ap_data.split('\\n')
new_lines = []
skip = False
for line in lines:
    if 'static uint64_t s_last_print_ticks = 0;' in line:
        skip = True
    if skip and 's_last_print_ticks = now_ticks;' in line:
        skip = False
        continue
    if skip and '}' in line and 'if' in line:
        pass
        
    if 'audio_forensic_record' in line or 'display_print' in line and not 'audio_player_print_telemetry' in line and not 'Entered audio_player' in line and not 'AUDIO_PLAYER' in line and not 'WAV format' in line:
        # We only want to strip spammy ones inside the update loop
        if 'audio_player_update' in '\n'.join(new_lines[-20:]) or 'audio_player_update' in ap_data:
            # Actually, to be safe, I'll just use simple regex or replace.
            pass

# Let's do a more robust replace for the telemetry inside audio_player_update
ap_data = re.sub(r'static uint64_t s_last_print_ticks = 0;.*?s_last_print_ticks = now_ticks;\s*\}', '', ap_data, flags=re.DOTALL)
ap_data = re.sub(r'audio_forensic_record\([^;]+;\n?', '', ap_data)
ap_data = re.sub(r'display_print\("\\n\[AUDIO ALIGNMENT VIOLATION\].*?display_print\("\\n"\);', '', ap_data, flags=re.DOTALL)
ap_data = re.sub(r'display_print\("\\n\[AUDIO LOOP BOUNDARY\].*?display_print\("\\n"\);\s*\}', '', ap_data, flags=re.DOTALL)

# Add producer telemetry tracking
prod_track = '''
    extern uint64_t timer_get_ticks(void);
    extern uint64_t step14_cycles_to_us(uint64_t);
    uint64_t now_ticks = timer_get_ticks();
    if (g_last_producer_ticks != 0) {
        uint64_t gap_ms = step14_cycles_to_us(now_ticks - g_last_producer_ticks) / 1000;
        if (gap_ms > g_ProducerMaxServiceGapMs) {
            g_ProducerMaxServiceGapMs = gap_ms;
        }
    }
    g_last_producer_ticks = now_ticks;

    size_t available_bytes = audio_stream_available(g_audio_session.stream_id);
    if (available_bytes < g_AudioRingMinAvailable) g_AudioRingMinAvailable = available_bytes;
'''
if 'g_AudioRingMinAvailable = available_bytes' not in ap_data:
    ap_data = ap_data.replace('size_t available_bytes = audio_stream_available(g_audio_session.stream_id);', prod_track, 1)

# Inside the refill loop
refill_track = '''g_ProducerRefillCalls++;
            int read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);
            if (read_bytes > 0) g_ProducerBytesRead += read_bytes;'''
ap_data = ap_data.replace('int read_bytes = vfs_read(g_audio_session.fd, g_read_buffer, to_read);', refill_track)

with open(player_path, 'w') as f:
    f.write(ap_data)

print("Patch applied.")
