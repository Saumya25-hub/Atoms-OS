# ATOMS OS — PHASE 5: JAVA RUNTIME EXPANSION & CORE CLASS LIBRARY FOUNDATION PATCH PLAN

## MANDATORY ATOMS OS RULE 0 PROTOCOL

```text
INVESTIGATE ➔ PLAN ➔ PATCH ➔ BUILD ➔ TEST ➔ CERTIFY
[COMPLETE]     [TASK 2 PLAN] [TASK 3 NEXT]  [TASK 4]    [TASK 4]
```

**Date:** 2026-09-15  
**Milestone:** Phase 5 — Java Runtime Expansion & Core Class Library Foundation  
**Target Executable:** `build/jvm.elf`  
**Target Image:** `build/atoms_uefi_test.img`

---

## 1. ARCHITECTURAL DESIGN & EXECUTION STRATEGY

To expand the JVM into a usable application foundation, Phase 5 will implement the following architectural enhancements:

```text
               ┌────────────────────────────────────────────────────────┐
               │              ATOMS OS Java Application                 │
               │        (Multi-Class, Packages, JAR, Resources)         │
               └───────────────────────────┬────────────────────────────┘
                                           │
       ┌───────────────────────────────────┴───────────────────────────────────┐
       │                       ATOMS JVM Core (Avian)                          │
       │  ┌──────────────────────┬──────────────────────┬───────────────────┐  │
       │  │ Dynamic Class Loader │ Bytecode Interpreter │ Object & GC Heap  │  │
       │  │ (Packages, Static)   │ (Fields, Wrappers)   │ (Mark-Sweep)      │  │
       │  └──────────┬───────────┴──────────┬───────────┴───────────┬───────┘  │
       │             │                      │                       │          │
       │  ┌──────────┴──────────────────────┴───────────────────────┴────────┐ │
       │  │             Core Class Library Foundation (java.*)               │ │
       │  │  • java.lang: Object, String, StringBuilder, Integer, Boolean,   │ │
       │  │               System (Properties, Time), Throwable, Thread       │ │
       │  │  • java.util: ArrayList (set, remove, clear, bounds check)       │ │
       │  │  • Resources: BOFS Resource Loader, JAR Asset Extractor          │ │
       │  │  • ZIP: Stored + Deflate Decompressor Engine                     │ │
       │  └─────────────────────────────────┬────────────────────────────────┘ │
       └────────────────────────────────────┼──────────────────────────────────┘
                                            │
                               ┌────────────┴────────────┐
                               │  ATOMS System Adapter   │
                               │  (System, Memory, Sys)  │
                               └────────────┬────────────┘
                                            │
                               ┌────────────┴────────────┐
                               │   BOS Ring 3 Runtime    │
                               │   (atoms_runtime.h)     │
                               └────────────┬────────────┘
                                            │
                               ┌────────────┴────────────┐
                               │   BOS Kernel (Ring 0)   │
                               └─────────────────────────┘
```

---

## 2. DETAILED SUBSYSTEM SPECIFICATIONS

### 2.1 Object Fundamentals & String Expansion
- **`java/lang/Object`:**
  - `equals(Object)`: Compares pointer identity (`this == other`).
  - `hashCode()`: Returns pointer hash (`(int32_t)(intptr_t)this`).
  - `toString()`: Returns `className + "@" + hex(hashCode)`.
  - `getClass()`: Returns pseudo `Class` object descriptor.
- **`java/lang/String`:**
  - `length()`: Returns string character length.
  - `charAt(int)`: Returns UTF-8 byte/char at index with bounds checking (`IndexOutOfBoundsException`).
  - `equals(Object)`: Byte-by-byte content comparison if argument is a String.
  - `startsWith(String)` / `endsWith(String)`: Prefix/suffix substring comparison.
  - `indexOf(String)`: Substring search returning starting index or -1.
  - `substring(int, int)`: Allocates new String from heap substring.
  - `concat(String)`: Concatenates two strings into a new heap String.

### 2.2 StringBuilder & Primitive Wrappers
- **`StringBuilder`:**
  - `append(int)`: Formats 32-bit signed integer to ASCII in place.
  - `append(Object)`: Invokes `Object.toString()` and appends result.
  - `length()`: Returns current length.
  - `setLength(int)`: Resets character count.
- **Primitive Wrappers:**
  - `Integer.parseInt(String)`: Parses base-10 ASCII string to signed integer with error checking.
  - `Integer.toString(int)`: Converts integer to String object.
  - `Integer.valueOf(int)` / `Boolean.valueOf(boolean)`: Value boxing.

