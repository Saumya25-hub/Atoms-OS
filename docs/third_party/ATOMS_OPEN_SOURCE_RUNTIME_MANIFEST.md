# ATOMS OS — Open-Source Userspace Runtime Manifest & Provenance

> **Document ID:** ATOMS-RUNTIME-MANIFEST-001  
> **Target OS:** ATOMS OS (Native 64-bit BOS Kernel)  
> **Status:** **OFFICIALLY ACQUIRED & STAGED**  
> **Date:** September 7, 2026  

---

## 1. Primary Upstream Acquisition Registry

All userspace C/C++ runtime components have been acquired directly from authoritative upstream repositories without third-party forks or unofficial repackaging:

| Component | Upstream Git URL | Exact Commit Hash | Commit Date | Preserved Location | Primary License |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **musl libc** | `https://git.musl-libc.org/git/musl` | `5e9972eaef08ccf55dabe254ac829a30329793d3` | Sun Sep 6 14:25:12 2026 -0400 | `d:\Signatures_OS\third_party\musl\` | **Standard MIT License** |
| **LLVM libc++** | `https://github.com/llvm/llvm-project.git` | `1871cb232f00d67193c3ef085e26b67b56029d21` | Mon Sep 7 16:43:16 2026 +0300 | `d:\Signatures_OS\third_party\llvm\libcxx\` | **Apache 2.0 with LLVM Exception** |
| **LLVM libc++abi** | `https://github.com/llvm/llvm-project.git` | `1871cb232f00d67193c3ef085e26b67b56029d21` | Mon Sep 7 16:43:16 2026 +0300 | `d:\Signatures_OS\third_party\llvm\libcxxabi\`| **Apache 2.0 with LLVM Exception** |

---

## 2. License Preservation & Provenance

### A. musl libc License (MIT)
- **License File:** `third_party/musl/COPYRIGHT`
- **Copyright:** © 2005-2026 Rich Felker, et al.
- **Terms:** Permissive MIT License. Free for commercial, non-commercial, and operating system use.
- **Redistribution Requirement:** Retain copyright notice and permission notice in binary distributions.

### B. LLVM libc++ & libc++abi License (Apache 2.0 with LLVM Exception)
- **License File:** `third_party/llvm/LICENSE.TXT`
- **Copyright:** © 2003-2026 LLVM Project Authors.
- **Terms:** Apache License v2.0 with LLVM Exceptions.
- **Crucial Exception Clause:**
  > *"As an exception, if you produce an executable which includes code derived from this software, you may distribute that software without restriction, and without need to display any attribution or license notices."*
- **Significance for ATOMS OS:** Allows static compilation of `libc++.a` into proprietary and custom ATOMS userspace binaries without viral copyleft obligations.

---

## 3. Directory Layout Specification

In accordance with strict modular separation:

```text
d:\Signatures_OS\
├── third_party/
│   ├── chromium/
│   │   └── src/                  # Official Chromium upstream source
│   ├── musl/                     # Official musl upstream source (MIT)
│   └── llvm/
│       ├── libcxx/               # Official LLVM libc++ source
│       └── libcxxabi/            # Official LLVM libc++abi source
│
└── atoms/
    └── userspace/
        └── runtime/
            ├── syscall/          # Direct ATOMS IA32_LSTAR MSR syscall bridge
            ├── pthread/          # musl pthread to ATOMS thread/futex binding
            ├── tls/              # FS/GS segment register initialization
            ├── startup/          # crt0/crt1 entry point (_start)
            └── adapters/         # musl platform configuration for ATOMS target
```
