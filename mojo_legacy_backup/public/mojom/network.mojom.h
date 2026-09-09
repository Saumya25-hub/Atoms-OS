/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_MOJOM_NETWORK_MOJOM_H_
#define MOJO_PUBLIC_MOJOM_NETWORK_MOJOM_H_

#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "userspace/runtime/cpp/include/string"

namespace mojom {

constexpr uint32_t kNetworkInterfaceId = 0x2000;

enum NetworkMethod {
    NETWORK_METHOD_URL_REQUEST   = 1,
    NETWORK_METHOD_URL_RESPONSE  = 10,
    NETWORK_METHOD_NETWORK_ERROR = 11,
};

class NetworkHost {
public:
    virtual ~NetworkHost() {}
    virtual void URLRequest(uint32_t request_id, const std::string& url, const std::string& method, const std::string& headers) = 0;
};

class NetworkClient {
public:
    virtual ~NetworkClient() {}
    virtual void URLResponse(uint32_t request_id, int32_t status_code, const std::string& headers, const std::string& body) = 0;
    virtual void NetworkError(uint32_t request_id, int32_t error_code, const std::string& message) = 0;
};

// Serialization & Dispatch Helpers for NetworkHost
class NetworkHostProxy {
public:
    explicit NetworkHostProxy(mojo::MessagePipeHandle handle) : handle_(handle) {}

    MojoResult URLRequest(uint32_t request_id, const std::string& url, const std::string& method, const std::string& headers) {
        mojo::Message msg(kNetworkInterfaceId, NETWORK_METHOD_URL_REQUEST);
        msg.WriteUInt32(request_id);
        msg.WriteString(url);
        msg.WriteString(method);
        msg.WriteString(headers);
        return mojo::WriteMessage(handle_, &msg);
    }

private:
    mojo::MessagePipeHandle handle_;
};

// Serialization & Dispatch Helpers for NetworkClient
class NetworkClientProxy {
public:
    explicit NetworkClientProxy(mojo::MessagePipeHandle handle) : handle_(handle) {}

    MojoResult URLResponse(uint32_t request_id, int32_t status_code, const std::string& headers, const std::string& body) {
        mojo::Message msg(kNetworkInterfaceId, NETWORK_METHOD_URL_RESPONSE);
        msg.WriteUInt32(request_id);
        msg.WriteInt32(status_code);
        msg.WriteString(headers);
        msg.WriteString(body);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult NetworkError(uint32_t request_id, int32_t error_code, const std::string& message) {
        mojo::Message msg(kNetworkInterfaceId, NETWORK_METHOD_NETWORK_ERROR);
        msg.WriteUInt32(request_id);
        msg.WriteInt32(error_code);
        msg.WriteString(message);
        return mojo::WriteMessage(handle_, &msg);
    }

private:
    mojo::MessagePipeHandle handle_;
};

// Stub Dispatcher for incoming messages to NetworkHost
inline bool DispatchNetworkHostMessage(NetworkHost* impl, mojo::Message* msg) {
    if (!impl || !msg || msg->interface_id() != kNetworkInterfaceId) return false;

    switch (msg->method_ordinal()) {
        case NETWORK_METHOD_URL_REQUEST: {
            uint32_t req_id = 0;
            std::string url, method, headers;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadString(&url) ||
                !msg->ReadString(&method) || !msg->ReadString(&headers)) return false;
            impl->URLRequest(req_id, url, method, headers);
            return true;
        }
        default:
            return false;
    }
}

// Stub Dispatcher for incoming messages to NetworkClient
inline bool DispatchNetworkClientMessage(NetworkClient* impl, mojo::Message* msg) {
    if (!impl || !msg || msg->interface_id() != kNetworkInterfaceId) return false;

    switch (msg->method_ordinal()) {
        case NETWORK_METHOD_URL_RESPONSE: {
            uint32_t req_id = 0;
            int32_t status = 0;
            std::string headers, body;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadInt32(&status) ||
                !msg->ReadString(&headers) || !msg->ReadString(&body)) return false;
            impl->URLResponse(req_id, status, headers, body);
            return true;
        }
        case NETWORK_METHOD_NETWORK_ERROR: {
            uint32_t req_id = 0;
            int32_t err = 0;
            std::string errMsg;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadInt32(&err) ||
                !msg->ReadString(&errMsg)) return false;
            impl->NetworkError(req_id, err, errMsg);
            return true;
        }
        default:
            return false;
    }
}

} // namespace mojom

#endif // MOJO_PUBLIC_MOJOM_NETWORK_MOJOM_H_
