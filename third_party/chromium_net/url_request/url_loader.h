/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_URL_REQUEST_URL_LOADER_H_
#define THIRD_PARTY_CHROMIUM_NET_URL_REQUEST_URL_LOADER_H_

#include "third_party/chromium_net/base/gurl.h"
#include "third_party/chromium_net/http/http_request_headers.h"
#include "third_party/chromium_net/http/http_response_headers.h"
#include "third_party/chromium_net/http/http_cache.h"
#include "third_party/chromium_net/cookies/cookie_store.h"
#include "userspace/runtime/cpp/include/string"

namespace net {

enum URLLoaderError {
    NET_OK = 0,
    ERR_FAILED = -1,
    ERR_NAME_NOT_RESOLVED = -105,
    ERR_CONNECTION_REFUSED = -111,
    ERR_CONNECTION_TIMED_OUT = -118,
    ERR_SSL_PROTOCOL_ERROR = -107,
    ERR_INVALID_URL = -300,
    ERR_EMPTY_RESPONSE = -324,
    ERR_TOO_MANY_REDIRECTS = -310,
};

struct URLLoaderResult {
    int net_error;
    int http_status_code;
    HttpResponseHeaders response_headers;
    std::string response_body;
    std::string final_url;
    bool from_cache;
};

class URLLoader {
public:
    URLLoader(CookieStore* cookie_store, HttpCache* cache);
    ~URLLoader();

    URLLoaderResult Load(const GURL& url);
    URLLoaderResult Load(const GURL& url, const HttpRequestHeaders& extra_headers);

    static constexpr int kMaxRedirects = 10;

private:
    URLLoaderResult DoSingleRequest(const GURL& url, const HttpRequestHeaders& headers);
    CookieStore* cookie_store_;
    HttpCache* cache_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_URL_REQUEST_URL_LOADER_H_
