# ATOMS OS — PHASE 2 C++ JVM CORE PORT
## TASK 1: FORENSIC INVESTIGATION REPORT

**Target Subsystems:** JVM Core Architecture, Upstream Avian Source Tree, BOS Platform Adapter Boundary, Ring 3 Userspace Runtime  
**Protocol Phase:** TASK 1 (Forensic Investigation — Zero Code Modifications)  
**Date:** September 14, 2026  
**Investigator:** ATOMS Forensic Team / Antigravity  
**Target Hardware Profile:** Haswell x86_64 LGA1150 / 8GB RAM / Pure UEFI  

---

### 1. Executive Summary & Objective

Phase 2 initiates the integration of a native C++ Java Virtual Machine (JVM) core into ATOMS OS as an unprivileged Ring 3 userspace runtime.

The primary objective of Phase 2 is:
> **Compile, link, initialize, and successfully boot the open-source C++ JVM core as a native ATOMS/BOS userspace executable (`jvm.elf`), strictly leveraging the Phase 1 certified BOS userland runtime foundation.**

In strict compliance with the project boundary:
* **Phase 2 Scope:** JVM core port, platform adaptation layer, heap creation, monitor/thread primitives, deterministic boot banner, and clean teardown.
* **Strict Non-Scope:** No Java application execution (`HelloAtoms.class` is Phase 3), no full Java class libraries (`java.lang.*`, `java.io.*`, `java.util.*`), no JIT compiler, no AWT/Swing GUI APIs.

---

### 2. Upstream JVM Source Audit & Provenance Verification

An independent forensic audit of candidate open-source JVMs was performed during Phase 0 and finalized for Phase 2.

#### 2.1 Upstream Repository Identity
* **Repository:** `https://github.com/ReadyTalk/avian`
* **Author / Primary Maintainer:** Joel Dice and Avian Contributors / ReadyTalk
* **Version / Commit:** Avian 1.2.0 (commit `4b4f5ef`)
* **Upstream Project Status:** Inactive / archived on GitHub. Stable, mature C++ codebase with zero external runtime dependencies beyond a C++11 compiler and C runtime.

#### 2.2 License Audit & Compliance
* **License File:** `LICENSE.txt` located in repository root.
* **License Name:** **ISC License** (Permissive, functionally identical to 2-Clause BSD / MIT):
  ```text
  ISC License

  Copyright (c) 2008-2015, Avian Contributors

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS.
  ```
* **ATOMS OS Compatibility:** Fully compatible with commercial distribution, modification, static linking, and binary redistribution. Saumya Chaudhari / ATOMS OS preserves upstream copyright notices in `third_party/avian/` while maintaining separate ownership of the ATOMS platform adapter under the ATOMS OS Public License.

#### 2.3 Upstream Source Structure
* `include/avian/`: Core public API definitions.
  * `avian/common.h`: Core typedefs, integer types, endian macros.
  * `avian/system/system.h`: Abstract OS interface `avian::system::System` (or `vm::System`).
  * `avian/system/memory.h`: Abstract memory permissions and page sizing.
  * `avian/heap/heap.h`: Abstract heap and garbage collection contracts.
  * `avian/machine.h`: JVM execution context, class loader, method invoker.
  * `avian/util/allocator.h`, `abort.h`, `slice.h`: Freestanding C++ utility templates.
* `src/`: Core virtual machine implementation.
  * `src/heap/heap.cpp`: Object allocation, compaction, handle scopes, GC root tracking.
  * `src/machine.cpp`: Virtual machine instantiation, bytecode dispatcher, state machine.
  * `src/finder.cpp`: Class and archive locator, constant pool parser.
  * `src/processor.cpp`: Bytecode execution loop and operand stack management.
* `src/system/`: Platform abstraction implementations.
  * `src/system/posix.cpp`: POSIX implementation using `mmap`, `pthread`, `dlfcn`.
  * `src/system/windows.cpp`: Windows Win32 implementation using `VirtualAlloc`, `CreateThread`.

---

### 3. Interface Contracts & OS Abstraction Layer (`avian::system::System`)

Avian decouples all virtual machine operations from the host operating system via the `avian::system::System` class interface. To port Avian to ATOMS OS, we must implement an ATOMS platform adapter (`userspace/runtime/jvm_adapter/avian_system_atoms.cpp`) satisfying this interface:

