# ATOMS OS: COMPLETE THIRD-PARTY SOFTWARE LICENSES & NOTICES

**Document ID:** ATRIX-PHASE15-LICENSES-001  
**Phase:** Phase 15 — Master Third-Party Open Source Licensing Directory  
**Date:** 2026-08-26  

---

## 1. Chromium Core (Mojo IPC, Blink, GPU CommandBuffer, WebGL, Canvas, Media, File API, Web Audio, MSE, WebCodecs)
- **Project:** The Chromium Project / Mojo IPC / Blink / Chromium GPU / Chromium Net / Chromium DOM Storage / Content Security Policy / Web Audio
- **License:** BSD 3-Clause License
- **Copyright:** Copyright © 2014 The Chromium Authors. All rights reserved.

---

## 2. Google V8 JavaScript Engine
- **Project:** V8 JavaScript Engine
- **License:** BSD 3-Clause License
- **Copyright:** Copyright © 2014 the V8 project authors. All rights reserved.

---

## 3. Google Skia 2D Graphics Engine
- **Project:** Skia 2D Graphics Library
- **License:** BSD 3-Clause License
- **Copyright:** Copyright © 2011 Google Inc. All rights reserved.

---

## 4. LLVM / Clang / LLD
- **Project:** LLVM Project (Clang, LLD)
- **License:** Apache 2.0 with LLVM Exception
- **Copyright:** Copyright © LLVM Project Authors.

---

## 5. Musl Libc
- **Project:** Musl C Standard Library
- **License:** MIT License
- **Copyright:** Copyright © 2005-2020 Rich Felker, et al.

---

## 6. Filesystem Reference Specifications & Compatibility Research
- **Microsoft Open Specifications (`[MS-FSCC]`, `[MS-FSA]`):**
  - Public file system control codes and on-disk data structure specifications referenced for NTFS partition reading, attribute parsing, and forensic interoperability.
  - Copyright © Microsoft Corporation. Referenced for technical interoperability.
- **Linux Kernel `fs/ntfs3` & NTFS-3G:**
  - Public open-source implementations studied strictly as behavioral reference points for NTFS B-tree index traversal, collation tie-breaking, and allocation invariants.
  - **Attribution & Origin:** Neither Linux `fs/ntfs3` nor Tuxera NTFS-3G designs or code are claimed as ATOMS-original. Merely studying these implementations does not incorporate their code, but any future code reuse or porting from these sources must strictly respect their upstream licensing (GNU General Public License v2) and will be explicitly credited as third-party code.

---

## 7. Native Filesystem (BOFS) Statement
- **Project:** BOFS (BOS Operating Filesystem)
- **Status:** Native Filesystem for ATOMS OS (Planned / Architectural Phase)
- **Scope & Independence:**
  - BOFS is designed to be the native filesystem of ATOMS OS with its own independent architecture, metadata structures, and implementation.
  - **BOFS is NOT NTFS, is NOT Linux NTFS, and is NOT a copy or derivative of NTFS.**
  - Any future BOFS implementation remains distinct from legacy or third-party filesystem drivers.

---

## 8. Vendored Subdirectory Components
- **Capstone Disassembly Engine:**
  - Location: `capstone_src/capstone-4.0.2/`
  - License: BSD 3-Clause License (`capstone_src/capstone-4.0.2/LICENSE.TXT`)
  - Copyright © 2013-2018 Nguyen Anh Quynh et al.
- **Realtek Reference DKMS Package:**
  - Location: `realtek-r8125-dkms/`
  - License: GNU General Public License v2 (`realtek-r8125-dkms/LICENSE`)
  - Note: Retained strictly as an uncompiled reference driver package.
- **Doomgeneric Userspace Port:**
  - Location: `userspace/apps/doom/src/`
  - License: Doom Source License / GNU General Public License (`userspace/apps/doom/src/LICENSE`)
  - Copyright © 1993-1996 id Software, Inc.

---

## 9. Project Authorship & Core License Notice
- **ATOMS OS & BOS Kernel Core:**
  - Authored as part of the ATOMS OS project.
  - **Current Repository License Status:** The ATOMS OS core repository currently does not have an overarching open-source or copyleft license file (`LICENSE` / `COPYING`) at the root level. All rights in original ATOMS OS / BOS Kernel code remain reserved to the project authors pending a formal licensing decision. No license is assumed or invented.

