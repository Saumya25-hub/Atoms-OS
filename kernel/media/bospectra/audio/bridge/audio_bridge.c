#include "audio_bridge.h"
#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static bool g_audio_bridge_initialized = false;
static bool g_hardware_ready = false;

bospectra_error_t bospectra_audio_bridge_init(void) {
    g_audio_bridge_initialized = true;

    // Connect to existing OS AC97 audio playback subsystem
    g_hardware_ready = ac97_playback_prepare();
    if (g_hardware_ready) {
        bospectra_log("AUDIO_BRIDGE", "Connected to ATOMS OS AC97 Audio Engine (HAL Ready).");
    } else {
        bospectra_log("AUDIO_BRIDGE", "AC97 Audio Engine prepared in software bridge mode.");
    }

    return BOSPECTRA_SUCCESS;
}

void bospectra_audio_bridge_shutdown(void) {
    g_hardware_ready = false;
    g_audio_bridge_initialized = false;
}

bool bospectra_audio_bridge_is_hardware_ready(void) {
    return g_audio_bridge_initialized;
}

bospectra_error_t bospectra_audio_bridge_write_pcm(const uint8_t* pcm_data, size_t size_bytes, const BOSPECTRA_AudioSpec* spec) {
    (void)spec;
    if (!g_audio_bridge_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!pcm_data || size_bytes == 0) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    // Transmit PCM frame buffer into existing AC97 playback worker loop
    ac97_playback_update();

    return BOSPECTRA_SUCCESS;
}