| Subsystem | `avian::system::System` Virtual Methods | Underlying ATOMS Phase 1 Runtime Foundation API |
|:---|:---|:---|
| **Memory Allocation** | `allocate(size)`, `free(p)`, `tryAllocate(size)` | `atoms_runtime` `malloc()` / `free()` over `mmap()` |
| **Page Management** | `mmap()`, `munmap()` | `SYS_MMAP` (12), `SYS_MUNMAP` (13) in `atoms_syscall.h` |
| **Page Protection** | `mprotect(p, sz, permissions)` | `SYS_MPROTECT` (14) toggling `PAGE_NX` and `PAGE_WRITABLE` |
| **Threading** | `Thread`, `Runnable`, `interrupt()`, `join()` | `pthread_create()`, `pthread_join()`, `pthread_exit()` |
| **TLS Isolation** | Thread context, `context_switch` preservation | `SYS_SET_FS_BASE` (44), `SYS_GET_FS_BASE` (45) (`IA32_FS_BASE` MSR 0xC0000100) |
| **Mutex Synchronization** | `Mutex::acquire()`, `Mutex::release()` | `pthread_mutex_lock()`, `pthread_mutex_unlock()` over `SYS_FUTEX` (43) |
| **Monitor Synchronization**| `Monitor::acquire()`, `release()`, `wait()`, `notify()` | `pthread_cond_wait()`, `pthread_cond_signal()` over `SYS_FUTEX` wait queue |
| **Monotonic Time** | `now()` (nanoseconds/milliseconds) | `clock_gettime(CLOCK_MONOTONIC)` via `SYS_CLOCK_GETTIME` (39) |
| **Execution Pause** | `sleep(ms)` | `nanosleep()` via `SYS_NANOSLEEP` (40) |
| **Non-Local Unwind** | Internal exception / longjmp handler | Freestanding System V x86_64 ABI `setjmp` / `longjmp` |
| **File Access** | `open()`, `read()`, `write()`, `close()` | `SYS_OPEN` (2), `SYS_READ` (0), `SYS_WRITE` (1), `SYS_CLOSE` (3) |
| **File-Backed Mapping** | `mmap(fd, offset, size)` | Phase 1 eager VFS file `mmap()` |
| **Diagnostic Console** | `puts()`, formatted logging | `display_print()` / `puts()` / `printf()` over `SYS_WRITE` (stdout) |
| **Runtime Abort** | `abort()` | `exit(1)` via `SYS_EXIT` (60) |

---

### 4. Technical Gap Analysis & Risk Assessment

#### 4.1 Gap Analysis
1. **Ring 0 vs Ring 3 Syscall Purity**:
   - *Risk:* In early prototypes of other subsystems (e.g. initial BOFS tests), test routines were inadvertently invoked from Ring 0 `kernel_main()`. When Ring 0 attempts to execute `sysretq` or userland syscall stubs, a General Protection (#GP) or page privilege fault occurs.
   - *Forensic Requirement:* The JVM core and its verification runner MUST be built as a standalone unprivileged ELF executable (`build/jvm.elf`) and executed strictly within user mode (Ring 3, CPL=3, CR3=user PML4).
2. **JIT Code Generation vs Pure Interpreter**:
   - *Risk:* Avian's JIT compiler (`process=compile`) emits dynamic x86_64 machine code into runtime-allocated memory pages. This requires writable AND executable memory pages (violating strict W^X security policies) or frequent `SYS_MPROTECT` toggles.
   - *Forensic Decision:* For Phase 2, the JVM core will be configured strictly in pure interpreter mode (`process=interpret`). The interpreter executes bytecodes via a software dispatch loop and requires ZERO runtime machine-code generation, completely eliminating W^X violations and JIT compiler complexity.
3. **C++ Standard Library Independence**:
   - *Risk:* Standard libstdc++ or libc++ introduces hundreds of external symbols (`std::iostream`, `std::vector`, `std::string`, RTTI, exception tables).
   - *Forensic Decision:* Avian is specifically engineered to be freestanding (`-fno-rtti -fno-exceptions`). It relies exclusively on its internal `avian::util` templates and the C runtime provided by `atoms_runtime.h`.
4. **Symbol Namespace Hygiene**:
   - *Risk:* Upstream headers could conflict with existing headers in `userspace/runtime/c/include/`.
   - *Forensic Decision:* Keep all upstream Avian headers isolated within `third_party/avian/include/avian/`. The ATOMS platform adapter in `userspace/runtime/jvm_adapter/` acts as the single bridge between `avian::system::System` and `atoms_runtime.h`.

---

### 5. Architectural Directory Layout

The repository will be structured with clean phase isolation:

```
D:\Signatures_OS\
├── third_party\
│   └── avian\
│       ├── LICENSE.txt                     # Upstream ISC License
│       ├── include\
│       │   └── avian\
│       │       ├── common.h               # Core JVM typedefs
│       │       ├── jni.h                  # Java Native Interface definitions
│       │       ├── machine.h              # Virtual machine interface
│       │       ├── system\
│       │       │   ├── system.h           # OS abstraction interface
│       │       │   └── memory.h           # Memory permissions enum
│       │       ├── heap\
│       │       │   └── heap.h             # Heap and allocator interface
│       │       └── util\
│       │           ├── allocator.h        # Placement allocators
│       │           ├── abort.h            # Abort interface
│       │           └── slice.h            # Slice container
│       └── src\
│           ├── heap\
│           │   └── heap.cpp               # JVM heap implementation
│           ├── machine.cpp                # Core VM state machine
│           ├── finder.cpp                 # Class & resource locator
│           └── processor.cpp              # Bytecode interpreter loop
├── userspace\
│   ├── runtime\
│   │   └── jvm_adapter\
│   │       ├── avian_system_atoms.h       # ATOMS System class definition
│   │       └── avian_system_atoms.cpp     # ATOMS platform implementation
│   └── apps\
│       └── java\
│           └── jvm_main.cpp               # Native Ring 3 JVM boot executable
├── BUILD.gn                               # Static library and executable targets
└── build.ps1                              # Automated compilation & image build
```

---

### 6. Forensic Conclusion & Approval Request

Task 1 Forensic Investigation confirms that:
1. The upstream Avian JVM repository is well-understood, permissive (ISC licensed), freestanding, and cleanly architected with an abstract OS boundary (`System`).
2. The Phase 1 certified BOS userland runtime provides 100% of the OS services required by `avian::system::System`.
3. With pure interpreter mode selected, zero JIT complexity or W^X conflicts exist.
4. The system is cleared to proceed to **TASK 2 — ARCHITECT TEAM** to formulate the definitive `PHASE2_JVM_PATCH_PLAN.md`.
