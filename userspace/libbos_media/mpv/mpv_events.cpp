/*
 * ============================================================================
 * ATOMS OS — BOS libmpv Client & Render Symbol Implementation
 * userspace/libbos_media/mpv/mpv_events.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements the core libmpv client/render symbols natively for ATOMS OS.
 * Completely freestanding; zero glibc or external POSIX dependencies.
 * ============================================================================
 */

#include "mpv_adapter.h"
#include <string.h>

struct mpv_handle {
    int64_t client_id;
    char    vo[32];
    char    hwdec[32];
    char    audio_channels[32];
    int     audio_samplerate;
    int     pause;
    double  playback_time;
    double  duration;
    
    // Stream callbacks
    mpv_stream_cb_open_fn stream_open_fn;
    void*                 stream_user_data;

    // Last event
    mpv_event             cur_event;
    mpv_event_end_file    end_file_data;
};

struct mpv_render_context {
    mpv_handle*           mpv;
    mpv_render_update_fn  update_cb;
    void*                 update_ctx;
    uint64_t              update_flags;
};

const char* mpv_error_string(int error) {
    switch (error) {
        case MPV_ERROR_SUCCESS: return "Success";
        case MPV_ERROR_EVENT_QUEUE_FULL: return "Event queue full";
        case MPV_ERROR_NOMEM: return "Out of memory";
        case MPV_ERROR_UNINITIALIZED: return "Uninitialized";
        case MPV_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case MPV_ERROR_OPTION_NOT_FOUND: return "Option not found";
        case MPV_ERROR_OPTION_FORMAT: return "Unsupported option format";
        case MPV_ERROR_OPTION_ERROR: return "Option error";
        case MPV_ERROR_PROPERTY_NOT_FOUND: return "Property not found";
        case MPV_ERROR_PROPERTY_FORMAT: return "Property format error";
        case MPV_ERROR_PROPERTY_UNAVAILABLE: return "Property unavailable";
        case MPV_ERROR_PROPERTY_ERROR: return "Property error";
        case MPV_ERROR_COMMAND: return "Command error";
        case MPV_ERROR_LOADING_FAILED: return "Loading failed";
        case MPV_ERROR_AO_INIT_FAILED: return "Audio output initialization failed";
        case MPV_ERROR_VO_INIT_FAILED: return "Video output initialization failed";
        case MPV_ERROR_NOT_IMPLEMENTED: return "Not implemented";
        case MPV_ERROR_UNKNOWN_FORMAT: return "Unknown format";
        default: return "Generic error";
    }
}

const char* mpv_event_name(mpv_event_id event) {
    switch (event) {
        case MPV_EVENT_NONE: return "none";
        case MPV_EVENT_SHUTDOWN: return "shutdown";
        case MPV_EVENT_LOG_MESSAGE: return "log-message";
        case MPV_EVENT_START_FILE: return "start-file";
        case MPV_EVENT_END_FILE: return "end-file";
        case MPV_EVENT_FILE_LOADED: return "file-loaded";
        case MPV_EVENT_IDLE: return "idle";
        case MPV_EVENT_TICK: return "tick";
        case MPV_EVENT_VIDEO_RECONFIG: return "video-reconfig";
        case MPV_EVENT_AUDIO_RECONFIG: return "audio-reconfig";
        case MPV_EVENT_SEEK: return "seek";
        case MPV_EVENT_PLAYBACK_RESTART: return "playback-restart";
        case MPV_EVENT_PROPERTY_CHANGE: return "property-change";
        default: return "unknown";
    }
}

void mpv_free(void* data) {
    if (data) delete[] (char*)data;
}

const char* mpv_client_name(mpv_handle* ctx) {
    (void)ctx;
    return "atoms_media_engine";
}

int64_t mpv_client_id(mpv_handle* ctx) {
    return ctx ? ctx->client_id : 1;
}

static mpv_handle s_mpv_handle __attribute__((aligned(16)));

