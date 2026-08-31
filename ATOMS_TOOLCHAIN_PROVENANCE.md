# ATOMS OS TOOLCHAIN PROVENANCE & SOURCE ATTRIBUTION

**Document ID:** ATRIX-PHASE8-PROVENANCE-001  
**Phase:** Phase 8 — Toolchain Source Provenance Record  
**Date:** 2026-08-26  

---

## 1. Toolchain Component Classification

| Toolchain Component | Implementation File(s) | Category | Upstream Project / Author | License |
|:---|:---|:---|:---|:---|
| **GN Engine Binary** | `tools/gn.exe` | **OPEN-SOURCE THIRD-PARTY** | The Chromium Authors / Google LLC | BSD 3-Clause |
| **Ninja Engine Binary** | `tools/ninja.exe` | **OPEN-SOURCE THIRD-PARTY** | Evan Martin / Ninja Project | Apache 2.0 |
| **GN Root Configuration** | `.gn` | **ATOMS INTEGRATION** | ATOMS OS Team | Proprietary / ATOMS |
| **GN Build Configuration** | `gn/BUILDCONFIG.gn` | **ADAPTED CODE** | Adapted from Chromium `build/config/BUILDCONFIG.gn` | BSD 3-Clause |
| **ATOMS Target Toolchain** | `gn/toolchains/atoms/BUILD.gn` | **ADAPTED CODE** | Adapted from Chromium GN toolchain conventions | BSD 3-Clause |
| **Host Toolchain** | `gn/toolchains/host/BUILD.gn` | **ADAPTED CODE** | Adapted from Chromium host toolchain definitions | BSD 3-Clause |
| **Compiler Flags Config** | `gn/config/BUILD.gn` | **ATOMS ORIGINAL / ADAPTED** | ATOMS OS Team / Chromium Project | BSD 3-Clause |
| **Root GN Targets** | `BUILD.gn` | **ATOMS ORIGINAL** | ATOMS OS Team | Proprietary / ATOMS |
| **GN Build Driver Script** | `tools/gn_build.ps1` | **ATOMS ORIGINAL** | ATOMS OS Team | Proprietary / ATOMS |
| **Host Code Generator** | `tools/codegen/test_code_generator.py` | **ATOMS ORIGINAL** | ATOMS OS Team | Proprietary / ATOMS |
| **Chromium Base Probe** | `third_party/chromium_probe/` | **ADAPTED CODE** | The Chromium Authors / Google LLC | BSD 3-Clause |
| **Abseil Status/Span Probe** | `third_party/chromium_probe/` | **ADAPTED CODE** | Abseil Authors / Google LLC | Apache 2.0 |
