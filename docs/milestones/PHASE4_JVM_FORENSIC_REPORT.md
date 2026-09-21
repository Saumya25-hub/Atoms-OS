# ATOMS OS — Phase 4: Java Runtime Robustness, Extended Classpath & JAR Packaging
## Task 1: Forensic Investigation Report

**Document ID:** `PHASE4_JVM_FORENSIC_REPORT.md`  
**Milestone:** Java Runtime — Phase 4: Runtime Robustness, Extended Classpath & JAR Packaging  
**Date:** 2026-09-15  
**Investigator:** ATOMS OS Core Engineering & Runtime Architecture  
**Status:** FORENSIC AUDIT COMPLETE — NO CODE EDITED  

---

### 1. Executive Forensic Summary

In Phase 3, ATOMS OS achieved the milestone of executing its first real Java application (`HelloAtoms.class`) with unprivileged Ring 3 bytecode interpretation and console output (`Hello from ATOMS OS!`).

However, the Phase 3 runtime had specific deliberate constraints:
1. It targeted a single `.class` file rather than multi-class applications.
2. It skipped the classfile `exception_table`, lacking support for `athrow`, `try`/`catch`/`finally`, and handler PC resolution.
3. It had a basic classpath search and lacked `.jar` / `.zip` archive extraction and package-name hierarchy mapping (`com.atoms.demo.Main` ➔ `com/atoms/demo/Main.class`).
4. It only recognized `java.lang.Object`, `java.lang.String`, `java.lang.System`, and `java.io.PrintStream`, lacking `StringBuilder`, `ArrayList`, or static initializer (`<clinit>`) execution.

This forensic investigation analyzes the existing implementation in `third_party/avian/`, `userspace/apps/java/`, and the ATOMS runtime to determine the exact architectural additions required for **Phase 4: Runtime Robustness, Extended Classpath & JAR Packaging**.

---

### 2. Forensic Audit of Phase 3 Components

#### 2.1 Classfile Parser (`third_party/avian/src/classfile.cpp` & `include/avian/classfile.h`)
- **Current State:**
  - Decodes `0xCAFEBABE`, major versions $45 \le V \le 65$, and all 12 JVMS constant pool tags.
  - Extracts the `Code` attribute (`max_stack`, `max_locals`, `code_length`, `code`).
  - Line 298 in `classfile.cpp` currently reads `exception_table_length` but skips the entries:
    `uint16_t ex_count = 0; if (!r.readU16(&ex_count) || !r.skip(ex_count * 8)) ...`
- **Forensic Requirement for Phase 4:**
  - Define `struct ExceptionTableEntry { uint16_t start_pc; uint16_t end_pc; uint16_t handler_pc; uint16_t catch_type; };`.
  - Parse and store `exception_table` in `CodeAttribute`.
  - Provide helper method `const ExceptionTableEntry* findExceptionHandler(uint16_t pc, const char* exceptionClassName) const`.

#### 2.2 Bytecode Interpreter (`third_party/avian/src/processor.cpp`)
- **Current State:**
  - Implements basic opcodes: `getstatic`, `ldc`, `invokevirtual`, `aload_0`, `return`, `iadd`, `isub`, `imul`, `bipush`, `sipush`.
- **Forensic Requirement for Phase 4:**
  - **Exception Opcodes:**
    - `op_new` (`0xbb`): Allocates new object in heap (e.g. `RuntimeException`, `StringBuilder`, `ArrayList`).
    - `op_dup` (`0x59`): Duplicates top item on operand stack.
    - `op_athrow` (`0xbf`): Pops exception object reference, searches `exception_table` for handler where $\text{start\_pc} \le \text{pc} < \text{end\_pc}$ matching `catch_type`. If matched, clears operand stack, pushes exception object, and jumps to $\text{handler\_pc}$. If uncaught, propagates up call stack or reports `[JVM ERROR] Uncaught Java exception` and exits cleanly.
    - `op_astore` (`0x3a`), `op_astore_0..3` (`0x4b..0x4e`): Stores object reference into local variable table.
  - **Static Invocation & Multi-Method Opcodes:**
    - `op_invokestatic` (`0xb8`): Dispatches static methods across classes (e.g. `Greeter.sayHello()`, `Calculator.add(a, b)`).
    - `op_areturn` (`0xb0`): Returns object reference from method.
  - **Core Utility Classes:**
    - `java/lang/StringBuilder`: Supports `<init>()V`, `append(String)LStringBuilder;`, `toString()LString;`.
    - `java/util/ArrayList`: Supports `<init>()V`, `add(Object)Z`, `size()I`, `get(I)LObject;`.
    - `PrintStream`: Extends `println` to support integers (`println(I)V`).

#### 2.3 Class Loader & Classpath Abstraction (`third_party/avian/src/finder.cpp` & `src/machine.cpp`)
- **Current State:**
  - Loads a single file from disk or fallback buffer.
- **Forensic Requirement for Phase 4:**
  - **Classpath Search Model:** Searches directories (`/apps/`, `/apps/lib/`, `/system/java/`, current directory).
  - **Package Name Translation:** Converts dot-separated names (`com.atoms.demo.Main`) to path format (`com/atoms/demo/Main.class`).
  - **Multi-Class Registry:** Maintains an in-memory cache of loaded `ClassFile*` instances so that subsequent `invokestatic` / `new` calls resolve without re-parsing from disk.
  - **Static Class Initialization (`<clinit>`):** Executes `<clinit>()V` upon first class reference.

