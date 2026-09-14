# ATOMS OS — Chromium Source License & Provenance Audit

> **Document ID:** ATOMS-CHROMIUM-LIC-001  
> **Repository:** `d:\Signatures_OS\third_party\chromium\src\`  
> **Source Origin:** Official Google Chromium Open Source Project (`chromium.googlesource.com`)  
> **Classification:** Engineering Provenance Audit & Intellectual Property Registry  
> **Date:** September 7, 2026  

---

## 1. Primary Chromium License

The core Chromium source tree acquired at `d:\Signatures_OS\third_party\chromium\src\` is governed by the **3-Clause BSD License**:

```text
// Copyright 2015 The Chromium Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
```

- **File Preserved:** `d:\Signatures_OS\third_party\chromium\src\LICENSE`
- **License Type:** Permissive Open Source (BSD 3-Clause)
- **Derivative Works Allowed:** YES
- **Commercial Distribution Allowed:** YES
- **Patent Retaliation Clause:** Standard BSD (No viral copyleft)

---

## 2. Dependency Provenance & Multi-License Matrix

Chromium incorporates third-party libraries via its `DEPS` specification. Each subsystem carries distinct, independent licenses that must be preserved and attributed:

| Subsystem / Library | Upstream Home | Primary License | Redistribution Requirement | Impact on ATOMS OS / ATRIX |
| :--- | :--- | :--- | :--- | :---: |
| **Chromium Core (`base`, `net`, `mojo`, `build`)** | `chromium.googlesource.com` | **BSD 3-Clause** | Retain copyright notice & disclaimer in documentation. | **100% Compatible** |
| **V8 JavaScript Engine** | `chromium.googlesource.com/v8/v8` | **BSD 3-Clause** | Retain V8 copyright notice & disclaimer. | **100% Compatible** |
| **Blink Rendering Engine** | `chromium.googlesource.com/chromium/src/third_party/blink` | **BSD 3-Clause** | Retain Blink copyright notices. | **100% Compatible** |
| **Skia 2D Graphics** | `skia.googlesource.com/skia` | **BSD 3-Clause** | Retain Skia copyright notices. | **100% Compatible** |
| **BoringSSL** | `boringssl.googlesource.com/boringssl` | **OpenSSL / ISC** | Retain original OpenSSL and SSLeay acknowledgments. | **100% Compatible** |
| **FreeType Font Engine**| `freetype.org` | **FTL (FreeType License)** | Attribution in documentation (`Portions of this software are copyright © The FreeType Project`). | **100% Compatible** |
| **HarfBuzz Text Shaper**| `github.com/harfbuzz/harfbuzz` | **Old MIT License** | Retain permission and copyright notice. | **100% Compatible** |
| **ICU (icu4c)** | `icu.unicode.org` | **Unicode-DFS-2016** | Retain Unicode copyright notice and terms of use. | **100% Compatible** |
| **dav1d (AV1 Decoder)** | `code.videolan.org/videolan/dav1d` | **BSD 2-Clause** | Retain copyright notice & disclaimer. | **100% Compatible** |
| **libvpx (VP8/VP9)** | `chromium.googlesource.com/webm/libvpx` | **BSD 3-Clause** | Retain WebM copyright notices. | **100% Compatible** |
| **libopus (Audio)** | `opus-codec.org` | **BSD 3-Clause** | Retain Opus / Xiph.Org copyright notices. | **100% Compatible** |
| **musl libc (Userspace)**| `musl.libc.org` | **Standard MIT** | Retain MIT copyright notice in binary distribution. | **100% Compatible** |
| **LLVM libc++ / ABI** | `github.com/llvm/llvm-project` | **Apache 2.0 with LLVM Exception** | Allows static linking into proprietary/custom binaries without copyleft. | **100% Compatible** |

---

## 3. Strict License Non-Contamination Rules for ATOMS OS

To protect ATOMS OS from intellectual property contamination and legal exposure:
1. **Zero Copyleft (GPL) Contamination:**
   No GPLv2, GPLv3, or AGPL code is permitted into the ATRIX browser binary or the BOS kernel. All proposed open-source components are strictly **permissive** (BSD, MIT, Apache 2.0 with exception, FTL).
2. **Preservation of Upstream Attribution:**
   When ATRIX Browser ships on ATOMS OS, an `about:credits` or `chrome://credits` internal page will automatically aggregate and render all upstream `LICENSE` files, satisfying Section 2 of the BSD, MIT, and Apache licenses.
3. **No Patent or Trademark Infringement:**
   The browser binary is branded **ATRIX Browser on ATOMS OS**. Google trademarks ("Chrome", "Google Chrome", Chrome logo) are **NOT** used, complying with Section 3 of the BSD license.
