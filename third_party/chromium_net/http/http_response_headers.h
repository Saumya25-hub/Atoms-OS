/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_RESPONSE_HEADERS_H_
#define THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_RESPONSE_HEADERS_H_

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"
#include "http_request_headers.h"

namespace net {

class HttpResponseHeaders {
public:
    HttpResponseHeaders();
    explicit HttpResponseHeaders(const std::string& raw_headers);
    ~HttpResponseHeaders();

    int response_code() const { return response_code_; }
    const std::string& status_line() const { return status_line_; }

    bool GetNormalizedHeader(const std::string& key, std::string* out) const;
    bool HasHeader(const std::string& key) const;
    int64_t GetContentLength() const;
    bool IsRedirect() const;
    bool GetLocationHeader(std::string* out_location) const;

    std::vector<std::string> GetSetCookieHeaders() const;
    const std::vector<HeaderKeyValuePair>& GetHeaders() const { return headers_; }
    std::string raw_headers() const { return raw_headers_; }

private:
    void ParseRawHeaders(const std::string& raw);

    int response_code_;
    std::string status_line_;
    std::string raw_headers_;
    std::vector<HeaderKeyValuePair> headers_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_HTTP_HTTP_RESPONSE_HEADERS_H_
