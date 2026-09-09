/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_MOJOM_STORAGE_MOJOM_H_
#define MOJO_PUBLIC_MOJOM_STORAGE_MOJOM_H_

#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "userspace/runtime/cpp/include/string"

namespace mojom {

constexpr uint32_t kStorageInterfaceId = 0x3000;

enum StorageMethod {
    STORAGE_METHOD_GET       = 1,
    STORAGE_METHOD_SET       = 2,
    STORAGE_METHOD_REMOVE    = 3,
    STORAGE_METHOD_CLEAR     = 4,
    STORAGE_METHOD_RESPONSE  = 10,
};

class StorageHost {
public:
    virtual ~StorageHost() {}
    virtual void StorageGet(uint32_t request_id, const std::string& origin, const std::string& key) = 0;
    virtual void StorageSet(uint32_t request_id, const std::string& origin, const std::string& key, const std::string& value) = 0;
    virtual void StorageRemove(uint32_t request_id, const std::string& origin, const std::string& key) = 0;
    virtual void StorageClear(uint32_t request_id, const std::string& origin) = 0;
};

class StorageClient {
public:
    virtual ~StorageClient() {}
    virtual void StorageResponse(uint32_t request_id, bool success, const std::string& value) = 0;
};

// Serialization & Dispatch Helpers for StorageHost
class StorageHostProxy {
public:
    explicit StorageHostProxy(mojo::MessagePipeHandle handle) : handle_(handle) {}

    MojoResult StorageGet(uint32_t request_id, const std::string& origin, const std::string& key) {
        mojo::Message msg(kStorageInterfaceId, STORAGE_METHOD_GET);
        msg.WriteUInt32(request_id);
        msg.WriteString(origin);
        msg.WriteString(key);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult StorageSet(uint32_t request_id, const std::string& origin, const std::string& key, const std::string& value) {
        mojo::Message msg(kStorageInterfaceId, STORAGE_METHOD_SET);
        msg.WriteUInt32(request_id);
        msg.WriteString(origin);
        msg.WriteString(key);
        msg.WriteString(value);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult StorageRemove(uint32_t request_id, const std::string& origin, const std::string& key) {
        mojo::Message msg(kStorageInterfaceId, STORAGE_METHOD_REMOVE);
        msg.WriteUInt32(request_id);
        msg.WriteString(origin);
        msg.WriteString(key);
        return mojo::WriteMessage(handle_, &msg);
    }

    MojoResult StorageClear(uint32_t request_id, const std::string& origin) {
        mojo::Message msg(kStorageInterfaceId, STORAGE_METHOD_CLEAR);
        msg.WriteUInt32(request_id);
        msg.WriteString(origin);
        return mojo::WriteMessage(handle_, &msg);
    }

private:
    mojo::MessagePipeHandle handle_;
};

// Serialization & Dispatch Helpers for StorageClient
class StorageClientProxy {
public:
    explicit StorageClientProxy(mojo::MessagePipeHandle handle) : handle_(handle) {}

    MojoResult StorageResponse(uint32_t request_id, bool success, const std::string& value) {
        mojo::Message msg(kStorageInterfaceId, STORAGE_METHOD_RESPONSE);
        msg.WriteUInt32(request_id);
        msg.WriteBool(success);
        msg.WriteString(value);
        return mojo::WriteMessage(handle_, &msg);
    }

private:
    mojo::MessagePipeHandle handle_;
};

// Stub Dispatcher for incoming messages to StorageHost
inline bool DispatchStorageHostMessage(StorageHost* impl, mojo::Message* msg) {
    if (!impl || !msg || msg->interface_id() != kStorageInterfaceId) return false;

    switch (msg->method_ordinal()) {
        case STORAGE_METHOD_GET: {
            uint32_t req_id = 0;
            std::string origin, key;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadString(&origin) || !msg->ReadString(&key)) return false;
            impl->StorageGet(req_id, origin, key);
            return true;
        }
        case STORAGE_METHOD_SET: {
            uint32_t req_id = 0;
            std::string origin, key, val;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadString(&origin) ||
                !msg->ReadString(&key) || !msg->ReadString(&val)) return false;
            impl->StorageSet(req_id, origin, key, val);
            return true;
        }
        case STORAGE_METHOD_REMOVE: {
            uint32_t req_id = 0;
            std::string origin, key;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadString(&origin) || !msg->ReadString(&key)) return false;
            impl->StorageRemove(req_id, origin, key);
            return true;
        }
        case STORAGE_METHOD_CLEAR: {
            uint32_t req_id = 0;
            std::string origin;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadString(&origin)) return false;
            impl->StorageClear(req_id, origin);
            return true;
        }
        default:
            return false;
    }
}

// Stub Dispatcher for incoming messages to StorageClient
inline bool DispatchStorageClientMessage(StorageClient* impl, mojo::Message* msg) {
    if (!impl || !msg || msg->interface_id() != kStorageInterfaceId) return false;

    switch (msg->method_ordinal()) {
        case STORAGE_METHOD_RESPONSE: {
            uint32_t req_id = 0;
            bool success = false;
            std::string val;
            if (!msg->ReadUInt32(&req_id) || !msg->ReadBool(&success) || !msg->ReadString(&val)) return false;
            impl->StorageResponse(req_id, success, val);
            return true;
        }
        default:
            return false;
    }
}

} // namespace mojom

#endif // MOJO_PUBLIC_MOJOM_STORAGE_MOJOM_H_
