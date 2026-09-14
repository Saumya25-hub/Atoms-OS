# ATOMS OS — PHASE 1 SOURCE PROVENANCE & LICENSE AUDIT
**Subsystem:** Media Engine Source Provenance  
**Milestone:** Phase 1 (Kernel Media ➔ Ring-3 Userspace Migration)  
**Date:** September 12, 2026  
**Status:** **VERIFIED & COMPLIANT (RULE 2 ENFORCED)**  

---

## 1. LICENSING DIRECTIVE & INTEGRITY POLICY

In strict compliance with **Rule 2** of the ATOMS OS Media Engine Specification:
1. **Zero GPL Code:** No GPL-v1, GPL-v2, GPL-v3, or AGPL code is permitted in the ATOMS production media stack.
2. **Zero Non-Free Code:** Proprietary, royalty-bearing, or non-commercial binary blobs are strictly prohibited.
3. **Allowed Permissive & Weak-Copyleft Licenses:**
   - **MIT / Expat**
   - **BSD-2-Clause / BSD-3-Clause**
   - **ISC**
   - **Apache-2.0**
   - **CC0 / Public Domain**
   - **LGPL-2.1-or-later** (used modularly with clear attribution, without GPL components enabled)

---

## 2. AUDITED THIRD-PARTY MEDIA SOURCES

### 2.1 Android Stagefright H.264 Baseline Decoder (`h264bsd`)
* **Project Name:** Android Open Source Project (AOSP) Stagefright / Hantro G1 Software Decoder
* **Upstream Repository:** `https://android.googlesource.com/platform/frameworks/av/`
* **Target Version / Release:** Android 2.3 (Gingerbread) / Standalone port
* **Exact Files Included:**
  - `third_party/media/h264/src/h264bsd_byte_stream.c`
  - `third_party/media/h264/src/h264bsd_cavlc.c`
  - `third_party/media/h264/src/h264bsd_conceal.c`
  - `third_party/media/h264/src/h264bsd_deblocking.c`
  - `third_party/media/h264/src/h264bsd_decoder.c`
  - `third_party/media/h264/src/h264bsd_dpb.c`
  - `third_party/media/h264/src/h264bsd_image.c`
  - `third_party/media/h264/src/h264bsd_inter_prediction.c`
  - `third_party/media/h264/src/h264bsd_intra_prediction.c`
  - `third_party/media/h264/src/h264bsd_macroblock_layer.c`
  - `third_party/media/h264/src/h264bsd_nal_unit.c`
  - `third_party/media/h264/src/h264bsd_neighbour.c`
  - `third_party/media/h264/src/h264bsd_pic_order_cnt.c`
  - `third_party/media/h264/src/h264bsd_pic_param_set.c`
  - `third_party/media/h264/src/h264bsd_reconstruct.c`
  - `third_party/media/h264/src/h264bsd_sei.c`
  - `third_party/media/h264/src/h264bsd_seq_param_set.c`
  - `third_party/media/h264/src/h264bsd_slice_data.c`
  - `third_party/media/h264/src/h264bsd_slice_group_map.c`
  - `third_party/media/h264/src/h264bsd_slice_header.c`
  - `third_party/media/h264/src/h264bsd_storage.c`
  - `third_party/media/h264/src/h264bsd_stream.c`
  - `third_party/media/h264/src/h264bsd_transform.c`
  - `third_party/media/h264/src/h264bsd_util.c`
  - `third_party/media/h264/src/h264bsd_vlc.c`
  - `third_party/media/h264/src/h264bsd_vui.c`
  - `third_party/media/h264/include/*.h`
* **Original License:** **Apache License 2.0**
* **Modifications in ATOMS:**
  - Integrated freestanding headers (replacing standard libc `<assert.h>` and `<stdio.h>`).
  - Added High Profile SPS/PPS syntax parsing in `h264bsd_seq_param_set.c` and `h264bsd_pic_param_set.c`.
  - Added CABAC macroblock hook in `h264bsd_slice_data.c`.
* **Attribution Requirement:** Apache 2.0 attribution preserved in all file headers.

---

### 2.2 minimp4 ISO Base Media File Format Demuxer
* **Project Name:** minimp4
* **Upstream Repository:** `https://github.com/lieff/minimp4`
* **Author:** Dmitry Boldyrev
* **Original License:** **CC0 1.0 Universal / Public Domain**
* **Exact Files Included:**
  - `third_party/media/mp4/include/mp4_demux.h`
  - `third_party/media/mp4/src/mp4_demux.c`
* **Modifications in ATOMS:**
  - Adapted memory allocation calls to use freestanding string/memory routines.
  - Added 64-bit chunk offset (`co64`) parsing and custom stream callbacks.
* **Attribution Requirement:** None legally required (Public Domain / CC0), but author acknowledged in documentation.

---

### 2.3 FFmpeg `libavcodec` Arithmetic CABAC Tables
* **Project Name:** FFmpeg Project (`libavcodec`)
* **Upstream Repository:** `https://git.ffmpeg.org/ffmpeg.git`
* **Target Version:** FFmpeg n6.1 / n7.0
* **Original License:** **GNU Lesser General Public License version 2.1 or later (LGPL v2.1+)**
* **Exact Files Adapted:**
  - `third_party/media/h264/src/h264bsd_cabac.c` (State tables `ff_h264_cabac_tables`, renormalization, and bypass decoding derived from `libavcodec/cabac.c` and `libavcodec/h264_cabac.c`).
  - `third_party/media/h264/include/h264bsd_cabac.h`
* **Modifications in ATOMS:**
  - Pure integer arithmetic context; GPL extensions (`--enable-gpl`) completely omitted.
* **Attribution Requirement:** Notice of LGPL v2.1+ included in `third_party/media/LICENSE`.

---

### 2.4 dr_libs Audio Decoders (`dr_wav`, `dr_mp3`, `dr_flac`)
* **Project Name:** dr_libs
* **Author:** David Reid
* **Upstream Repository:** `https://github.com/mackron/dr_libs`
* **Original License:** **MIT / Public Domain (Unlicense)**
* **Exact Files Included:**
  - Header-only decoders in `kernel/audio/codecs/` and `userspace/libbos_media/`
* **Modifications in ATOMS:**
  - Bound to ATOMS VFS stream readers.
* **Attribution Requirement:** Author copyright preserved in headers.

---

### 2.5 libmpv Client API Headers
* **Project Name:** mpv Media Player
* **Upstream Repository:** `https://github.com/mpv-player/mpv`
* **Target Version:** v0.38.0 (`2b3f1a8c9e46a784d59fcf533b66e3957cecb8ff`)
* **Original License:** LGPL v2.1+ (configured via `-Dgpl=false`)
* **Exact Files Included:**
  - `third_party/media/mpv/include/mpv/client.h`
  - `third_party/media/mpv/include/mpv/render.h`
  - `third_party/media/mpv/include/mpv/stream_cb.h`
  - `third_party/media/mpv/LICENSE`
  - `third_party/media/mpv/COPYRIGHT`
* **Modifications in ATOMS:**
  - Headers only; no upstream GPL code imported.
* **Attribution Requirement:** LGPL v2.1+ license file maintained in directory.

---

## 3. PROVENANCE VERDICT

All third-party media software imported into ATOMS OS adheres strictly to permissive (Apache 2.0, MIT, Public Domain/CC0) or weak-copyleft (LGPL v2.1+) licensing. Zero GPL-only or non-free assets exist in the media supply chain.
