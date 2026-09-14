#include "audio_bridge.h"
#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/hal/audio_hal.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static bool g_audio_bridge_initialized = false;
static bool g_hardware_ready = false;
static uint32_t g_bospectra_stream_id = 0;

bospectra_error_t bospectra_audio_bridge_init(void) {
    g_audio_bridge_initialized = true;

    // Connect to BOS audio mixer and HAL
    if (g_bospectra_stream_id == 0) {
        g_bospectra_stream_id = audio_stream_create(0);
        if (g_bospectra_stream_id != 0) {
            audio_set_volume(g_bospectra_stream_id, 255);
            audio_mixer_add_stream(g_bospectra_stream_id);
            bospectra_log("AUDIO_BRIDGE", "Registered BOSpectra audio stream with BOS Audio Mixer.");
        }
    }

    g_hardware_ready = (audio_hal_get_active_driver() != NULL);
    if (g_hardware_ready) {
        bospectra_log("AUDIO_BRIDGE", "Connected to ATOMS OS Audio HAL (Real Hardware Output Active).");
    } else {
        bospectra_log("AUDIO_BRIDGE", "Audio Engine prepared in software bridge mode.");
    }

    return BOSPECTRA_SUCCESS;
}

void bospectra_audio_bridge_shutdown(void) {
    if (g_bospectra_stream_id != 0) {
        audio_mixer_remove_stream(g_bospectra_stream_id);
        audio_stream_destroy(g_bospectra_stream_id);
        g_bospectra_stream_id = 0;
    }
    g_hardware_ready = false;
    g_audio_bridge_initialized = false;
}

bool bospectra_audio_bridge_is_hardware_ready(void) {
    return g_audio_bridge_initialized;
}

bospectra_error_t bospectra_audio_bridge_write_pcm(const uint8_t* pcm_data, size_t size_bytes, const BOSPECTRA_AudioSpec* spec) {
    if (!g_audio_bridge_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!pcm_data || size_bytes == 0) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (g_bospectra_stream_id == 0) {
        g_bospectra_stream_id = audio_stream_create(0);
        if (g_bospectra_stream_id != 0) {
            audio_set_volume(g_bospectra_stream_id, 255);
            audio_mixer_add_stream(g_bospectra_stream_id);
        }
    }

    AudioPcmFormat fmt;
    fmt.format = PCM_FORMAT_S16_LE;
    fmt.sample_rate = (spec && spec->sample_rate > 0) ? spec->sample_rate : 44100;
    fmt.channels = (spec && spec->channels > 0) ? spec->channels : 2;
    fmt.bit_depth = 16;
    fmt.is_signed = true;

    if (g_bospectra_stream_id != 0) {
        audio_stream_set_format(g_bospectra_stream_id, &fmt);

        AudioPcmPacket pkt;
        pkt.format = fmt;
        pkt.frame_count = (uint32_t)(size_bytes / (fmt.channels * 2));
        pkt.timestamp = 0;
        pkt.flags = 0;
        pkt.pcm_data = (uint8_t*)pcm_data;
        pkt.size_bytes = (uint32_t)size_bytes;

        audio_stream_write(g_bospectra_stream_id, &pkt);
    }

    // Trigger HAL pointer update / DMA refill
    audio_hal_update_pointers(0);

    return BOSPECTRA_SUCCESS;
}