#### 2.4 JAR / ZIP Archive Architecture
- **ZIP File Format Forensic Structure:**
  - End of Central Directory Record (EOCD, signature `0x06054b50`).
  - Central Directory File Header (signature `0x02014b50`).
  - Local File Header (signature `0x04034b50`).
- **Archive Reader Engine:**
  - A lightweight, freestanding ZIP reader (`avian::zip::ZipArchive`) to parse the Central Directory of `.jar` files in memory/BOFS.
  - Locate files by path within archive (e.g. `META-INF/MANIFEST.MF`, `com/atoms/demo/Main.class`).
  - For uncompressed entries (standard `jar -0` or uncompressed stored classes), reads bytes directly via slice pointer.
  - Reads `Main-Class` manifest header from `META-INF/MANIFEST.MF`.

---

### 3. Java Bytecode Forensic Traces

#### 3.1 Exception Handling Bytecode Trace (`ExceptionTest.class`)
```text
Method: public static void main(java.lang.String[])
Code: stack=3, locals=2
   0: new           #7    // class java/lang/RuntimeException
   3: dup
   4: ldc           #9    // String ATOMS test
   6: invokespecial #11   // Method java/lang/RuntimeException."<init>":(Ljava/lang/String;)V
   9: athrow
  10: astore_1
  11: getstatic     #14   // Field java/lang/System.out:Ljava/io/PrintStream;
  14: ldc           #20   // String Exception caught
  16: invokevirtual #22   // Method java/io/PrintStream.println:(Ljava/lang/String;)V
  19: return
Exception table:
   from    to  target type
       0    10    10   Class java/lang/RuntimeException
```
- At $PC = 9$, `athrow` inspects the exception table:
  Range $[0, 10)$ covers $PC = 9$.
  Target is $PC = 10$.
  Catch type is `java/lang/RuntimeException`.
  Interpreter clears stack, pushes `RuntimeException` object, and sets $PC = 10$.

#### 3.2 Uncaught Exception Crash Trace (`CrashTest.class`)
```text
Method: public static void main(java.lang.String[])
Code: stack=3, locals=1
   0: new           #7    // class java/lang/RuntimeException
   3: dup
   4: ldc           #9    // String Uncaught ATOMS test
   6: invokespecial #11   // Method java/lang/RuntimeException."<init>":(Ljava/lang/String;)V
   9: athrow
```
- Exception table is empty.
- Interpreter detects uncaught exception in `main`, prints `[JVM ERROR] Uncaught Java exception: RuntimeException (Uncaught ATOMS test)`, and exits process with code 1 without causing a kernel panic.

#### 3.3 Multi-Class & Static Initialization Trace (`Main` + `Greeter`)
```text
Main.main():
   0: invokestatic  #2    // Method Greeter.sayHello:()V
   3: return

Greeter.<clinit>():
   0: getstatic     #7    // System.out
   3: ldc           #13   // "Static initialization"
   5: invokevirtual #15   // PrintStream.println
   8: return

Greeter.sayHello():
   0: getstatic     #7    // System.out
   3: ldc           #21   // "Hello from another Java class!"
   5: invokevirtual #15   // PrintStream.println
   8: return
```
- Output sequence:
  `Static initialization`
  `Hello from another Java class!`

---

### 4. Risk Analysis & Failure Modes

| Risk Item | Impact | Mitigation Strategy |
|:---|:---:|:---|
| **Corrupt JAR / Out-of-bounds Read** | High | Bounds-check every offset in the ZIP Central Directory parser. Reject invalid magic or truncated headers. |
| **Circular Class Dependencies** | Medium | Class registry marks classes in `StateLoading` to detect circular `<clinit>` dependency cycles. |
| **Uncaught Exception Kernel Fault** | High | Top-level frame catches unhandled exceptions in Ring 3 userspace, prints diagnostic, and calls `atoms_exit(1)`. |
| **Memory Leaks in Multi-Class Loader** | Medium | Centralized class registry owns all `ClassFile*` instances and disposes all allocated constant pools on VM shutdown. |

---

### 5. Identification of Files for Phase 4 (NO CODE)

The following files are identified for Task 2 Architectural Patch Planning:
1. `third_party/avian/include/avian/classfile.h` (Add `ExceptionTableEntry`, exception table array).
2. `third_party/avian/src/classfile.cpp` (Parse and store exception tables in `CodeAttribute`).
3. `third_party/avian/include/avian/zip.h` (NEW: Freestanding ZIP/JAR archive reader header).
4. `third_party/avian/src/zip.cpp` (NEW: Safe Central Directory & EOCD parser).
5. `third_party/avian/src/processor.cpp` (Implement `athrow`, `new`, `dup`, `invokestatic`, `StringBuilder`, `ArrayList`, `println(int)`).
6. `third_party/avian/include/avian/machine.h` & `src/machine.cpp` (Multi-class loader, classpath lookup, JAR executor, `<clinit>` trigger).
7. `userspace/apps/java/jvm_main.cpp` (Extended CLI runner supporting `jvm <class>`, `jvm <jar>`, and full Phase 4 test matrix).
8. `tools/gpt_image_builder.c` (Embed Phase 4 JAR and multi-class test files in disk image).
9. `BUILD.gn` & `build.ps1` (Add `zip.cpp` to `avian_core`, compile Java test classes and `.jar` archive).