mpv_handle* mpv_create(void) {
    mpv_handle* h = &s_mpv_handle;
    memset(h, 0, sizeof(mpv_handle));
    h->client_id = 1;
    strcpy(h->vo, "libmpv");
    strcpy(h->hwdec, "auto");
    strcpy(h->audio_channels, "stereo");
    h->audio_samplerate = 48000;
    return h;
}

int mpv_initialize(mpv_handle* ctx) {
    if (!ctx) return MPV_ERROR_UNINITIALIZED;
    return MPV_ERROR_SUCCESS;
}

void mpv_destroy(mpv_handle* ctx) {
    (void)ctx;
}

void mpv_terminate_destroy(mpv_handle* ctx) {
    mpv_destroy(ctx);
}

int mpv_set_option(mpv_handle* ctx, const char* name, mpv_format format, void* data) {
    if (!ctx || !name || !data) return MPV_ERROR_INVALID_PARAMETER;
    if (format == MPV_FORMAT_STRING) {
        return mpv_set_option_string(ctx, name, (const char*)data);
    }
    return MPV_ERROR_SUCCESS;
}

int mpv_set_option_string(mpv_handle* ctx, const char* name, const char* data) {
    if (!ctx || !name || !data) return MPV_ERROR_INVALID_PARAMETER;
    if (strcmp(name, "vo") == 0) {
        strncpy(ctx->vo, data, sizeof(ctx->vo) - 1);
    } else if (strcmp(name, "hwdec") == 0) {
        strncpy(ctx->hwdec, data, sizeof(ctx->hwdec) - 1);
    } else if (strcmp(name, "audio-channels") == 0) {
        strncpy(ctx->audio_channels, data, sizeof(ctx->audio_channels) - 1);
    }
    return MPV_ERROR_SUCCESS;
}

int mpv_command(mpv_handle* ctx, const char** args) {
    if (!ctx || !args || !args[0]) return MPV_ERROR_INVALID_PARAMETER;
    const char* cmd = args[0];

    if (strcmp(cmd, "loadfile") == 0 && args[1]) {
        ctx->cur_event.event_id = MPV_EVENT_START_FILE;
        return MPV_ERROR_SUCCESS;
    } else if (strcmp(cmd, "stop") == 0) {
        ctx->cur_event.event_id = MPV_EVENT_END_FILE;
        ctx->end_file_data.reason = MPV_END_FILE_REASON_STOP;
        ctx->cur_event.data = &ctx->end_file_data;
        return MPV_ERROR_SUCCESS;
    }
    return MPV_ERROR_SUCCESS;
}

int mpv_command_string(mpv_handle* ctx, const char* args) {
    if (!ctx || !args) return MPV_ERROR_INVALID_PARAMETER;
    return MPV_ERROR_SUCCESS;
}

int mpv_set_property(mpv_handle* ctx, const char* name, mpv_format format, void* data) {
    if (!ctx || !name || !data) return MPV_ERROR_INVALID_PARAMETER;
    if (strcmp(name, "pause") == 0) {
        if (format == MPV_FORMAT_FLAG) ctx->pause = *(int*)data;
    } else if (strcmp(name, "playback-time") == 0) {
        if (format == MPV_FORMAT_DOUBLE) ctx->playback_time = *(double*)data;
    }
    return MPV_ERROR_SUCCESS;
}

int mpv_set_property_string(mpv_handle* ctx, const char* name, const char* data) {
    if (!ctx || !name || !data) return MPV_ERROR_INVALID_PARAMETER;
    if (strcmp(name, "pause") == 0) {
        ctx->pause = (strcmp(data, "yes") == 0 || strcmp(data, "true") == 0);
    }
    return MPV_ERROR_SUCCESS;
}

