/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "text.h"

namespace blink {

Text::Text(Document* document, const std::string& data)
    : Node(document, kTextNode)
    , data_(data)
{
}

} // namespace blink
