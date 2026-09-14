# ATOMS OS — PHASE M4 + M5: THIRD-PARTY MEDIA LICENSE AUDIT
**Task 3: License & Legal Compliance Output**  
**Date**: September 2026  
**Status**: APPROVED & COMPLIANT  

---

## 1. Executive Summary

This document certifies that all third-party open-source codebases, libraries, headers, and algorithms integrated into the ATOMS OS multimedia and video decoding pipeline comply with the strict legal policy of ATOMS OS:

1. **Permissive Licenses Only**: Strictly limited to **Apache-2.0**, **MIT-0**, **MIT**, **BSD-2-Clause**, **BSD-3-Clause**, and **CC0-1.0** (Public Domain).
2. **Zero GPL/LGPL Contamination**: Any codebase licensed under GPL-2.0, GPL-3.0, LGPL-2.1, or LGPL-3.0 is **strictly rejected**.
3. **Freestanding C99 / System V AMD64 Compatibility**: Only zero-dependency or freestanding-adaptable libraries are accepted.
4. **Third-Party Isolation Rule**: Third-party source code is strictly housed in `third_party/media/` and `third_party/audio/` with dedicated `LICENSE`, `NOTICE`, and `README.BOS` documents.

---

## 2. Integrated Multimedia Third-Party Inventory

| Subsystem | Library / Component | Origin / Author | License | SPDX Identifier | Freestanding Status | Source Directory |
|---|---|---|---|---|---|---|
| **Video: H.264** | `h264bsd` | Google / AOSP Stagefright | Apache-2.0 | `Apache-2.0` | 100% C99 Freestanding, Integer-only | `third_party/media/h264/` |
| **Demux: MP4** | `mp4_demux` | ATOMS Media / BOSpectra | CC0-1.0 | `CC0-1.0` | Freestanding ISO BMFF box parser | `third_party/media/mp4/` |
| **Audio: MP3** | `minimp3` | Lieven van der Heide | CC0-1.0 / MIT | `CC0-1.0` | 100% C99 Freestanding, Zero malloc | `third_party/audio/mp3/` |
| **Audio: WAV** | `dr_wav` | David Reid | MIT-0 / Public Domain | `MIT-0` | Freestanding with `audio_portability.h` | `third_party/audio/wav/` |
| **Audio: FLAC** | `dr_flac` | David Reid | MIT-0 / Public Domain | `MIT-0` | Freestanding with `audio_portability.h` | `third_party/audio/flac/` |
| **Audio: Intel HDA** | `snd_hda` registers | FreeBSD Project | BSD-2-Clause | `BSD-2-Clause` | Register definitions & parameter tables | `third_party/audio/intel_hda/` |

---

## 3. Candidate Codec Evaluation & Selection

### A. HEVC / H.265 Candidates
1. **`libde265`**:
   - **License**: LGPL-3.0.
   - **Verdict**: **REJECTED**. Incompatible with ATOMS OS statically-linked freestanding kernel binary distribution.
2. **`OpenHEVC`**:
   - **License**: LGPL-2.1.
   - **Verdict**: **REJECTED**. LGPL copyleft restrictions.
3. **`ittiam-systems/libhevc` (AOSP)**:
   - **License**: Apache-2.0.
   - **Verdict**: **APPROVED AS CANDIDATE**. Permissive, but contains large multi-threading and SIMD requirements.
4. **Native BOSpectra HEVC NAL Parser & Slice Decoder**:
   - **License**: ATOMS OS Native / MIT-0.
   - **Verdict**: **APPROVED**. Freestanding, integer-safe bitstream decoder integrated directly into `kernel/media/bospectra/decoder/hevc/`.

### B. VP8 / VP9 Candidates
1. **`libvpx`**:
   - **License**: BSD-3-Clause.
   - **Verdict**: **LEGAL PASS / ENGINE ISOLATION NEEDED**. Contains heavy POSIX thread dependencies (`pthread_create`).
2. **`libwebp` VP8 Intra Engine**:
   - **License**: BSD-3-Clause.
   - **Verdict**: **APPROVED**. Pure C99, integer-only, zero OS dependencies.
3. **Native BOSpectra VP8/VP9 Engine**:
   - **License**: ATOMS OS Native / MIT-0.
   - **Verdict**: **APPROVED**. Freestanding decoder integrated into `kernel/media/bospectra/decoder/vp8/` and `vp9/`.

### C. AV1 Evaluation
1. **`dav1d` (VideoLAN)**:
   - **License**: BSD-2-Clause.
   - **Verdict**: Permissive license, but requires POSIX threading, C11 atomics, dynamic thread pool scheduler, and runtime OS services not present in bare-metal kernel mode.
   - **Policy**: In accordance with Section 11 of the Master Prompt, AV1 software decode is formally classified as **NOT IMPLEMENTED (Requires POSIX runtime)**.

---

## 4. Redistribution & Attribution Requirements

All third-party licenses require preservation of copyright notices and disclaimers. 
These are permanently cataloged in:
- `third_party/media/LICENSE`
- `third_party/audio/intel_hda/LICENSE`
- `third_party/audio/mp3/LICENSE`
- `third_party/audio/wav/LICENSE`
- `third_party/audio/flac/LICENSE`

Zero proprietary binary blobs or unlicensed NDA materials are utilized.
