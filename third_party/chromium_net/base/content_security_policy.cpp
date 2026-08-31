/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "content_security_policy.h"
#include "userspace/runtime/c/include/ctype.h"
#include "userspace/runtime/c/include/string.h"

namespace net {

ContentSecurityPolicy::ContentSecurityPolicy()
    : is_active_(false) {}

ContentSecurityPolicy::~ContentSecurityPolicy() {}

static std::string TrimWhitespace(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start++;
    }
    if (start == s.size()) return "";
    size_t end = s.size() - 1;
    while (end > start && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r' || s[end] == '\n')) {
        end--;
    }
    return s.substr(start, end - start + 1);
}

ContentSecurityPolicy ContentSecurityPolicy::Parse(const std::string& header_value, const SecurityOrigin& self_origin) {
    ContentSecurityPolicy csp;
    csp.self_origin_ = self_origin;

    if (header_value.empty()) {
        csp.is_active_ = false;
        return csp;
    }

    csp.is_active_ = true;

    // Split on ';'
    size_t pos = 0;
    while (pos < header_value.size()) {
        size_t next_semi = header_value.find(';', pos);
        if (next_semi == std::string::npos) {
            next_semi = header_value.size();
        }

        std::string token = TrimWhitespace(header_value.substr(pos, next_semi - pos));
        pos = next_semi + 1;

        if (token.empty()) continue;

        // Extract directive name and source expressions
        size_t first_space = token.find(' ');
        std::string dir_name;
        std::string src_list_str;

        if (first_space == std::string::npos) {
            dir_name = token;
            src_list_str = "";
        } else {
            dir_name = token.substr(0, first_space);
            src_list_str = TrimWhitespace(token.substr(first_space + 1));
        }

        CSPDirective dir;
        if (dir_name == "default-src") dir.type = CSP_DIRECTIVE_DEFAULT_SRC;
        else if (dir_name == "script-src") dir.type = CSP_DIRECTIVE_SCRIPT_SRC;
        else if (dir_name == "style-src") dir.type = CSP_DIRECTIVE_STYLE_SRC;
        else if (dir_name == "img-src") dir.type = CSP_DIRECTIVE_IMG_SRC;
        else if (dir_name == "frame-ancestors") dir.type = CSP_DIRECTIVE_FRAME_ANCESTORS;
        else if (dir_name == "connect-src") dir.type = CSP_DIRECTIVE_CONNECT_SRC;
        else dir.type = CSP_DIRECTIVE_UNKNOWN;

        if (dir.type != CSP_DIRECTIVE_UNKNOWN) {
            // Parse sources
            size_t s_pos = 0;
            while (s_pos < src_list_str.size()) {
                size_t next_sp = src_list_str.find(' ', s_pos);
                if (next_sp == std::string::npos) {
                    next_sp = src_list_str.size();
                }

                std::string src_str = TrimWhitespace(src_list_str.substr(s_pos, next_sp - s_pos));
                s_pos = next_sp + 1;

                if (src_str.empty()) continue;

                CSPSource source;
                source.is_self = (src_str == "'self'");
                source.is_unsafe_inline = (src_str == "'unsafe-inline'");
                source.is_none = (src_str == "'none'");
                source.allow_all = (src_str == "*");

                if (!source.is_self && !source.is_unsafe_inline && !source.is_none && !source.allow_all) {
                    GURL u(src_str);
                    if (u.is_valid()) {
                        source.scheme = u.scheme();
                        source.host = u.host();
                        source.port = u.port();
                    } else {
                        source.host = src_str;
                        source.port = 0;
                    }
                }

                dir.sources.push_back(source);
            }

            csp.directives_.push_back(dir);
        }
    }

    return csp;
}

const CSPDirective* ContentSecurityPolicy::FindDirective(CSPDirectiveType type) const {
    for (size_t i = 0; i < directives_.size(); i++) {
        if (directives_[i].type == type) {
            return &directives_[i];
        }
    }
    return nullptr;
}

bool ContentSecurityPolicy::CheckSourceList(CSPDirectiveType type, const GURL& url, bool is_inline) const {
    if (!is_active_) return true;

    const CSPDirective* dir = FindDirective(type);
    if (!dir) {
        // Fall back to default-src
        dir = FindDirective(CSP_DIRECTIVE_DEFAULT_SRC);
    }

    if (!dir) return true; // No matching directive

    if (dir->sources.empty()) return false;

    for (size_t i = 0; i < dir->sources.size(); i++) {
        const CSPSource& src = dir->sources[i];

        if (src.is_none) return false;
        if (src.allow_all) return true;

        if (is_inline) {
            if (src.is_unsafe_inline) return true;
        } else {
            if (src.is_self) {
                if (url.scheme() == self_origin_.scheme() && url.host() == self_origin_.host()) {
                    return true;
                }
            } else if (!src.host.empty()) {
                if (url.host() == src.host) {
                    if (src.scheme.empty() || url.scheme() == src.scheme) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool ContentSecurityPolicy::AllowScriptFromSource(const GURL& url, bool is_inline) const {
    return CheckSourceList(CSP_DIRECTIVE_SCRIPT_SRC, url, is_inline);
}

bool ContentSecurityPolicy::AllowStyleFromSource(const GURL& url, bool is_inline) const {
    return CheckSourceList(CSP_DIRECTIVE_STYLE_SRC, url, is_inline);
}

bool ContentSecurityPolicy::AllowImageFromSource(const GURL& url) const {
    return CheckSourceList(CSP_DIRECTIVE_IMG_SRC, url, false);
}

bool ContentSecurityPolicy::AllowConnection(const GURL& url) const {
    return CheckSourceList(CSP_DIRECTIVE_CONNECT_SRC, url, false);
}

bool ContentSecurityPolicy::AllowFrameAncestors(const SecurityOrigin& parent_origin) const {
    if (!is_active_) return true;

    const CSPDirective* dir = FindDirective(CSP_DIRECTIVE_FRAME_ANCESTORS);
    if (!dir) return true;

    for (size_t i = 0; i < dir->sources.size(); i++) {
        const CSPSource& src = dir->sources[i];
        if (src.is_none) return false;
        if (src.allow_all) return true;
        if (src.is_self) {
            if (self_origin_.IsSameOriginWith(parent_origin)) return true;
        } else if (!src.host.empty()) {
            if (parent_origin.host() == src.host) return true;
        }
    }

    return false;
}

} // namespace net
