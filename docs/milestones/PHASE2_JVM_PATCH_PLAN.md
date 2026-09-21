# ATOMS OS — PHASE 2 C++ JVM CORE PORT
## TASK 2: ARCHITECTURAL PATCH PLAN

**Document ID:** `ATOMS-JVM-PHASE2-PLAN-001`  
**Classification:** ARCHITECTURAL SPECIFICATION / PATCH PLAN  
**Date:** September 14, 2026  
**Status:** **PLAN APPROVED — READY FOR IMPLEMENTATION**  
**Architect:** ATOMS Architecture Team / Antigravity  
**Target Hardware Profile:** Haswell x86_64 LGA1150 / 8GB RAM / Pure UEFI  

---

### 1. Architectural Scope & Target File Ledger

In strict compliance with Rule 0 Phase Isolation, only the files explicitly listed in the ledger below are authorized for modification or creation. No unrelated kernel, driver, or userspace files may be touched.

| # | File Path | Action | Architectural Rationale |
|---|:---|:---:|:---|
| 1 | `third_party/avian/LICENSE.txt` | Create | Upstream ISC License text preserving original copyright. |
| 2 | `third_party/avian/include/avian/common.h` | Create | Core Avian JVM definitions, word sizing, types, and compiler pragmas. |
| 3 | `third_party/avian/include/avian/jni.h` | Create | Standard Java Native Interface definitions and JNIEnv/JavaVM function tables. |
| 4 | `third_party/avian/include/avian/system/system.h` | Create | Abstract OS interface `avian::system::System` and synchronization abstractions. |
| 5 | `third_party/avian/include/avian/system/memory.h` | Create | Memory protection bitmasks matching ATOMS `PROT_*` values. |
| 6 | `third_party/avian/include/avian/heap/heap.h` | Create | Abstract heap interface, handle scopes, and object allocation contracts. |
| 7 | `third_party/avian/include/avian/machine.h` | Create | Virtual machine lifecycle interface and VM configuration state. |
| 8 | `third_party/avian/include/avian/util/allocator.h` | Create | Freestanding allocator templates. |
| 9 | `third_party/avian/include/avian/util/abort.h` | Create | Standard abort interface. |
| 10 | `third_party/avian/include/avian/util/slice.h` | Create | Freestanding slice and buffer container. |
| 11 | `third_party/avian/src/heap/heap.cpp` | Create | Core heap arena implementation, handle scopes, GC root tracking. |
| 12 | `third_party/avian/src/machine.cpp` | Create | Virtual machine state machine, bootstrap sequence, JNI lifecycle. |
| 13 | `third_party/avian/src/finder.cpp` | Create | Class and archive lookup engine. |
| 14 | `third_party/avian/src/processor.cpp` | Create | Pure interpreter execution engine. |
| 15 | `userspace/runtime/jvm_adapter/avian_system_atoms.h` | Create | ATOMS Platform Adapter class definition inheriting `avian::system::System`. |
| 16 | `userspace/runtime/jvm_adapter/avian_system_atoms.cpp` | Create | ATOMS Platform Adapter implementation mapping `System` to `atoms_runtime.h`. |
| 17 | `userspace/apps/java/jvm_main.cpp` | Create | Native Ring 3 JVM boot executable with 10 deterministic hardware/subsystem tests. |
| 18 | `BUILD.gn` | Modify | Add `static_library("avian_core")` and `executable("jvm_test_runner")`. |
| 19 | `build.ps1` | Modify | Add Clang compilation for Avian core, link `build/jvm.elf`, update image builders. |
| 20 | `docs/milestones/PHASE2_JVM_PATCH_REPORT.md` | Create | Task 3 engineering and modification report. |
| 21 | `docs/milestones/PHASE2_JVM_CERTIFICATION_REPORT.md` | Create | Task 4 formal certification report covering all 22 required sections. |

---

### 2. Component Architecture & Design Specifications

#### 2.1 Separation of Concerns Boundary
* **Upstream JVM Core (`third_party/avian/`)**:
  - Implements the core JVM data structures, class loaders, object layout, handle scopes, and interpreter loop.
  - Zero knowledge of BOS Kernel internals, raw syscall numbers, or ATOMS data structures.
  - Interacts with the host environment ONLY through abstract virtual methods of `avian::system::System`.
  - Freestanding: Compiled with `-fno-rtti -fno-exceptions -std=c++20`.
* **ATOMS Platform Adapter (`userspace/runtime/jvm_adapter/`)**:
  - Concrete class `AtomsSystem` inherits from `avian::system::System`.
  - Connects Avian's abstract OS calls to Phase 1 certified `atoms_runtime.h`:
    1. **Memory**: `allocate(size)` invokes `malloc()`, `free(p)` invokes `free()`, direct page allocation calls `SYS_MMAP` / `SYS_MUNMAP`.
    2. **W^X Protection**: `mprotect()` maps Avian permission bitmask to `SYS_MPROTECT` with `PAGE_NX` enforcement.
    3. **Threads & TLS**: `System::Thread` wraps `pthread_create()`, ensuring `IA32_FS_BASE` MSR (0xC0000100) context-switch safety.
    4. **Mutex & Monitor**: Implemented using `pthread_mutex_t` and `pthread_cond_t` backed by the non-spinning 64-slot `SYS_FUTEX` wait queue.
    5. **Time**: `System::now()` calls `clock_gettime(CLOCK_MONOTONIC)`, `System::sleep()` calls `nanosleep()`.
    6. **Unwind**: Uses freestanding System V x86_64 ABI `setjmp` / `longjmp`.
    7. **I/O & Logging**: `System::write()` connects to stdout/stderr via `SYS_WRITE`.

