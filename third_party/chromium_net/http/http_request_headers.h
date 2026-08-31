/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_REQUEST_HEADERS_H_
#define THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_REQUEST_HEADERS_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"

namespace net {

struct HeaderKeyValuePair {
    std::string key;
    std::string value;
};

class HttpRequestHeaders {
public:
    HttpRequestHeaders();
    ~HttpRequestHeaders();

    bool GetHeader(const std::string& key, std::string* out) const;
    void SetHeader(const std::string& key, const std::string& value);
    void RemoveHeader(const std::string& key);
    bool HasHeader(const std::string& key) const;
    void Clear();

    std::string ToString() const;
    const std::vector<HeaderKeyValuePair>& GetHeaders() const { return headers_; }

private:
    std::vector<HeaderKeyValuePair> headers_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_REQUEST_HEADERS_H_
