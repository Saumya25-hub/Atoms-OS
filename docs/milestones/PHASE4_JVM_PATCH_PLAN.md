# ATOMS OS — Phase 4: Java Runtime Robustness, Extended Classpath & JAR Packaging
## Task 2: Architectural Patch Plan

**Document ID:** `PHASE4_JVM_PATCH_PLAN.md`  
**Milestone:** Java Runtime — Phase 4: Runtime Robustness, Extended Classpath & JAR Packaging  
**Date:** 2026-09-15  
**Architect:** ATOMS OS Core Engineering & Userspace Architecture  
**Inputs:** [`PHASE4_JVM_FORENSIC_REPORT.md`](file:///D:/Signatures_OS/docs/milestones/PHASE4_JVM_FORENSIC_REPORT.md)  
**Status:** ARCHITECTURAL PLAN COMPLETE — AWAITING PATCH IMPLEMENTATION  

---

### 1. Executive Strategy & Architectural Objectives

The primary goal of Phase 4 is to transition the ATOMS OS Java Runtime from a single-class proof-of-concept into a multi-class, archive-aware, exception-safe application runtime.

```text
       ┌─────────────────────────────────────────────────────────┐
       │                   JAVA APPLICATION                      │
       │     (.class hierarchy, package names, or .jar archive)  │
       └────────────────────────────┬────────────────────────────┘
                                    │
                                    ▼
       ┌─────────────────────────────────────────────────────────┐
       │                    ATOMS JVM CORE                       │
       │  ┌───────────────────┬───────────────────────────────┐  │
       │  │ ClassLoader / VFS │ Central Class Registry Cache  │  │
       │  │ ZIP / JAR Reader  │ Package Name Converter        │  │
       │  ├───────────────────┼───────────────────────────────┤  │
       │  │ Classfile Parser  │ Exception Table Parser        │  │
       │  ├───────────────────┼───────────────────────────────┤  │
       │  │ Interpreter Loop  │ Multi-Method / invokestatic   │  │
       │  │ Exception Search  │ athrow / Handler Resolution   │  │
       │  │ Core Class Model  │ StringBuilder, ArrayList      │  │
       │  └───────────────────┴───────────────────────────────┘  │
       └────────────────────────────┬────────────────────────────┘
                                    │
                                    ▼
       ┌─────────────────────────────────────────────────────────┐
       │             ATOMS Unified Runtime (Ring 3)              │
       │              atoms_runtime.h (VFS / Syscalls)           │
       └─────────────────────────────────────────────────────────┘
```

---

### 2. File Change Ledger

The following file ledger defines all files authorized for creation and modification in Task 3:

| File Path | Action | Architectural Purpose |
|:---|:---:|:---|
| [`third_party/avian/include/avian/classfile.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/classfile.h) | **MODIFY** | Add `ExceptionTableEntry` structure and exception table accessor methods. |
| [`third_party/avian/src/classfile.cpp`](file:///D:/Signatures_OS/third_party/avian/src/classfile.cpp) | **MODIFY** | Parse `exception_table` in `Code` attribute and implement handler matching. |
| [`third_party/avian/include/avian/zip.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/zip.h) | **CREATE** | Freestanding ZIP / JAR Central Directory and entry extraction declarations. |
| [`third_party/avian/src/zip.cpp`](file:///D:/Signatures_OS/third_party/avian/src/zip.cpp) | **CREATE** | Bounds-checked EOCD and Central Directory parser with `MANIFEST.MF` reader. |
| [`third_party/avian/src/processor.cpp`](file:///D:/Signatures_OS/third_party/avian/src/processor.cpp) | **MODIFY** | Implement `athrow`, `new`, `dup`, `invokestatic`, `StringBuilder`, `ArrayList`, `println(I)`. |
| [`third_party/avian/include/avian/machine.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/machine.h) | **MODIFY** | Add class registry cache, classpath configuration, and JAR execution interfaces. |
| [`third_party/avian/src/machine.cpp`](file:///D:/Signatures_OS/third_party/avian/src/machine.cpp) | **MODIFY** | Implement multi-class loading, package resolution, JAR execution, and `<clinit>` trigger. |
| [`userspace/apps/java/jvm_main.cpp`](file:///D:/Signatures_OS/userspace/apps/java/jvm_main.cpp) | **MODIFY** | Implement enhanced CLI (`jvm <class>`, `jvm <jar>`) and comprehensive Phase 4 test suite. |
| [`tools/gpt_image_builder.c`](file:///D:/Signatures_OS/tools/gpt_image_builder.c) | **MODIFY** | Embed Phase 4 JAR and multi-class test files in the bootable disk image. |
| [`BUILD.gn`](file:///D:/Signatures_OS/BUILD.gn) | **MODIFY** | Add `zip.cpp` to `avian_core` static library target. |
| [`build.ps1`](file:///D:/Signatures_OS/build.ps1) | **MODIFY** | Add `javac` compilation of Phase 4 test classes and `jar` packaging rules. |

---

### 3. Detailed Component Architecture

#### 3.1 Exception Handling Architecture
- **Metadata Structure:**
  ```cpp
  struct ExceptionTableEntry {
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type; // Constant pool index to Class, or 0 for any/finally
  };
  ```
- **Runtime Execution (`athrow`):**
  1. Pops the thrown exception object pointer from the operand stack.
  2. Scans `exception_table` of the currently executing method:
     - Tests condition: $\text{start\_pc} \le \text{pc} < \text{end\_pc}$.
     - If $\text{catch\_type} == 0$, matches unconditionally (`finally` block).
     - If $\text{catch\_type} > 0$, resolves class name from constant pool and checks type compatibility.
  3. If a handler is found:
     - Clears the method operand stack.
     - Pushes the exception object reference.
     - Sets $\text{pc} = \text{handler\_pc}$ and resumes bytecode execution.
  4. If uncaught in method:
     - Unwinds execution frame and returns an exception status code to the caller.
     - If uncaught at the root `main()` method:
       Emits `[JVM ERROR] Uncaught Java exception: <Type> (<Message>)` and terminates the process cleanly via `atoms_exit(1)`.

#### 3.2 Multi-Class & Classpath Resolution
- **Class Registry:** Maintains an array of cached `ClassFile*` pointers indexed by normalized class name (`java/lang/String`, `Greeter`, `com/atoms/demo/Main`).
- **Resolution Strategy:**
  1. Check registry cache. If already loaded, return pointer.
  2. If not found:
     - Convert dot notation (`com.atoms.demo.Greeter`) to path (`com/atoms/demo/Greeter.class`).
     - Probe active JAR archives.
     - Probe filesystem search paths (`/apps/`, `/apps/lib/`, `/system/java/`, `./`).
  3. Upon parsing a new class:
     - Insert into registry.
     - If the class declares `<clinit>()V`, invoke static initialization immediately before returning.

#### 3.3 Freestanding ZIP / JAR Archive Engine (`zip.h` & `zip.cpp`)
- **End of Central Directory (EOCD) Locator:** Scans buffer backwards from end to locate signature `0x06054b50`.
- **Central Directory Parser:** Iterates through records (`0x02014b50`), extracting entry names, compressed/uncompressed lengths, compression methods, and local header offsets.
- **Entry Extraction:** For stored/uncompressed entries (compression method 0), provides direct zero-copy slice pointers to entry bytes.
- **Manifest Parser:** Locates `META-INF/MANIFEST.MF`, parses line `Main-Class: <ClassName>`, trims trailing whitespace/CR/LF.

#### 3.4 Extended Core Class Library
- **`java/lang/StringBuilder`:**
  - Struct `JavaStringBuilder`: dynamically allocated char array, length, capacity.
  - Methods: `<init>()V`, `append(String)LStringBuilder;`, `toString()LString;`.
- **`java/util/ArrayList`:**
  - Struct `JavaArrayList`: dynamic array of `void*` elements, size, capacity.
  - Methods: `<init>()V`, `add(Object)Z`, `size()I`, `get(I)LObject;`.
- **`java/io/PrintStream`:**
  - Supported overloads: `println(String)V`, `println(int)V`.

---

### 4. Risk Analysis & Mitigation

1. **Risk: Malformed ZIP/JAR Causing Buffer Overrun**  
   *Mitigation:* All offsets, headers, and lengths in `zip.cpp` are validated against total archive slice size. Any invalid offset halts parsing immediately.
2. **Risk: Stack Overflow in Recursive Method Calls**  
   *Mitigation:* Set maximum call depth limit (e.g. 64 frames) to prevent stack exhaustion.
3. **Risk: False-Positive Fake Execution**  
   *Mitigation:* Bytecode disassembly and step-by-step verification prove that bytecode instructions (`athrow`, `invokestatic`, `new`, `dup`) execute legitimately.

---

### 5. Rollback Strategy

If regressions occur:
```powershell
git checkout HEAD -- third_party/avian/ userspace/apps/java/ tools/gpt_image_builder.c BUILD.gn build.ps1
```
