/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_BASE_CONTENT_SECURITY_POLICY_H_
#define THIRD_PARTY_CHROMIUM_NET_BASE_CONTENT_SECURITY_POLICY_H_

#include "gurl.h"
#include "security_origin.h"
#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"

namespace net {

enum CSPDirectiveType {
    CSP_DIRECTIVE_DEFAULT_SRC = 0,
    CSP_DIRECTIVE_SCRIPT_SRC  = 1,
    CSP_DIRECTIVE_STYLE_SRC   = 2,
    CSP_DIRECTIVE_IMG_SRC     = 3,
    CSP_DIRECTIVE_FRAME_ANCESTORS = 4,
    CSP_DIRECTIVE_CONNECT_SRC = 5,
    CSP_DIRECTIVE_UNKNOWN     = 6
};

struct CSPSource {
    bool is_self;
    bool is_unsafe_inline;
    bool is_none;
    bool allow_all;
    std::string scheme;
    std::string host;
    uint16_t port;
};

struct CSPDirective {
    CSPDirectiveType type;
    std::vector<CSPSource> sources;
};

class ContentSecurityPolicy {
public:
    ContentSecurityPolicy();
    ~ContentSecurityPolicy();

    static ContentSecurityPolicy Parse(const std::string& header_value, const SecurityOrigin& self_origin);

    bool AllowScriptFromSource(const GURL& url, bool is_inline) const;
    bool AllowStyleFromSource(const GURL& url, bool is_inline) const;
    bool AllowImageFromSource(const GURL& url) const;
    bool AllowFrameAncestors(const SecurityOrigin& parent_origin) const;
    bool AllowConnection(const GURL& url) const;

    bool is_active() const { return is_active_; }
    size_t directive_count() const { return directives_.size(); }

private:
    bool CheckSourceList(CSPDirectiveType type, const GURL& url, bool is_inline) const;
    const CSPDirective* FindDirective(CSPDirectiveType type) const;

    std::vector<CSPDirective> directives_;
    SecurityOrigin self_origin_;
    bool is_active_;
};

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_BASE_CONTENT_SECURITY_POLICY_H_
