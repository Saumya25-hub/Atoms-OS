/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "url_loader.h"
#include "third_party/chromium_net/adapter/atoms_network_adapter.h"

namespace net {

URLLoader::URLLoader(CookieStore* cookie_store, HttpCache* cache)
    : cookie_store_(cookie_store)
    , cache_(cache)
{
}

URLLoader::~URLLoader() {}

URLLoaderResult URLLoader::Load(const GURL& url) {
    HttpRequestHeaders empty_headers;
    return Load(url, empty_headers);
}

URLLoaderResult URLLoader::Load(const GURL& url, const HttpRequestHeaders& extra_headers) {
    URLLoaderResult result;
    result.net_error = NET_OK;
    result.http_status_code = 0;
    result.response_body = "";
    result.final_url = url.spec();
    result.from_cache = false;

    if (!url.is_valid()) {
        result.net_error = ERR_INVALID_URL;
        return result;
    }

    // 1. Check HTTP cache first
    if (cache_) {
        HttpCacheEntry cache_entry;
        if (cache_->Lookup(url.spec(), &cache_entry)) {
            // Cache hit — check if revalidation is needed
            if (!cache_->NeedsRevalidation(cache_entry)) {
                result.http_status_code = cache_entry.status_code;
                result.response_headers = HttpResponseHeaders(cache_entry.response_headers_raw);
                result.response_body = cache_entry.response_body;
                result.from_cache = true;
                return result;
            }
            // Needs revalidation — add conditional headers
            // (Fall through to network with If-None-Match / If-Modified-Since)
        }
    }

    // 2. Follow redirects
    GURL current_url = url;
    HttpRequestHeaders headers = extra_headers;
    int redirect_count = 0;

    while (redirect_count < kMaxRedirects) {
        // Inject cookies into the request
        if (cookie_store_) {
            std::string cookie_header = cookie_store_->GetCookieHeaderForURL(current_url);
            if (!cookie_header.empty()) {
                headers.SetHeader("Cookie", cookie_header);
            }
        }

        // 3. Dispatch to ATOMS network adapter
        result = DoSingleRequest(current_url, headers);

        if (result.net_error != NET_OK) return result;

        // 4. Process Set-Cookie headers
        if (cookie_store_) {
            std::vector<std::string> set_cookies = result.response_headers.GetSetCookieHeaders();
            for (size_t i = 0; i < set_cookies.size(); i++) {
                cookie_store_->SetCookie(current_url, set_cookies[i]);
            }
        }

        // 5. Handle redirects
        if (result.response_headers.IsRedirect()) {
            std::string location;
            if (result.response_headers.GetLocationHeader(&location)) {
                // Handle absolute and relative redirects
                if (location.find("://") != (size_t)-1) {
                    current_url = GURL(location);
                } else {
                    // Relative redirect
                    current_url = GURL(current_url.scheme() + "://" + current_url.host() + location);
                }
                redirect_count++;
                result.final_url = current_url.spec();
                headers.RemoveHeader("Cookie"); // Will be re-injected
                continue;
            }
        }

        // 6. Cache the response
        if (cache_ && result.http_status_code == 200) {
            std::string etag_val;
            result.response_headers.GetNormalizedHeader("ETag", &etag_val);
            std::string lm_val;
            result.response_headers.GetNormalizedHeader("Last-Modified", &lm_val);
            cache_->Store(current_url.spec(), result.http_status_code,
                         result.response_headers.raw_headers(),
                         result.response_body, etag_val, lm_val);
        }

        break;
    }

    if (redirect_count >= kMaxRedirects) {
        result.net_error = ERR_TOO_MANY_REDIRECTS;
    }

    return result;
}

URLLoaderResult URLLoader::DoSingleRequest(const GURL& url, const HttpRequestHeaders& headers) {
    return AtomsNetworkAdapter_Fetch(url, headers);
}

} // namespace net
