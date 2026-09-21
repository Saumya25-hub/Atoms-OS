# ATOMS OS — Phase 3: Java Bytecode Execution & Minimal Bootstrap Classpath
## Task 2: Architectural Patch Plan

**Document ID:** `PHASE3_JVM_PATCH_PLAN.md`  
**Milestone:** Java Runtime — Phase 3: Java Bytecode Execution + Minimal Bootstrap Classpath  
**Date:** 2026-09-14  
**Architect:** ATOMS OS Core Engineering & Userspace Architecture  
**Inputs:** [`PHASE3_JVM_FORENSIC_REPORT.md`](file:///D:/Signatures_OS/docs/milestones/PHASE3_JVM_FORENSIC_REPORT.md)  
**Status:** ARCHITECTURAL PLAN COMPLETE — AWAITING PATCH IMPLEMENTATION  

---

### 1. Executive Plan & Architectural Objectives

Under ATOMS OS Rule 0 Protocol (Investigate ➔ Plan ➔ Patch ➔ Certify), Task 2 defines the precise technical strategy to achieve genuine, unprivileged, standards-compliant Java bytecode execution of `HelloAtoms.class` on ATOMS OS.

The execution path strictly follows the canonical architectural chain:
```text
HelloAtoms.class
      ↓
BOFS / VFS (SYS_OPEN / SYS_READ)
      ↓
JVM Class Loader / Finder
      ↓
Binary Classfile Parser (Header, Constant Pool, Methods, Code Attribute)
      ↓
Constant Pool Resolution (Utf8, Class, NameAndType, Fieldref, Methodref, String)
      ↓
Minimal Bootstrap Classpath (Object, String, System.out, PrintStream)
      ↓
Bytecode Interpreter (getstatic, ldc, invokevirtual, return)
      ↓
Native Dispatch: PrintStream.println(String)
      ↓
ATOMS Runtime (atoms_write to STDOUT)
      ↓
BOS Syscall Gateway (SYS_WRITE)
      ↓
BOS Console Output: "Hello from ATOMS OS!"
```

---

### 2. Files to be Created or Modified

The following file ledger defines the complete and exclusive set of files authorized for modification in Task 3:

| File Path | Action | Architectural Purpose |
|:---|:---:|:---|
| [`userspace/apps/java/HelloAtoms.java`](file:///D:/Signatures_OS/userspace/apps/java/HelloAtoms.java) | **CREATE** | Canonical Java source code for `HelloAtoms`. |
| [`third_party/avian/include/avian/classfile.h`](file:///D:/Signatures_OS/third_party/avian/include/avian/classfile.h) | **CREATE** | JVMS 8 binary classfile definitions, constant pool tags, and metadata structures. |
| [`third_party/avian/src/classfile.cpp`](file:///D:/Signatures_OS/third_party/avian/src/classfile.cpp) | **CREATE** | Strict bounds-checked binary classfile parser and constant pool resolver. |
| [`third_party/avian/src/processor.cpp`](file:///D:/Signatures_OS/third_party/avian/src/processor.cpp) | **MODIFY** | Implement constant-pool-indexed bytecode opcodes (`getstatic`, `ldc`, `invokevirtual`, `aload`, `return`). |
| [`third_party/avian/src/machine.cpp`](file:///D:/Signatures_OS/third_party/avian/src/machine.cpp) | **MODIFY** | Bind minimal bootstrap classpath instances (`System.out`, `PrintStream`, `String` factory) to JVM machine state. |
| [`userspace/apps/java/jvm_main.cpp`](file:///D:/Signatures_OS/userspace/apps/java/jvm_main.cpp) | **MODIFY** | Implement CLI class launcher (`jvm <path>`) and automated test harness executing positive and negative tests. |
| [`tools/gpt_image_builder.c`](file:///D:/Signatures_OS/tools/gpt_image_builder.c) | **MODIFY** | Package `HelloAtoms.class` into the bootable GPT UEFI disk image. |
| [`BUILD.gn`](file:///D:/Signatures_OS/BUILD.gn) | **MODIFY** | Add `classfile.cpp` to the `avian_core` static library target. |
| [`build.ps1`](file:///D:/Signatures_OS/build.ps1) | **MODIFY** | Add `javac` compilation of `HelloAtoms.java`, Clang++ compilation of `classfile.cpp`, and disk packaging. |

---

### 3. Detailed Component Architecture

#### 3.1 Classfile Binary Parser (`classfile.h` & `classfile.cpp`)
- **Safe Reader:** `ByteReader` structure tracking pointer offset and remaining bytes to guarantee zero buffer overruns.
- **Magic Validation:** Reads `uint32_t magic`. If `magic != 0xCAFEBABE`, returns `ParseErrorInvalidMagic`.
- **Version Bounds:** Reads `minor_version` and `major_version`. Accepts $45 \le \text{major} \le 52$ (up to Java 8). Rejects unsupported versions with `ParseErrorUnsupportedVersion`.
- **Constant Pool Engine:** Reads `cp_count`. Parses all 12 standard tags. Properly handles 2-slot entries for `Long` and `Double`.
- **Code Attribute Locator:** Iterates through attributes of each method; matches attribute name `"Code"` against UTF-8 constant pool entries; extracts `max_stack`, `max_locals`, `code_length`, and byte array pointer.

#### 3.2 Bytecode Interpreter Execution Loop (`processor.cpp`)
- **Execution Frame:**
  - `stack[64]`: Operand stack holding integer and object pointer values (`intptr_t`).
  - `locals[16]`: Local variable array. `locals[0]` stores the `String[] args` array pointer.
  - `pc`: Program counter advancing through bytecode array.
- **Opcode Handling:**
  - `op_getstatic` (`0xb2`):
    - Reads 16-bit big-endian index: `uint16_t field_idx = readU16(code, pc)`.
    - Resolves `Fieldref` ➔ Class name & Field name.
    - If `java/lang/System.out`, pushes heap pointer to the global `PrintStream` object.
  - `op_ldc` (`0x12`):
    - Reads 8-bit index: `uint8_t const_idx = code[pc++]`.
    - Resolves `String` entry ➔ Utf8 string payload.
    - Allocates/resolves a Java `String` instance from the Avian heap containing the UTF-8 payload.
    - Pushes `String` object reference onto the operand stack.
  - `op_invokevirtual` (`0xb6`):
    - Reads 16-bit index: `uint16_t method_idx = readU16(code, pc)`.
    - Resolves `Methodref` ➔ Class name, Method name, Descriptor.
    - If `java/io/PrintStream.println:(Ljava/lang/String;)V`:
      - Pops argument (Java `String` reference).
      - Pops receiver object (`PrintStream` reference).
      - Validates non-null receiver.
      - Extracts string text from Java String object.
      - Dispatches to platform `system_->write(1, str, len)` and prints newline.
  - `op_aload_0` (`0x2a`): Pushes `locals[0]` (`args` array).
  - `op_return` (`0xb1`): Completes method execution; returns 0.

#### 3.3 Bootstrap Classpath & Native Bindings (`machine.cpp`)
- Creates pre-allocated instances in the heap during VM boot:
  - `System.out`: Instance of `java/io/PrintStream`.
  - Registered class metadata pointers for `java/lang/Object`, `java/lang/String`, `java/lang/System`, `java/io/PrintStream`.

#### 3.4 Standalone Launcher & Test Suite (`jvm_main.cpp`)
- **CLI Mode:** When invoked as `jvm <filename>`:
  - Opens `<filename>` via `system->open(path, 0)`.
  - Reads class bytes into memory.
  - Invokes `ClassFile::parse()`.
  - Locates `public static void main([Ljava/lang/String;)V`.
  - Constructs `String[]` array in the heap.
  - Executes `main()`.
  - Exits with return code 0.
- **Test Matrix Mode:** When invoked without arguments:
  - Runs Phase 2 Subsystem Verification (10/10 checkpoints).
  - Runs Phase 3 Bytecode Execution on `HelloAtoms.class`.
  - Runs Negative Tests:
    1. Missing class test.
    2. Invalid magic test (`0xDEADBEEF`).
    3. Unsupported version test (version `999.0`).
    4. Missing main method test.

---

### 4. Risk Analysis & Failure Modes

1. **Risk: Malformed Class Causing Memory Fault**  
   *Mitigation:* All reads in `classfile.cpp` are checked against remaining buffer length. Any out-of-bounds read aborts cleanly with an explicit error code.
2. **Risk: False-Positive Fake Execution**  
   *Mitigation:* Verification requires tracing through the actual constant pool resolution, `getstatic` field resolution, `ldc` string allocation, and `invokevirtual` dispatch.
3. **Risk: Kernel Regression**  
   *Mitigation:* `kernel.c` and all Ring 0 files are strictly locked. All execution is unprivileged Ring 3 userspace.

---

### 5. Rollback Plan

If any phase fails or regressions occur:
```powershell
git checkout HEAD -- third_party/avian/ userspace/apps/java/ tools/gpt_image_builder.c BUILD.gn build.ps1
```
All files will be reverted cleanly to the Phase 2 certified state.
