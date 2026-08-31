/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_BINDINGS_CORE_V8_SCRIPT_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_BINDINGS_CORE_V8_SCRIPT_CONTROLLER_H_

#include "userspace/runtime/cpp/include/string"

namespace blink {

class Document;

class ScriptController {
public:
    explicit ScriptController(Document* document);
    ~ScriptController();

    bool executeScript(const std::string& script_source);

private:
    Document* document_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_BINDINGS_CORE_V8_SCRIPT_CONTROLLER_H_
