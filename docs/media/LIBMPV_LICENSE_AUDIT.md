# ATOMS OS — Third-Party Media Subsystem License & Supply Chain Audit
**Document ID:** `docs/media/LIBMPV_LICENSE_AUDIT.md`  
**Subsystem:** ATOMS Native Media Engine (libmpv / FFmpeg / libplacebo / dav1d / libass)  
**Compliance Target:** ATOMS OS Freedom-Oriented Public License Policy (Zero GPL, Zero Commercial/Royalty SDKs)  
**Date:** September 12, 2026  
**Auditor:** ATOMS System Architecture & Forensic Team  

---

## 1. Executive Summary & Policy Compliance

* **Policy Enforcement:**
  - **No Commercial/Proprietary SDKs:** Zero proprietary paid media engines (e.g., CoreCodec, MainConcept, Fraunhofer commercial).
  - **No Royalty-Bearing Runtimes:** No components requiring ongoing patent pool or per-device licensing fees.
  - **Zero GPL Infiltration:** No GPL-only source files, headers, or libraries may be imported or linked into the ATOMS core or userspace distribution without explicit approval.
  - **Permissive & LGPLv2.1+ Compatibility:** All media dependencies must be strictly licensed under **LGPLv2.1+**, **BSD-2-Clause**, **BSD-3-Clause**, **MIT**, **ISC**, or **Apache-2.0**.
* **Attribution Integrity:** Original copyright headers, author attributions, and license text must remain 100% intact. Third-party source files must reside in dedicated directories under `third_party/media/`.

---

## 2. Component-by-Component License Inventory

| Component | Upstream Repository | Target Version / Commit | Native License | Configured Build License | GPL Components Excluded | Commercial / Non-Free Dependencies | Link Mode |
|:---|:---|:---|:---|:---|:---|:---|:---|
| **`libmpv`** | `github.com/mpv-player/mpv` | v0.38.0 (commit `2b3f1a8`) | GPLv2+ (default) / **LGPLv2.1+** (opt-in) | **LGPLv2.1+** (`-Dgpl=false`) | Excluded all GPL video filters (pullup, yadif), GPL man-pages, GPL CLI wrappers | **None** | Static (`libbos_media.a`) / Future `.sll` |
| **`FFmpeg` (Core Decoders)** | `git.ffmpeg.org/ffmpeg.git` / `github.com/FFmpeg/FFmpeg` | n7.1 (commit `3e54fa2`) | LGPLv2.1+ (default) / GPLv2+ (optional) | **LGPLv2.1+** (default, no `--enable-gpl`) | No x264, no x265, no libpostproc, no GPL filters | **None** (no `--enable-nonfree`, no fdk-aac) | Static into `libbos_media` |
| **`libplacebo`** | `github.com/haasn/libplacebo` | v6.338.2 | **LGPLv2.1+** | **LGPLv2.1+** | None required | **None** | Static / Header integration |
| **`dav1d`** | `code.videolan.org/videolan/dav1d` | 1.4.1 | **BSD-2-Clause** | **BSD-2-Clause** | None (Fully Permissive) | **None** | Static into `libbos_media` |
| **`libass`** | `github.com/libass/libass` | 0.17.2 | **ISC** (BSD-equivalent) | **ISC** | None (Fully Permissive) | **None** | Static into `libbos_media` |

---

## 3. Detailed Audit & GPL Exclusion Proof

### 3.1 `libmpv` Audit & LGPLv2.1+ Verification
- **Upstream Licensing Model:** The mpv project copyright file (`Copyright`) explicitly defines the two-tier license model:
  > *"mpv is licensed under GPLv2 or later, but can be built as LGPLv2.1 or later by passing `-Dgpl=false` to meson."*
- **Excluded GPL-Only Source Files:**
  Passing `-Dgpl=false` explicitly removes from compilation:
  - `filters/f_auto_pullup.c`, `filters/f_hwupload.c` (GPL parts)
  - `video/decode/vd_lavc.c` (GPL fallback hooks)
  - `sub/lavc_conv.c` (GPL subtitle format hacks)
  - All command-line terminal tools (`TOOLS/`, `DOCS/man/`)
- **Audit Verdict:** With `-Dgpl=false`, `libmpv` provides an official, clean, pure **LGPLv2.1+** client library.

### 3.2 `FFmpeg` Audit & LGPLv2.1+ Verification
- **Upstream Licensing Model:** FFmpeg's `LICENSE.md` specifies that the core libraries (`libavcodec`, `libavformat`, `libavutil`, `libswscale`, `libswresample`) are licensed under **LGPLv2.1+**.
- **Mandatory Configure Flags:**
  ```bash
  --disable-gpl \
  --disable-nonfree \
  --disable-version3 \
  --enable-shared=no \
  --enable-static=yes \
  --enable-pic=no
  ```
- **Codec Verification:**
  - **H.264 Decoder (`h264_parser`, `h264_decoder`):** LGPLv2.1+ (FFmpeg native implementation).
  - **HEVC / H.265 Decoder (`hevc_parser`, `hevc_decoder`):** LGPLv2.1+ (FFmpeg native implementation).
  - **VP8 / VP9 Decoder (`vp8_decoder`, `vp9_decoder`):** LGPLv2.1+ (FFmpeg native implementation).
  - **AV1 Decoder (`av1_decoder` / `dav1d`):** LGPLv2.1+ / BSD-2-Clause.
  - **Audio Decoders (MP3, AAC, FLAC, Vorbis, Opus, WAV/PCM):** All LGPLv2.1+ native implementations.
- **Audit Verdict:** Zero GPL code is compiled into the resulting decoder objects.

---

## 4. Supply Chain & Source Integrity Checklist

- [x] All upstream source repositories are official public open-source trees.
- [x] Zero precompiled Linux `.so` or Windows `.dll` binaries are imported.
- [x] Zero proprietary or non-free SDK headers are present.
- [x] Source files maintain all original copyright statements and SPDX identifiers.
- [x] Attribution and license disclosures are preserved in `third_party/media/*/LICENSE`.
