# ATOMS OS: THIRD-PARTY RUNTIME LICENSES & LEGAL NOTICES

**Document ID:** ATRIX-PHASE7-LICENSES-001  
**Phase:** Phase 7 — Open-Source License & Provenance Documentation  
**Date:** 2026-08-26  

---

## 1. Third-Party Open-Source Components Utilized in ATOMS Runtime

The ATOMS OS Userspace and C/C++ Runtime layer integrates and adapts mature, standard open-source projects under highly permissive licenses.

---

### 1.1 musl libc
- **Project:** musl libc (C Standard Library)
- **Version:** 1.2.5
- **Original Authors:** Rich Felker, Szabolcs Nagy, and contributors (https://musl.libc.org/)
- **License:** **MIT License**
- **License Text:**
```text
Copyright © 2005-2024 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

---

### 1.2 LLVM libc++ & libc++abi
- **Project:** LLVM libc++ (C++ Standard Library & Runtime)
- **Version:** 19.x / 20.x
- **Original Authors:** The LLVM Project / University of Illinois (https://libcxx.llvm.org/)
- **License:** **Apache 2.0 License with LLVM Exceptions**
- **License Summary:** Permissive license permitting free commercial and non-commercial redistribution, modification, and sublicensing. Does not require releasing source code of proprietary or independent host operating systems.

---

### 1.3 dlmalloc / TLSF Memory Allocator
- **Project:** dlmalloc (Doug Lea Malloc)
- **Original Author:** Doug Lea
- **License:** **Public Domain / CC0 / Permissive**
- **Attribution:** This software is in the public domain and is used as the underlying basis for userland arena and heap chunk management.
