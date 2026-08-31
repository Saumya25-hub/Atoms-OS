/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "webgl_rendering_context.h"
#include "kernel/graphics/gl/gl.h"
#include "userspace/libs/opengl32/include/opengl32_api.h"

static uint32_t g_webgl_id_counter = 1;

namespace blink {

WebGLRenderingContext::WebGLRenderingContext(gpu::GpuChannelHost* channel)
    : channel_(channel),
      is_context_lost_(false),
      bound_buffer_(0),
      bound_texture_(0),
      bound_framebuffer_(0),
      active_program_(0) {
}

WebGLRenderingContext::~WebGLRenderingContext() {
}

WebGLRenderingContext* WebGLRenderingContext::Create(gpu::GpuChannelHost* channel) {
    return new WebGLRenderingContext(channel);
}

void WebGLRenderingContext::clearColor(float red, float green, float blue, float alpha) {
    if (is_context_lost_) return;
    if (channel_) {
        gpu::GpuCommand cmd;
        cmd.type = gpu::CMD_CLEAR_COLOR;
        cmd.f1 = red; cmd.f2 = green; cmd.f3 = blue; cmd.f4 = alpha;
        channel_->SendCommand(cmd);
        channel_->Flush();
    } else {
        glClearColor(red, green, blue, alpha);
    }
}

void WebGLRenderingContext::clear(uint32_t mask) {
    if (is_context_lost_) return;
    if (channel_) {
        gpu::GpuCommand cmd;
        cmd.type = gpu::CMD_CLEAR;
        cmd.arg1 = mask;
        channel_->SendCommand(cmd);
        channel_->Flush();
    } else {
        glClear(mask);
    }
}

void WebGLRenderingContext::viewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (is_context_lost_) return;
    if (channel_) {
        gpu::GpuCommand cmd;
        cmd.type = gpu::CMD_VIEWPORT;
        cmd.arg1 = (uint32_t)x; cmd.arg2 = (uint32_t)y;
        cmd.arg3 = (uint32_t)width; cmd.arg4 = (uint32_t)height;
        channel_->SendCommand(cmd);
        channel_->Flush();
    } else {
        glViewport(x, y, width, height);
    }
}

uint32_t WebGLRenderingContext::createShader(uint32_t type) {
    if (is_context_lost_) return 0;
    (void)type;
    return g_webgl_id_counter++;
}

void WebGLRenderingContext::shaderSource(uint32_t shader, const std::string& source) {
    if (is_context_lost_) return;
    (void)shader;
    (void)source;
}

void WebGLRenderingContext::compileShader(uint32_t shader) {
    if (is_context_lost_) return;
    (void)shader;
}

uint32_t WebGLRenderingContext::createProgram() {
    if (is_context_lost_) return 0;
    return g_webgl_id_counter++;
}

void WebGLRenderingContext::attachShader(uint32_t program, uint32_t shader) {
    if (is_context_lost_) return;
    (void)program;
    (void)shader;
}

void WebGLRenderingContext::linkProgram(uint32_t program) {
    if (is_context_lost_) return;
    (void)program;
}

void WebGLRenderingContext::useProgram(uint32_t program) {
    if (is_context_lost_) return;
    active_program_ = program;
}

uint32_t WebGLRenderingContext::createBuffer() {
    if (is_context_lost_) return 0;
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    return (uint32_t)buf;
}

void WebGLRenderingContext::bindBuffer(uint32_t target, uint32_t buffer) {
    if (is_context_lost_) return;
    bound_buffer_ = buffer;
    glBindBuffer(target, buffer);
}

void WebGLRenderingContext::bufferData(uint32_t target, const void* data, size_t size, uint32_t usage) {
    if (is_context_lost_) return;
    (void)usage;
    glBindBuffer(target, bound_buffer_);
}

uint32_t WebGLRenderingContext::createTexture() {
    if (is_context_lost_) return 0;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    return (uint32_t)tex;
}

void WebGLRenderingContext::bindTexture(uint32_t target, uint32_t texture) {
    if (is_context_lost_) return;
    bound_texture_ = texture;
    glBindTexture(target, texture);
}

void WebGLRenderingContext::texImage2D(uint32_t target, int32_t level, uint32_t internalformat,
                                       int32_t width, int32_t height, int32_t border,
                                       uint32_t format, uint32_t type, const void* pixels) {
    if (is_context_lost_) return;
    glTexImage2D(target, level, (GLint)internalformat, (GLsizei)width, (GLsizei)height, (GLint)border, format, type, pixels);
}

uint32_t WebGLRenderingContext::createFramebuffer() {
    if (is_context_lost_) return 0;
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    return (uint32_t)fbo;
}

void WebGLRenderingContext::bindFramebuffer(uint32_t target, uint32_t framebuffer) {
    if (is_context_lost_) return;
    bound_framebuffer_ = framebuffer;
    glBindFramebuffer(target, framebuffer);
}

void WebGLRenderingContext::framebufferTexture2D(uint32_t target, uint32_t attachment, uint32_t textarget, uint32_t texture, int32_t level) {
    if (is_context_lost_) return;
    glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void WebGLRenderingContext::drawArrays(uint32_t mode, int32_t first, int32_t count) {
    if (is_context_lost_) return;
    if (channel_) {
        gpu::GpuCommand cmd;
        cmd.type = gpu::CMD_DRAW_ARRAYS;
        cmd.arg1 = mode; cmd.arg2 = (uint32_t)first; cmd.arg3 = (uint32_t)count;
        channel_->SendCommand(cmd);
        channel_->Flush();
    } else {
        glDrawArrays(mode, first, count);
    }
}

void WebGLRenderingContext::drawElements(uint32_t mode, int32_t count, uint32_t type, uint32_t offset) {
    if (is_context_lost_) return;
    if (channel_) {
        gpu::GpuCommand cmd;
        cmd.type = gpu::CMD_DRAW_ELEMENTS;
        cmd.arg1 = mode; cmd.arg2 = (uint32_t)count; cmd.arg3 = type;
        cmd.data_ptr = (const void*)(uintptr_t)offset;
        channel_->SendCommand(cmd);
        channel_->Flush();
    } else {
        glDrawElements(mode, count, type, (const void*)(uintptr_t)offset);
    }
}

void WebGLRenderingContext::LoseContext() {
    is_context_lost_ = true;
}

void WebGLRenderingContext::RestoreContext() {
    is_context_lost_ = false;
}

std::string WebGLRenderingContext::getParameter(uint32_t pname) const {
    if (pname == 0x1F02 /* GL_VERSION */) {
        return "WebGL 1.0 (OpenGL 2.0 ATOMS BGL)";
    } else if (pname == 0x1F00 /* GL_VENDOR */) {
        return "ATOMS OS Graphics Project";
    } else if (pname == 0x1F01 /* GL_RENDERER */) {
        return "ATOMS Software Rasterizer BGL V1.0";
    }
    return "WebGL 1.0";
}

} // namespace blink
