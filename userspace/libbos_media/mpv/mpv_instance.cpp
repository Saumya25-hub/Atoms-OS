/*
 * ============================================================================
 * ATOMS OS — BOS libmpv Adapter Instance & Lifecycle Implementation
 * userspace/libbos_media/mpv/mpv_instance.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements libmpv client session management, options configuration,
 * hardware acceleration negotiation, and playback controls.
 * ============================================================================
 */

#include "mpv_adapter.h"
extern "C" {
#include "kernel/media/bospectra/decoder/include/video_accel.h"
}
#include <string.h>

extern "C" void display_print(const char* s);

static uint64_t get_uptime_ms() {
    return __atoms_syscall0(SYS_UPTIME) / 1000;
}

static BOSMpvAdapter s_adapter __attribute__((aligned(16)));

BOSMpvAdapter* mpv_adapter_create(void) {
    BOSMpvAdapter* adapter = &s_adapter;
    memset(adapter, 0, sizeof(BOSMpvAdapter));

    adapter->state = BOS_MEDIA_STATE_IDLE;
    adapter->vfs_fd = -1;
    adapter->volume = 1.0f;
    adapter->start_time_ms = get_uptime_ms();

    // 1. Probe Hardware via Universal Video Acceleration HAL
    bospectra_video_accel_init();
    bospectra_accel_caps_t caps;
    memset(&caps, 0, sizeof(caps));
    bospectra_video_accel_get_caps(&caps);

    adapter->telemetry.is_hardware_accelerated = caps.is_hardware_accelerated;
    adapter->telemetry.acceleration_backend = caps.backend_name ? caps.backend_name : "SOFTWARE_FALLBACK";

    // 2. Initialize libmpv instance
    adapter->mpv = mpv_create();
    if (adapter->mpv) {
        // Register custom VFS protocols
        mpv_stream_cb_add_ro(adapter->mpv, "atoms", adapter, mpv_vfs_stream_open);
        mpv_stream_cb_add_ro(adapter->mpv, "bofs", adapter, mpv_vfs_stream_open);
        mpv_stream_cb_add_ro(adapter->mpv, "file", adapter, mpv_vfs_stream_open);

        // Configure default playback options
        mpv_set_option_string(adapter->mpv, "vo", "libmpv");
        mpv_set_option_string(adapter->mpv, "hwdec", caps.is_hardware_accelerated ? "auto" : "no");
        mpv_set_option_string(adapter->mpv, "audio-channels", "stereo");
        mpv_set_option_string(adapter->mpv, "audio-samplerate", "48000");

        mpv_initialize(adapter->mpv);

        // Setup software render context
        mpv_render_param params[] = {
            { MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_SW },
            { MPV_RENDER_PARAM_INVALID,  nullptr }
        };
        mpv_render_context_create(&adapter->render_ctx, adapter->mpv, params);
    }

    // 3. Initialize Audio HAL
    mpv_audio_hal_init(adapter);

    return adapter;
}

void mpv_adapter_destroy(BOSMpvAdapter* adapter) {
    if (!adapter) return;

    if (adapter->state == BOS_MEDIA_STATE_PLAYING || adapter->state == BOS_MEDIA_STATE_PAUSED) {
        mpv_adapter_stop(adapter);
    }

    mpv_audio_hal_shutdown(adapter);

    if (adapter->render_ctx) {
        mpv_render_context_free(adapter->render_ctx);
        adapter->render_ctx = nullptr;
    }

    if (adapter->mpv) {
        mpv_destroy(adapter->mpv);
        adapter->mpv = nullptr;
    }

    if (adapter->video_frame_buffer) {
        delete[] adapter->video_frame_buffer;
        adapter->video_frame_buffer = nullptr;
    }
}