### 2.3 Collections & Safety (`java/util/ArrayList`)
- **Methods:** `add(Object)`, `get(int)`, `set(int, Object)`, `remove(int)`, `size()`, `isEmpty()`, `clear()`.
- **Safety Invariant:** Accessing index $< 0$ or $\ge size$ raises `java/lang/IndexOutOfBoundsException` through the interpreter exception table system without memory corruption.

### 2.4 System Properties & CLI Arguments
- **`System.getProperty(String)`:**
  - `os.name` $\rightarrow$ `"ATOMS OS"`
  - `os.arch` $\rightarrow$ `"x86_64"`
  - `java.version` $\rightarrow$ `"1.8.0_ATOMS"`
  - `java.vendor` $\rightarrow$ `"ATOMS OS Project"`
  - `user.dir` $\rightarrow$ `"/apps"`
- **`System.currentTimeMillis()` / `System.nanoTime()`:** Mapped to `System::now()` (RDTSC / PIT monotonic clock).
- **Application Arguments:** Construct genuine `String[]` array from CLI `argv`, passing elements to `main(String[])`.

### 2.5 Static & Instance Field Support
- **`putstatic` (`0xb3`) / `getstatic` (`0xb2`):** Maintain class static field storage map (`class_name + field_name` $\rightarrow$ `intptr_t`).
- **`putfield` (`0xb5`) / `getfield` (`0xb4`):** Read and write instance field slots on heap objects.

### 2.6 JAR Resource Loading & Deflate Support
- Implement `getResourceAsStream(String)` / `getSystemResourceAsStream(String)` extracting raw byte assets from BOFS and JAR ZIP tables.
- Implement standalone, bounds-checked Deflate decompressor for compressed JAR entries (Method 8).

---

## 3. FILE MODIFICATION LEDGER

| File Path | Action | Description |
| :--- | :---: | :--- |
| [`third_party/avian/include/avian/processor.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/processor.h) | Create/Modify | Declare expanded interpreter structures, static field registry, and library APIs. |
| [`third_party/avian/src/processor.cpp`](file:///D:/Signatures_OS/third_party/avian/src/processor.cpp) | Modify | Implement String, StringBuilder, ArrayList, System properties, getstatic/putstatic, getfield/putfield, and wrapper methods. |
| [`third_party/avian/include/avian/zip.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/zip.h) | Modify | Add resource extraction and Deflate decompression declarations. |
| [`third_party/avian/src/zip.cpp`](file:///D:/Signatures_OS/third_party/avian/src/zip.cpp) | Modify | Implement resource extraction and freestanding Deflate/Inflate stream decoder. |
| [`third_party/avian/include/avian/machine.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/machine.h) | Modify | Add `getResource()`, CLI argument packaging, and static field management. |
| [`third_party/avian/src/machine.cpp`](file:///D:/Signatures_OS/third_party/avian/src/machine.cpp) | Modify | Implement String[] argument construction, static field storage, and resource loading. |
| [`userspace/apps/java/jvm_main.cpp`](file:///D:/Signatures_OS/userspace/apps/java/jvm_main.cpp) | Modify | Update launcher with 25-point Phase 5 certification suite and multi-class demo execution. |
| [`tools/gen_phase5_assets.py`](file:///D:/Signatures_OS/tools/gen_phase5_assets.py) | Create | Build script to compile Phase 5 Java classes, JAR assets, and generate test asset headers. |
| [`BUILD.gn`](file:///D:/Signatures_OS/BUILD.gn) | Maintain | Keep GN build targets aligned. |
| [`build.ps1`](file:///D:/Signatures_OS/build.ps1) | Maintain | Maintain master build pipeline integration. |

---

## 4. ROLLBACK & RISK MITIGATION PLAN

1. **Risk:** Heap fragmentation during large memory pressure test (10,000 objects).  
   *Mitigation:* The Avian heap uses contiguous chunk allocation with Mark-Sweep collection; test verifies GC collection cycles without exhaustion.
2. **Risk:** Corrupt or truncated compressed Deflate JAR data causing infinite loops.  
   *Mitigation:* Inflate engine implements strict input/output bounds checks and aborts cleanly returning `false`.
3. **Rollback:** In the event of regression, revert via git:
   ```powershell
   git checkout HEAD -- third_party/avian/ userspace/apps/java/ tools/
   ```
