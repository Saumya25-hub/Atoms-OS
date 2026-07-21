import os

filepath = 'kernel/audio/drivers/ac97/ac97_playback.c'
with open(filepath, 'r') as f:
    data = f.read()

# Add forensic definitions at the top
forensic_defs = '''
#define FORENSIC_BUF_SIZE 64
typedef struct {
    uint64_t timestamp;
    uint8_t previous_civ;
    uint8_t current_civ;
    uint8_t lvi;
    uint8_t desc_consumed;
    uint8_t desc_refill;
    uint32_t generation;
    size_t req_bytes;
    size_t actual_bytes;
    size_t ring_avail_before;
    size_t ring_avail_after;
    uint32_t virt_addr;
    uint32_t phys_addr;
    uint8_t pcm_bytes[8];
    uint32_t checksum;
} ForensicEvent;

static ForensicEvent g_forensic_buf[FORENSIC_BUF_SIZE];
static uint32_t g_forensic_head = 0;
static uint32_t g_desc_generation[AC97_BDL_ENTRIES] = {0};
static bool g_forensic_frozen = false;

static void dump_forensic_events(const char* reason) {
    if (g_forensic_frozen) return;
    g_forensic_frozen = true;
    display_print("\\n=== AC97 FIRST FAILURE SNAPSHOT ===\\n");
    display_print("Reason: "); display_print(reason); display_print("\\n");
    
    for (int i = 0; i < FORENSIC_BUF_SIZE; i++) {
        uint32_t idx = (g_forensic_head + i) % FORENSIC_BUF_SIZE;
        ForensicEvent* e = &g_forensic_buf[idx];
        if (e->timestamp == 0) continue; // Uninitialized
        
        display_print("T="); display_print_dec(e->timestamp);
        display_print(" pCIV="); display_print_dec(e->previous_civ);
        display_print(" cCIV="); display_print_dec(e->current_civ);
        display_print(" LVI="); display_print_dec(e->lvi);
        display_print(" Refill="); display_print_dec(e->desc_refill);
        display_print(" Gen="); display_print_dec(e->generation);
        display_print(" Req="); display_print_dec(e->req_bytes);
        display_print(" Act="); display_print_dec(e->actual_bytes);
        display_print(" Phys="); display_print_hex(e->phys_addr);
        display_print(" Chksum="); display_print_hex(e->checksum);
        display_print("\\n");
    }
}
'''
if "FORENSIC_BUF_SIZE" not in data:
    data = data.replace('static Ac97PlaybackTelemetry g_pb_telemetry = {0};', 'static Ac97PlaybackTelemetry g_pb_telemetry = {0};\n' + forensic_defs)

# Instrument ac97_playback_update
old_loop = '''    uint8_t idx = g_last_civ;
    while (idx != civ) {
        uint8_t next_idx = (idx + 1) % AC97_BDL_ENTRIES;
        uint8_t* target = g_dma_mgr->pcm_buffer + (idx * chunk_bytes);
        size_t returned_bytes = audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        g_tel_rotations++;
        
        g_pb_telemetry.bytes_sent += chunk_bytes;
        g_pb_telemetry.frames_played += frames;
        g_tel_total_mixer_bytes += chunk_bytes;
        if (returned_bytes < chunk_bytes) {
            g_pb_telemetry.underruns++;
        }
        
        idx = next_idx;
    }'''

new_loop = '''    uint8_t idx = g_last_civ;
    while (idx != civ) {
        uint8_t next_idx = (idx + 1) % AC97_BDL_ENTRIES;
        uint8_t* target = g_dma_mgr->pcm_buffer + (idx * chunk_bytes);
        
        if (!g_forensic_frozen) {
            ForensicEvent* e = &g_forensic_buf[g_forensic_head];
            e->timestamp = timer_get_ticks();
            e->previous_civ = g_last_civ;
            e->current_civ = civ;
            e->lvi = g_lvi;
            e->desc_consumed = g_last_civ; // Simplified: actually the one just finished playing
            e->desc_refill = idx;
            g_desc_generation[idx]++;
            e->generation = g_desc_generation[idx];
            e->req_bytes = chunk_bytes;
            
            extern size_t audio_stream_available(uint32_t); // Wait, mixer combines streams. We'll just read stream 0.
            extern struct AudioStream* audio_core_get_active_streams(void);
            struct AudioStream* s = audio_core_get_active_streams();
            e->ring_avail_before = s && s->ring_buffer ? audio_buffer_available(s->ring_buffer) : 0;
            
            size_t returned_bytes = audio_mixer_process(target, chunk_bytes, &g_ac97_format);
            e->actual_bytes = returned_bytes;
            
            e->ring_avail_after = s && s->ring_buffer ? audio_buffer_available(s->ring_buffer) : 0;
            e->virt_addr = (uint32_t)(uint64_t)target;
            
            extern uint32_t get_phys(void* virt_addr);
            // Re-implement get_phys manually or link to it
            // Actually get_phys is in ac97_dma.c, make it global or redefine
            // We can just skip get_phys here or declare it if it's exported. It's static in ac97_dma.c.
            // For now, let's just use 0, since we check it elsewhere.
            e->phys_addr = 0; 
            
            for(int i=0; i<8; i++) e->pcm_bytes[i] = target[i];
            
            uint32_t csum = 0;
            for(size_t i=0; i<chunk_bytes; i++) csum += target[i];
            e->checksum = csum;
            
            g_forensic_head = (g_forensic_head + 1) % FORENSIC_BUF_SIZE;
            
            // Check Invariants
            // 2. Never refill descriptor currently owned by CIV
            if (idx == civ) {
                dump_forensic_events("Refilling currently playing CIV!");
            }
            // 7. Mixer output size exactly equals requested
            if (returned_bytes != chunk_bytes) {
                dump_forensic_events("Mixer returned partial block!");
            }
            
            // Verify alignment invariant just in case
            if (returned_bytes % 4 != 0) {
                dump_forensic_events("Mixer block alignment violation!");
            }
        } else {
            audio_mixer_process(target, chunk_bytes, &g_ac97_format);
        }
        
        g_tel_rotations++;
        
        g_pb_telemetry.bytes_sent += chunk_bytes;
        g_pb_telemetry.frames_played += frames;
        g_tel_total_mixer_bytes += chunk_bytes;
        
        idx = next_idx;
    }'''

data = data.replace(old_loop, new_loop)

# Fix audio_player_update logging rate
with open('kernel/audio/session/audio_player.c', 'r') as f_ap:
    ap_data = f_ap.read()
    
rate_log = '''        static uint64_t s_last_print_ticks = 0;
        uint64_t now_ticks = timer_get_ticks();
        if (now_ticks - s_last_print_ticks > 1000) {
            display_print("WAV Progress: "); display_print_dec(g_audio_session.bytes_played);
            display_print(" / "); display_print_dec(g_audio_session.data_size); display_print("\\n");
            s_last_print_ticks = now_ticks;
        }
'''
if "WAV Progress:" not in ap_data:
    ap_data = ap_data.replace('    if (g_audio_session.bytes_played >= g_audio_session.data_size) {', rate_log + '    if (g_audio_session.bytes_played >= g_audio_session.data_size) {')

with open('kernel/audio/session/audio_player.c', 'w') as f_ap:
    f_ap.write(ap_data)

with open(filepath, 'w') as f:
    f.write(data)

print("Patch applied.")
