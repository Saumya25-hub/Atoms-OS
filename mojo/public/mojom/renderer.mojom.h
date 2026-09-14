/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_MOJOM_RENDERER_MOJOM_H_
#define MOJO_PUBLIC_MOJOM_RENDERER_MOJOM_H_

#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "userspace/runtime/cpp/include/string"

namespace mojom {

constexpr uint32_t kRendererInterfaceId = 0x1000;

enum RendererMethod {
    RENDERER_METHOD_FRAME_READY     = 1,
    RENDERER_METHOD_TITLE_CHANGED   = 2,
    RENDERER_METHOD_STATUS          = 3,
    RENDERER_METHOD_CRASH           = 4,
    RENDERER_METHOD_NAVIGATE        = 10,
    RENDERER_METHOD_DOM_EVENT       = 11,
    RENDERER_METHOD_SET_VIEWPORT    = 12,
};

class RendererHost {
public:
    virtual ~RendererHost() {}
    virtual void FrameReady(mojo::ScopedSharedBufferHandle frame_buffer, uint32_t width, uint32_t height, uint32_t stride) = 0;
    virtual void TitleChanged(const std::string& new_title) = 0;
    virtual void RendererStatus(uint32_t status_code, const std::string& detail) = 0;
    virtual void RendererCrash(int32_t exit_code, const std::string& reason) = 0;
};

class RendererClient {
public:
    virtual ~RendererClient() {}
    virtual void Navigate(const std::string& url, const std::string& html_source) = 0;
    virtual void SendDOMEvent(uint32_t type, int32_t x, int32_t y, uint32_t key_code, uint32_t modifiers) = 0;
    virtual void SetViewportSize(uint32_t width, uint32_t height) = 0;
};

// Serialization & Dispatch Helpers for RendererHost
class RendererHostProxy {
public:
    explicit RendererHostProxy(mojo::MessagePipeHandle handle) : handle_(handle) {}

    MojoResult FrameReady(mojo::ScopedSharedBufferHandle frame_buffer, uint32_t width, uint32_t height, uint32_t stride) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_FRAME_READY);
        msg.WriteUInt32(width);
        msg.WriteUInt32(height);
        msg.WriteUInt32(stride);
        if (frame_buffer.is_valid()) {
            msg.AttachHandle(frame_buffer.release().value());
        }
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult TitleChanged(const std::string& new_title) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_TITLE_CHANGED);
        msg.WriteString(new_title);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult RendererStatus(uint32_t status_code, const std::string& detail) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_STATUS);
        msg.WriteUInt32(status_code);
        msg.WriteString(detail);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult RendererCrash(int32_t exit_code, const std::string& reason) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_CRASH);
        msg.WriteInt32(exit_code);
        msg.WriteString(reason);
        return mojo::WriteMessage(handle_, &msg);
    }

private:
    mojo::MessagePipeHandle handle_;
};

// Serialization & Dispatch Helpers for RendererClient
class RendererClientProxy {
public:
    explicit RendererClientProxy(mojo::MessagePipeHandle handle) : handle_(handle) {}

    MojoResult Navigate(const std::string& url, const std::string& html_source) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_NAVIGATE);
        msg.WriteString(url);
        msg.WriteString(html_source);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult SendDOMEvent(uint32_t type, int32_t x, int32_t y, uint32_t key_code, uint32_t modifiers) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_DOM_EVENT);
        msg.WriteUInt32(type);
        msg.WriteInt32(x);
        msg.WriteInt32(y);
        msg.WriteUInt32(key_code);
        msg.WriteUInt32(modifiers);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult SetViewportSize(uint32_t width, uint32_t height) {
        mojo::Message msg(kRendererInterfaceId, RENDERER_METHOD_SET_VIEWPORT);
        msg.WriteUInt32(width);
        msg.WriteUInt32(height);
        return mojo::WriteMessage(handle_, &msg);
    }

private:
    mojo::MessagePipeHandle handle_;
};

// Stub Dispatcher for incoming messages to RendererHost
inline bool DispatchRendererHostMessage(RendererHost* impl, mojo::Message* msg) {
    if (!impl || !msg || msg->interface_id() != kRendererInterfaceId) return false;

    switch (msg->method_ordinal()) {
        case RENDERER_METHOD_FRAME_READY: {
            uint32_t w = 0, h = 0, stride = 0;
            if (!msg->ReadUInt32(&w) || !msg->ReadUInt32(&h) || !msg->ReadUInt32(&stride)) return false;
            MojoHandle raw_h = MOJO_HANDLE_INVALID;
            msg->ExtractHandle(&raw_h);
            mojo::SharedBufferHandle sb_handle(raw_h);
            mojo::ScopedSharedBufferHandle sb(sb_handle);
            impl->FrameReady(std::move(sb), w, h, stride);
            return true;
        }
        case RENDERER_METHOD_TITLE_CHANGED: {
            std::string t;
            if (!msg->ReadString(&t)) return false;
            impl->TitleChanged(t);
            return true;
        }
        case RENDERER_METHOD_STATUS: {
            uint32_t sc = 0;
            std::string d;
            if (!msg->ReadUInt32(&sc) || !msg->ReadString(&d)) return false;
            impl->RendererStatus(sc, d);
            return true;
        }
        case RENDERER_METHOD_CRASH: {
            int32_t ec = 0;
            std::string r;
            if (!msg->ReadInt32(&ec) || !msg->ReadString(&r)) return false;
            impl->RendererCrash(ec, r);
            return true;
        }
        default:
            return false;
    }
}

// Stub Dispatcher for incoming messages to RendererClient
inline bool DispatchRendererClientMessage(RendererClient* impl, mojo::Message* msg) {
    if (!impl || !msg || msg->interface_id() != kRendererInterfaceId) return false;

    switch (msg->method_ordinal()) {
        case RENDERER_METHOD_NAVIGATE: {
            std::string url, src;
            if (!msg->ReadString(&url) || !msg->ReadString(&src)) return false;
            impl->Navigate(url, src);
            return true;
        }
        case RENDERER_METHOD_DOM_EVENT: {
            uint32_t t = 0, key = 0, mod = 0;
            int32_t x = 0, y = 0;
            if (!msg->ReadUInt32(&t) || !msg->ReadInt32(&x) || !msg->ReadInt32(&y) ||
                !msg->ReadUInt32(&key) || !msg->ReadUInt32(&mod)) return false;
            impl->SendDOMEvent(t, x, y, key, mod);
            return true;
        }
        case RENDERER_METHOD_SET_VIEWPORT: {
            uint32_t w = 0, h = 0;
            if (!msg->ReadUInt32(&w) || !msg->ReadUInt32(&h)) return false;
            impl->SetViewportSize(w, h);
            return true;
        }
        default:
            return false;
    }
}

} // namespace mojom

#endif // MOJO_PUBLIC_MOJOM_RENDERER_MOJOM_H_
