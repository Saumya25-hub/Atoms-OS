/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_BASE_SECURITY_HEADERS_H_
#define THIRD_PARTY_CHROMIUM_NET_BASE_SECURITY_HEADERS_H_

#include "security_origin.h"
#include "gurl.h"
#include "userspace/runtime/cpp/include/string"

namespace net {

enum XFrameOptionsValue {
    XFO_NONE = 0,
    XFO_DENY = 1,
    XFO_SAMEORIGIN = 2,
    XFO_ALLOWALL = 3,
    XFO_INVALID = 4
};

enum ReferrerPolicyValue {
    REFERRER_POLICY_DEFAULT = 0,
    REFERRER_POLICY_NO_REFERRER = 1,
    REFERRER_POLICY_SAME_ORIGIN = 2,
    REFERRER_POLICY_ORIGIN = 3,
    REFERRER_POLICY_STRICT_ORIGIN = 4,
    REFERRER_POLICY_STRICT_ORIGIN_WHEN_CROSS_ORIGIN = 5,
    REFERRER_POLICY_UNSAFE_URL = 6
};

class SecurityHeaders {
public:
    static XFrameOptionsValue ParseXFrameOptions(const std::string& header_val);
    static bool CanFrameDocument(XFrameOptionsValue xfo, const SecurityOrigin& child_origin, const SecurityOrigin& parent_origin);

    static bool ParseHSTS(const std::string& header_val, uint64_t* out_max_age, bool* out_include_subdomains);
    static ReferrerPolicyValue ParseReferrerPolicy(const std::string& header_val);
    static std::string ComputeReferrer(ReferrerPolicyValue policy, const GURL& current_url, const GURL& destination_url);
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_BASE_SECURITY_HEADERS_H_
