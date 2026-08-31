/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "security_headers.h"
#include "userspace/runtime/c/include/stdlib.h"
#include "userspace/runtime/c/include/string.h"

namespace net {

static std::string ToLower(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); i++) {
        if (out[i] >= 'A' && out[i] <= 'Z') {
            out[i] = (char)(out[i] + ('a' - 'A'));
        }
    }
    return out;
}

XFrameOptionsValue SecurityHeaders::ParseXFrameOptions(const std::string& header_val) {
    std::string val = ToLower(header_val);
    if (val == "deny") return XFO_DENY;
    if (val == "sameorigin") return XFO_SAMEORIGIN;
    if (val == "allowall" || val == "allow-from") return XFO_ALLOWALL;
    if (val.empty()) return XFO_NONE;
    return XFO_INVALID;
}

bool SecurityHeaders::CanFrameDocument(XFrameOptionsValue xfo, const SecurityOrigin& child_origin, const SecurityOrigin& parent_origin) {
    if (xfo == XFO_NONE || xfo == XFO_ALLOWALL) return true;
    if (xfo == XFO_DENY) return false;
    if (xfo == XFO_SAMEORIGIN) {
        return child_origin.IsSameOriginWith(parent_origin);
    }
    return false;
}

bool SecurityHeaders::ParseHSTS(const std::string& header_val, uint64_t* out_max_age, bool* out_include_subdomains) {
    if (header_val.empty()) return false;
    std::string val = ToLower(header_val);

    size_t ma_pos = val.find("max-age=");
    if (ma_pos == std::string::npos) return false;

    size_t num_start = ma_pos + 8;
    size_t num_end = val.find(';', num_start);
    if (num_end == std::string::npos) num_end = val.size();

    std::string num_str = val.substr(num_start, num_end - num_start);
    uint64_t age = 0;
    for (size_t i = 0; i < num_str.size(); i++) {
        if (num_str[i] >= '0' && num_str[i] <= '9') {
            age = age * 10 + (uint64_t)(num_str[i] - '0');
        } else {
            break;
        }
    }

    if (out_max_age) *out_max_age = age;
    if (out_include_subdomains) {
        *out_include_subdomains = (val.find("includesubdomains") != std::string::npos);
    }
    return true;
}

ReferrerPolicyValue SecurityHeaders::ParseReferrerPolicy(const std::string& header_val) {
    std::string val = ToLower(header_val);
    if (val == "no-referrer") return REFERRER_POLICY_NO_REFERRER;
    if (val == "same-origin") return REFERRER_POLICY_SAME_ORIGIN;
    if (val == "origin") return REFERRER_POLICY_ORIGIN;
    if (val == "strict-origin") return REFERRER_POLICY_STRICT_ORIGIN;
    if (val == "strict-origin-when-cross-origin") return REFERRER_POLICY_STRICT_ORIGIN_WHEN_CROSS_ORIGIN;
    if (val == "unsafe-url") return REFERRER_POLICY_UNSAFE_URL;
    return REFERRER_POLICY_DEFAULT;
}

std::string SecurityHeaders::ComputeReferrer(ReferrerPolicyValue policy, const GURL& current_url, const GURL& destination_url) {
    if (!current_url.is_valid() || !destination_url.is_valid()) return "";
    if (policy == REFERRER_POLICY_NO_REFERRER) return "";

    bool is_https_downgrade = (current_url.scheme() == "https" && destination_url.scheme() == "http");
    if (is_https_downgrade && (policy == REFERRER_POLICY_STRICT_ORIGIN ||
                               policy == REFERRER_POLICY_STRICT_ORIGIN_WHEN_CROSS_ORIGIN ||
                               policy == REFERRER_POLICY_DEFAULT)) {
        return "";
    }

    bool is_same_origin = (current_url.scheme() == destination_url.scheme() &&
                           current_url.host() == destination_url.host() &&
                           current_url.port() == destination_url.port());

    if (policy == REFERRER_POLICY_SAME_ORIGIN) {
        return is_same_origin ? current_url.spec() : "";
    }

    if (policy == REFERRER_POLICY_ORIGIN || policy == REFERRER_POLICY_STRICT_ORIGIN) {
        return current_url.scheme() + "://" + current_url.host() + "/";
    }

    if (policy == REFERRER_POLICY_STRICT_ORIGIN_WHEN_CROSS_ORIGIN || policy == REFERRER_POLICY_DEFAULT) {
        if (is_same_origin) {
            return current_url.spec();
        } else {
            return current_url.scheme() + "://" + current_url.host() + "/";
        }
    }

    if (policy == REFERRER_POLICY_UNSAFE_URL) {
        return current_url.spec();
    }

    return current_url.spec();
}

} // namespace net