#### 2.2 Execution Engine Strategy: Pure Interpreter Mode
* JIT compilation (`process=compile`) is explicitly omitted for Phase 2 to prevent W^X memory violations and maintain strict platform security.
* Pure interpreter mode (`process=interpret`) processes standard JVM bytecode streams directly via a deterministic dispatch table, ensuring maximum portability, minimal footprint, and zero dependency on dynamic code-generation mechanics.

#### 2.3 Standalone Executable & Test Harness (`jvm.elf`)
* Located in `userspace/apps/java/jvm_main.cpp`.
* Implements standard `main(int argc, char** argv)` entry point.
* Executes a deterministic 10-point test harness exercising:
  1. `AtomsSystem` instantiation and platform capability validation.
  2. Memory page allocation via `System::allocate()` and `System::mprotect()`.
  3. Freestanding heap initialization and handle scope creation.
  4. Object allocation and field manipulation within the heap arena.
  5. Multi-threaded worker spawning via `System::Thread`.
  6. TLS base persistence across thread execution.
  7. Mutex lock/unlock and monitor wait/notify across threads via `SYS_FUTEX`.
  8. Monotonic clock and nanosleep precision timing.
  9. System V x86_64 stack unwinding via `setjmp`/`longjmp`.
  10. Full virtual machine boot and clean shutdown sequence.
* Emits prominent serial log banners:
  ```
  =====================================================================
            ATOMS OS NATIVE JAVA RUNTIME (AVIAN JVM CORE)
  =====================================================================
  [JVM] ATOMS JVM CORE INITIALIZED
  ...
  [JVM] ATOMS JVM CORE SHUTDOWN
  =====================================================================
  ```

---

### 3. Build System Integration Plan

#### 3.1 GN / Ninja (`BUILD.gn`)
```gn
static_library("avian_core") {
  sources = [
    "third_party/avian/src/heap/heap.cpp",
    "third_party/avian/src/machine.cpp",
    "third_party/avian/src/finder.cpp",
    "third_party/avian/src/processor.cpp",
    "userspace/runtime/jvm_adapter/avian_system_atoms.cpp",
  ]
  include_dirs = [
    "third_party/avian/include",
    "userspace/runtime/include",
    "userspace/runtime/c/include",
    "userspace/runtime/cpp/include",
  ]
  deps = [
    ":atoms_runtime_c",
    ":atoms_runtime_cpp",
  ]
}

executable("jvm_test_runner") {
  sources = [
    "userspace/apps/java/jvm_main.cpp",
  ]
  deps = [
    ":atoms_runtime_c",
    ":atoms_runtime_cpp",
    ":avian_core",
  ]
}
```

#### 3.2 Master Build Script (`build.ps1`)
* Compile all Avian core files and the ATOMS adapter with Clang++:
  `-target x86_64-pc-none-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++20`
* Link `build/jvm.elf` using `ld.lld` against userspace CRT, libc, libm, and pthread.
* Copy `jvm.elf` into GPT disk image `/bin/jvm.elf` via `gpt_image_builder.exe`.

---

### 4. Risk Analysis & Defensive Measures

| Identified Risk | Impact | Defensive Architectural Mitigation |
|:---|:---|:---|
| **Ring 0 / Ring 3 Privilege Conflict** | #GP fault on `sysretq` | The JVM is strictly a userland executable (`build/jvm.elf`). Never invoke userland `AtomsSystem` from kernel Ring 0. |
| **W^X Memory Violations** | Kernel panic or process termination | Strict interpreter mode. All executable memory remains strictly read-only after load. No RWX pages allocated. |
| **Thread Deadlock on Futex** | Process hangs during monitor wait | Futex test harness includes bounded timeouts and guaranteed wakeups (`FUTEX_WAKE`). |
| **C++ ABI Mismatch** | Unresolved vtable or typeinfo symbols | Compiled with `-fno-rtti -fno-exceptions`. All virtual tables resolved locally via freestanding C++ runtime. |
| **Regression in Existing Binaries** | Desktop shell or browser failure | Zero modification to existing kernel or userspace binaries. Avian is isolated in its own directory tree. |

---

### 5. Rollback Plan

If any unexpected regression occurs during Phase 2 patching:
1. Revert `BUILD.gn` and `build.ps1` to their pre-Phase 2 checkpoint.
2. Remove `third_party/avian/`, `userspace/runtime/jvm_adapter/`, and `userspace/apps/java/`.
3. Re-run `./tools/ninja.exe -C out/Default` and `build.ps1` to verify clean restoration.