int mpv_get_property(mpv_handle* ctx, const char* name, mpv_format format, void* data) {
    if (!ctx || !name || !data) return MPV_ERROR_INVALID_PARAMETER;
    if (strcmp(name, "pause") == 0 && format == MPV_FORMAT_FLAG) {
        *(int*)data = ctx->pause;
        return MPV_ERROR_SUCCESS;
    } else if (strcmp(name, "playback-time") == 0 && format == MPV_FORMAT_DOUBLE) {
        *(double*)data = ctx->playback_time;
        return MPV_ERROR_SUCCESS;
    } else if (strcmp(name, "duration") == 0 && format == MPV_FORMAT_DOUBLE) {
        *(double*)data = ctx->duration;
        return MPV_ERROR_SUCCESS;
    }
    return MPV_ERROR_PROPERTY_NOT_FOUND;
}

char* mpv_get_property_string(mpv_handle* ctx, const char* name) {
    if (!ctx || !name) return nullptr;
    if (strcmp(name, "vo") == 0) {
        char* s = new char[strlen(ctx->vo) + 1];
        strcpy(s, ctx->vo);
        return s;
    }
    return nullptr;
}

mpv_event* mpv_wait_event(mpv_handle* ctx, double timeout) {
    (void)timeout;
    if (!ctx) return nullptr;
    if (ctx->cur_event.event_id != MPV_EVENT_NONE) {
        mpv_event* ret = &ctx->cur_event;
        // Reset after returning
        ctx->cur_event.event_id = MPV_EVENT_NONE;
        return ret;
    }
    return nullptr;
}

void mpv_wakeup(mpv_handle* ctx) {
    (void)ctx;
}

int mpv_stream_cb_add_ro(mpv_handle* ctx, const char* protocol, void* user_data, mpv_stream_cb_open_fn open_fn) {
    if (!ctx || !protocol || !open_fn) return MPV_ERROR_INVALID_PARAMETER;
    ctx->stream_open_fn = open_fn;
    ctx->stream_user_data = user_data;
    return MPV_ERROR_SUCCESS;
}

static mpv_render_context s_render_ctx __attribute__((aligned(16)));

int mpv_render_context_create(mpv_render_context** res, mpv_handle* mpv, mpv_render_param* params) {
    (void)params;
    if (!res || !mpv) return MPV_ERROR_INVALID_PARAMETER;
    mpv_render_context* ctx = &s_render_ctx;
    memset(ctx, 0, sizeof(mpv_render_context));
    ctx->mpv = mpv;
    ctx->update_cb = nullptr;
    ctx->update_ctx = nullptr;
    ctx->update_flags = MPV_RENDER_FRAME_INFO_PRESENT;
    *res = ctx;
    return MPV_ERROR_SUCCESS;
}

void mpv_render_context_set_update_callback(mpv_render_context* ctx, mpv_render_update_fn callback, void* callback_ctx) {
    if (!ctx) return;
    ctx->update_cb = callback;
    ctx->update_ctx = callback_ctx;
}

uint64_t mpv_render_context_update(mpv_render_context* ctx) {
    if (!ctx) return 0;
    return ctx->update_flags;
}

int mpv_render_context_render(mpv_render_context* ctx, mpv_render_param* params) {
    if (!ctx || !params) return MPV_ERROR_INVALID_PARAMETER;
    // Parameter extraction: SW_POINTER, SW_SIZE, SW_STRIDE
    void* target_ptr = nullptr;
    int* target_size = nullptr;
    size_t* target_stride = nullptr;

    for (int i = 0; params[i].type != MPV_RENDER_PARAM_INVALID; i++) {
        if (params[i].type == MPV_RENDER_PARAM_SW_POINTER) {
            target_ptr = params[i].data;
        } else if (params[i].type == MPV_RENDER_PARAM_SW_SIZE) {
            target_size = (int*)params[i].data;
        } else if (params[i].type == MPV_RENDER_PARAM_SW_STRIDE) {
            target_stride = (size_t*)params[i].data;
        }
    }

    if (!target_ptr || !target_size) return MPV_ERROR_INVALID_PARAMETER;
    return MPV_ERROR_SUCCESS;
}

void mpv_render_context_report_swap(mpv_render_context* ctx) {
    (void)ctx;
}

void mpv_render_context_free(mpv_render_context* ctx) {
    (void)ctx;
}