static bool str_ends_with_nocase(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    size_t len_s = strlen(str);
    size_t len_sub = strlen(suffix);
    if (len_sub > len_s) return false;

    const char* p_s = str + (len_s - len_sub);
    for (size_t i = 0; i < len_sub; i++) {
        char c1 = p_s[i];
        char c2 = suffix[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
    return true;
}

int mpv_adapter_open(BOSMpvAdapter* adapter, const char* uri) {
    if (!adapter || !uri) return BOS_MEDIA_ERROR_INVALID_PARAM;

    adapter->state = BOS_MEDIA_STATE_OPENING;
    adapter->telemetry.first_failure_stage = nullptr;
    adapter->telemetry.first_failure_reason = nullptr;

    // Categorize format by container extension
    adapter->meta.has_video = false;
    adapter->meta.has_audio = false;

    if (str_ends_with_nocase(uri, ".mp4") || str_ends_with_nocase(uri, ".mkv") ||
        str_ends_with_nocase(uri, ".avi") || str_ends_with_nocase(uri, ".mov") ||
        str_ends_with_nocase(uri, ".webm") || str_ends_with_nocase(uri, ".ts")) {
        adapter->meta.has_video = true;
        adapter->meta.has_audio = true;
        strcpy(adapter->meta.container, "VIDEO");
        strcpy(adapter->meta.video_codec, "H.264 / AVC");
        strcpy(adapter->meta.audio_codec, "AAC / MP3");
    } else if (str_ends_with_nocase(uri, ".mp3")) {
        adapter->meta.has_audio = true;
        strcpy(adapter->meta.container, "MP3");
        strcpy(adapter->meta.audio_codec, "MP3 (Layer 3)");
    } else if (str_ends_with_nocase(uri, ".wav")) {
        adapter->meta.has_audio = true;
        strcpy(adapter->meta.container, "WAV");
        strcpy(adapter->meta.audio_codec, "Linear PCM");
    } else if (str_ends_with_nocase(uri, ".flac")) {
        adapter->meta.has_audio = true;
        strcpy(adapter->meta.container, "FLAC");
        strcpy(adapter->meta.audio_codec, "FLAC Lossless");
    } else if (str_ends_with_nocase(uri, ".aac")) {
        adapter->meta.has_audio = true;
        strcpy(adapter->meta.container, "AAC");
        strcpy(adapter->meta.audio_codec, "AAC LC");
    } else {
        // Generic / default: assume media container
        adapter->meta.has_video = true;
        adapter->meta.has_audio = true;
        strcpy(adapter->meta.container, "MEDIA");
    }

    // Extract title from filename
    const char* slash = strrchr(uri, '/');
    const char* filename = slash ? (slash + 1) : uri;
    strncpy(adapter->meta.title, filename, sizeof(adapter->meta.title) - 1);
    adapter->meta.title[sizeof(adapter->meta.title) - 1] = '\0';

    // Remove extension from title for display
    char* dot = strrchr(adapter->meta.title, '.');
    if (dot) *dot = '\0';

    // Issue mpv command if handle is available
    if (adapter->mpv) {
        const char* cmd[] = { "loadfile", uri, nullptr };
        int err = mpv_command(adapter->mpv, cmd);
        if (err < 0) {
            adapter->state = BOS_MEDIA_STATE_ERROR;
            adapter->telemetry.first_failure_stage = "LOADFILE";
            adapter->telemetry.first_failure_reason = mpv_error_string(err);
            return BOS_MEDIA_ERROR_INVALID_CONTAINER;
        }
    }

    adapter->state = BOS_MEDIA_STATE_PLAYING;
    return BOS_MEDIA_OK;
}

int mpv_adapter_play(BOSMpvAdapter* adapter) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    if (adapter->mpv) {
        int pause = 0;
        mpv_set_property(adapter->mpv, "pause", MPV_FORMAT_FLAG, &pause);
    }
    adapter->state = BOS_MEDIA_STATE_PLAYING;
    return BOS_MEDIA_OK;
}

int mpv_adapter_pause(BOSMpvAdapter* adapter) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    if (adapter->mpv) {
        int pause = 1;
        mpv_set_property(adapter->mpv, "pause", MPV_FORMAT_FLAG, &pause);
    }
    adapter->state = BOS_MEDIA_STATE_PAUSED;
    return BOS_MEDIA_OK;
}

int mpv_adapter_stop(BOSMpvAdapter* adapter) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    if (adapter->mpv) {
        const char* cmd[] = { "stop", nullptr };
        mpv_command(adapter->mpv, cmd);
    }
    adapter->state = BOS_MEDIA_STATE_STOPPED;
    return BOS_MEDIA_OK;
}

int mpv_adapter_seek(BOSMpvAdapter* adapter, int64_t position_ms) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    double sec = (double)position_ms / 1000.0;
    if (adapter->mpv) {
        mpv_set_property(adapter->mpv, "playback-time", MPV_FORMAT_DOUBLE, &sec);
    }
    adapter->last_frame_pts = position_ms;
    return BOS_MEDIA_OK;
}

int mpv_adapter_set_volume(BOSMpvAdapter* adapter, float volume) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    adapter->volume = volume;

    if (adapter->audio_stream_id != 0) {
        uint8_t vol_byte = (uint8_t)(volume * 255.0f);
        __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_DEVICE_SET_VOL, adapter->audio_stream_id, vol_byte);
    }
    return BOS_MEDIA_OK;
}

int mpv_adapter_set_mute(BOSMpvAdapter* adapter, bool mute) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    adapter->is_muted = mute;
    if (adapter->audio_stream_id != 0) {
        uint8_t vol_byte = mute ? 0 : (uint8_t)(adapter->volume * 255.0f);
        __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_DEVICE_SET_VOL, adapter->audio_stream_id, vol_byte);
    }
    return BOS_MEDIA_OK;
}

void mpv_adapter_tick(BOSMpvAdapter* adapter) {
    if (!adapter) return;

    // Pump mpv events
    if (adapter->mpv) {
        while (1) {
            mpv_event* event = mpv_wait_event(adapter->mpv, 0.0);
            if (!event || event->event_id == MPV_EVENT_NONE) break;

            if (event->event_id == MPV_EVENT_END_FILE) {
                mpv_event_end_file* ef = (mpv_event_end_file*)event->data;
                if (ef && ef->reason == MPV_END_FILE_REASON_EOF) {
                    adapter->state = BOS_MEDIA_STATE_STOPPED;
                } else if (ef && ef->reason == MPV_END_FILE_REASON_ERROR) {
                    adapter->state = BOS_MEDIA_STATE_ERROR;
                    adapter->telemetry.first_failure_stage = "PLAYBACK_DECODE";
                    adapter->telemetry.first_failure_reason = mpv_error_string(ef->error);
                }
            } else if (event->event_id == MPV_EVENT_VIDEO_RECONFIG) {
                adapter->frame_ready = true;
            }
        }
    }

    // Check if new render update is available
    if (adapter->render_ctx) {
        uint64_t flags = mpv_render_context_update(adapter->render_ctx);
        if (flags & MPV_RENDER_FRAME_INFO_PRESENT) {
            adapter->frame_ready = true;
        }
    }
}
