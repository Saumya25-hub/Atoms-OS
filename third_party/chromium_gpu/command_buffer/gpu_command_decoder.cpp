/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gpu_command_decoder.h"
#include "userspace/libs/opengl32/include/opengl32_api.h"

namespace gpu {

GpuCommandDecoder::GpuCommandDecoder()
    : is_initialized_(false),
      window_id_(0),
      width_(0),
      height_(0),
      draw_call_count_(0),
      frame_count_(0),
      context_(nullptr),
      drawable_(nullptr) {
}

GpuCommandDecoder::~GpuCommandDecoder() {
    Shutdown();
}

bool GpuCommandDecoder::Initialize(uint32_t window_id, uint32_t width, uint32_t height) {
    if (is_initialized_) return true;

    window_id_ = window_id;
    width_ = (width > 0) ? width : 1920;
    height_ = (height > 0) ? height : 1080;

    drawable_ = bglCreateDrawableForWindow(window_id_);
    if (drawable_) {
        context_ = bglCreateContext(drawable_);
        if (context_) {
            bglMakeCurrent(context_, drawable_);
        }
    }

    is_initialized_ = true;
    return true;
}

void GpuCommandDecoder::Shutdown() {
    if (!is_initialized_) return;

    if (context_) {
        bglDestroyContext(context_);
        context_ = nullptr;
    }
    if (drawable_) {
        bglDestroyDrawable(drawable_);
        drawable_ = nullptr;
    }

    is_initialized_ = false;
}

bool GpuCommandDecoder::ProcessCommand(const GpuCommand& cmd) {
    switch (cmd.type) {
        case CMD_NOP:
            return true;

        case CMD_INITIALIZE:
            return Initialize(cmd.arg1, cmd.arg2, cmd.arg3);

        case CMD_CLEAR_COLOR:
            glClearColor(cmd.f1, cmd.f2, cmd.f3, cmd.f4);
            return true;

        case CMD_CLEAR:
            glClear(cmd.arg1);
            return true;

        case CMD_VIEWPORT:
            glViewport((GLint)cmd.arg1, (GLint)cmd.arg2, (GLsizei)cmd.arg3, (GLsizei)cmd.arg4);
            return true;

        case CMD_CREATE_BUFFER: {
            GLuint buf = 0;
            glGenBuffers(1, &buf);
            return (buf > 0);
        }

        case CMD_BUFFER_DATA:
            glBindBuffer(cmd.arg1, cmd.arg2);
            return true;

        case CMD_CREATE_TEXTURE: {
            GLuint tex = 0;
            glGenTextures(1, &tex);
            return (tex > 0);
        }

        case CMD_TEX_IMAGE_2D:
            glBindTexture(GL_TEXTURE_2D, cmd.arg1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)cmd.arg2, (GLsizei)cmd.arg3, 0, GL_RGBA, GL_UNSIGNED_BYTE, cmd.data_ptr);
            return true;

        case CMD_CREATE_FRAMEBUFFER: {
            GLuint fbo = 0;
            glGenFramebuffers(1, &fbo);
            return (fbo > 0);
        }

        case CMD_FRAMEBUFFER_TEXTURE_2D:
            glBindFramebuffer(GL_FRAMEBUFFER, cmd.arg1);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cmd.arg2, 0);
            return true;

        case CMD_DRAW_ARRAYS:
            glDrawArrays(cmd.arg1, (GLint)cmd.arg2, (GLsizei)cmd.arg3);
            draw_call_count_++;
            return true;

        case CMD_DRAW_ELEMENTS:
            glDrawElements(cmd.arg1, (GLsizei)cmd.arg2, cmd.arg3, cmd.data_ptr);
            draw_call_count_++;
            return true;

        case CMD_SWAP_BUFFERS:
            if (context_) {
                bglSwapBuffers(context_);
            }
            frame_count_++;
            return true;

        case CMD_SHUTDOWN:
            Shutdown();
            return true;

        default:
            return false;
    }
}

bool GpuCommandDecoder::FlushCommands(CommandBuffer* buffer) {
    if (!buffer) return false;
    GpuCommand cmd;
    bool success = true;
    while (buffer->Get(&cmd)) {
        if (!ProcessCommand(cmd)) {
            success = false;
        }
    }
    return success;
}

} // namespace gpu
