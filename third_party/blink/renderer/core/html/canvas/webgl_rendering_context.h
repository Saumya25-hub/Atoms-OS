/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_WEBGL_RENDERING_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_WEBGL_RENDERING_CONTEXT_H_

#include "third_party/chromium_gpu/command_buffer/gpu_channel_host.h"
#include "userspace/runtime/cpp/include/string"
#include <stdint.h>
#include <stdbool.h>

namespace blink {

class WebGLRenderingContext {
public:
    WebGLRenderingContext(gpu::GpuChannelHost* channel = nullptr);
    ~WebGLRenderingContext();

    static WebGLRenderingContext* Create(gpu::GpuChannelHost* channel);

    // State & Clear
    void clearColor(float red, float green, float blue, float alpha);
    void clear(uint32_t mask);
    void viewport(int32_t x, int32_t y, int32_t width, int32_t height);

    // Shaders & Programs
    uint32_t createShader(uint32_t type);
    void shaderSource(uint32_t shader, const std::string& source);
    void compileShader(uint32_t shader);
    uint32_t createProgram();
    void attachShader(uint32_t program, uint32_t shader);
    void linkProgram(uint32_t program);
    void useProgram(uint32_t program);

    // Buffers
    uint32_t createBuffer();
    void bindBuffer(uint32_t target, uint32_t buffer);
    void bufferData(uint32_t target, const void* data, size_t size, uint32_t usage);

    // Textures
    uint32_t createTexture();
    void bindTexture(uint32_t target, uint32_t texture);
    void texImage2D(uint32_t target, int32_t level, uint32_t internalformat,
                    int32_t width, int32_t height, int32_t border,
                    uint32_t format, uint32_t type, const void* pixels);

    // Framebuffers
    uint32_t createFramebuffer();
    void bindFramebuffer(uint32_t target, uint32_t framebuffer);
    void framebufferTexture2D(uint32_t target, uint32_t attachment, uint32_t textarget, uint32_t texture, int32_t level);

    // Draw
    void drawArrays(uint32_t mode, int32_t first, int32_t count);
    void drawElements(uint32_t mode, int32_t count, uint32_t type, uint32_t offset);

    // Context Loss / Restoration
    bool isContextLost() const { return is_context_lost_; }
    void LoseContext();
    void RestoreContext();

    // Version / Profile
    std::string getParameter(uint32_t pname) const;

    gpu::GpuChannelHost* channel() { return channel_; }

private:
    gpu::GpuChannelHost* channel_;
    bool is_context_lost_;
    uint32_t bound_buffer_;
    uint32_t bound_texture_;
    uint32_t bound_framebuffer_;
    uint32_t active_program_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_WEBGL_RENDERING_CONTEXT_H_
