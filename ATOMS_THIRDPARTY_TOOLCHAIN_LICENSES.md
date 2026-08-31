# ATOMS OS: THIRD-PARTY TOOLCHAIN LICENSES & ATTRIBUTIONS

**Document ID:** ATRIX-PHASE8-LICENSES-001  
**Phase:** Phase 8 — Toolchain Licensing & Legal Notices  
**Date:** 2026-08-26  

---

## 1. Third-Party Toolchain & Build Components

The Phase 8 build infrastructure utilizes and adapts the following upstream open-source tools and specifications:

---

### 1.1 Chromium GN (Generate Ninja)
- **Project:** GN (Generate Ninja)
- **Source:** https://gn.googlesource.com/gn
- **Original Authors:** The Chromium Authors / Google LLC
- **License:** **BSD 3-Clause License**
- **License Notice:**
```text
// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
   * Neither the name of Google LLC nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

### 1.2 Ninja Build Tool
- **Project:** Ninja
- **Source:** https://ninja-build.org / https://github.com/ninja-build/ninja
- **Original Author:** Evan Martin and contributors
- **License:** **Apache 2.0 License**
- **License Summary:** Free for commercial and non-commercial use, redistribution, and modification without proprietary relicensing requirements.

---

### 1.3 LLVM / Clang Toolchain
- **Project:** The LLVM Project (Clang, LLD, llvm-ar)
- **Source:** https://llvm.org / https://github.com/llvm/llvm-project
- **Original Authors:** The LLVM Project / University of Illinois
- **License:** **Apache 2.0 with LLVM Exceptions**

---

### 1.4 Chromium Base & Abseil Primitives
- **Projects:** Chromium `base` library & Google Abseil C++ Common Libraries
- **Licenses:** **BSD 3-Clause / Apache 2.0**
- **Provenance:** Reference implementations adapted for ATOMS OS userspace compilation.
