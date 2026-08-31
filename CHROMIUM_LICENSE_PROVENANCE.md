# CHROMIUM & THIRD-PARTY OPEN-SOURCE LICENSE & PROVENANCE AUDIT

**Document ID:** ATRIX-PHASE6-LICENSE-001  
**Phase:** Phase 6 — Open-Source Legal & Licensing Provenance Audit  
**Date:** 2026-08-26  

---

## 1. Third-Party Component Licensing Audit

The Chromium project and all required embedded subsystems are distributed under exceptionally permissive, commercially and non-commercially friendly open-source licenses:

| Component | Upstream Copyright Holder | Primary License | Redistribution Obligations | GPL / Copyleft Risk |
|:---|:---|:---|:---|:---|
| **The Chromium Project** | The Chromium Authors / Google LLC | **BSD 3-Clause** | Retain copyright notice & license text in binaries/docs. | **NONE (Permissive)** |
| **Blink Web Engine** | The Chromium Authors / Apple Inc. | **BSD 3-Clause / LGPLv2.1 (historical portions)** | Retain copyright notices. Ensure no GPL-tainted WebKit forks are vendored. | **VERY LOW** |
| **V8 JavaScript Engine** | The V8 Project Authors / Google LLC | **BSD 3-Clause** | Retain copyright notice & license disclaimer. | **NONE (Permissive)** |
| **Skia 2D Graphics** | Google LLC | **BSD 3-Clause** | Retain copyright notice & license disclaimer. | **NONE (Permissive)** |
| **BoringSSL** | Google LLC / OpenSSL Project | **OpenSSL + ISC + BSD 3-Clause** | Include OpenSSL & BoringSSL attribution statements. | **NONE (Permissive)** |
| **ICU (Unicode)** | Unicode, Inc. | **Unicode-TOU / ICU License** | Include Unicode copyright and permission notices. | **NONE (Permissive)** |
| **HarfBuzz** | HarfBuzz Authors / Behdad Esfahbod | **Old MIT License** | Include HarfBuzz MIT license text. | **NONE (Permissive)** |
| **FreeType** | The FreeType Project / David Turner | **FreeType License (FTL) / BSD** | State that software is based in part on FreeType. | **NONE (Permissive)** |
| **libpng** | Glenn Randers-Pehrson | **libpng License** | Retain license disclaimer. | **NONE (Permissive)** |
| **libjpeg-turbo** | The libjpeg-turbo Project | **BSD 3-Clause / zlib** | Retain copyright notice. | **NONE (Permissive)** |
| **zlib** | Jean-loup Gailly and Mark Adler | **zlib License** | Retain copyright notice; do not misrepresent origin. | **NONE (Permissive)** |

---

## 2. Mandatory Provenance & Attribution Rules for ATOMS OS

1. **Explicit Authorship Attribution:**
   - Documentation must clearly distinguish between:
     - **ATOMS OS Kernel & Shell Code** (Authored by Saumya Chaudhari / ATOMS OS Team).
     - **Chromium / Blink / V8 / Skia Code** (Authored by The Chromium Authors & Google LLC).
     - **Third-Party Libraries** (Authored by respective upstream projects).
   - ATOMS OS will never falsely claim authorship of third-party Chromium components.

2. **Shipping License Bundle:**
   - A standard `about:credits` or `THIRD_PARTY_LICENSES` file will be generated and shipped alongside the ATRIX browser binary.

3. **No GPL / Copyleft Contamination:**
   - All components selected for ATOMS OS integration are strictly permissive (BSD, MIT, Apache 2.0, zlib).
   - No GPLv3 / AGPL components will be included, ensuring total architectural and licensing freedom for ATOMS OS.
