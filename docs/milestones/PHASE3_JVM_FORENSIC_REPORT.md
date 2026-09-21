# ATOMS OS — Phase 3: Java Bytecode Execution & Minimal Bootstrap Classpath
## Task 1: Forensic Investigation Report

**Document ID:** `PHASE3_JVM_FORENSIC_REPORT.md`  
**Milestone:** Java Runtime — Phase 3: Java Bytecode Execution + Minimal Bootstrap Classpath  
**Date:** 2026-09-14  
**Investigator:** ATOMS OS Core Engineering & Runtime Architecture  
**Status:** FORENSIC AUDIT COMPLETE — PENDING ARCHITECTURAL PLAN  

---

### 1. Executive Forensic Summary

In Phase 2, the C++ JVM core (Avian JVM architecture) was successfully ported, compiled, and certified as a native Ring 3 userspace executable (`build/jvm.elf`). The Phase 2 test suite proved that:
1. The JVM platform adapter (`avian::system::System`) operates cleanly over the Phase 1 certified userspace runtime foundation (`atoms_runtime.h`).
2. Virtual memory allocation (`atoms_mmap`), page permissions (`atoms_mprotect`), high-resolution clocks (`atoms_clock_gettime`), and futex-based mutex synchronization function without kernel transition traps or privilege violations.
3. The JVM core initializes (`[JVM] ATOMS JVM CORE INITIALIZED`) and shuts down cleanly (`[JVM] ATOMS JVM CORE SHUTDOWN`).

However, the Phase 2 milestone deliberately stopped short of executing real Java `.class` files. The goal of **Phase 3** is to bridge the gap between core VM lifecycle initialization and **real Java bytecode execution** of the canonical test program:

```java
public class HelloAtoms {
    public static void main(String[] args) {
        System.out.println("Hello from ATOMS OS!");
    }
}
```

This forensic audit investigates the exact JVM components, classfile parsing requirements, constant pool resolution, opcode execution loop, minimal bootstrap classpath, and filesystem loading mechanisms required to achieve true, un-faked bytecode execution on ATOMS OS.

---

### 2. Upstream JVM Source Audit & Bytecode Infrastructure

An exhaustive audit of the integrated Avian JVM source in `third_party/avian/` was conducted:

1. **Class Finder (`src/finder.cpp`):**
   - The Phase 2 implementation of `BootClasspathFinder` queries files via `system_->open(name, 0)` and `system_->mmap(fd, 0, 4096)`.
   - **Forensic Finding:** In Phase 2, this only probed the path. To support real class loading, the finder must read the complete classfile bytes into an allocated memory region (`system::System::Region`), query file size, and handle path prefixes (e.g. `/apps/HelloAtoms.class` and classpath directories `/system/java/bootstrap/`).

2. **Bytecode Interpreter (`src/processor.cpp`):**
   - The Phase 2 implementation of `InterpreterProcessor` provided basic arithmetic opcodes (`iadd`, `isub`, `imul`, `bipush`, `iload`, `istore`).
   - **Forensic Finding:** To execute `HelloAtoms.class`, the interpreter must implement constant-pool-indexed opcodes:
     - `getstatic` (`0xb2`): Resolves static field reference (e.g. `java/lang/System.out`).
     - `ldc` (`0x12`): Resolves 8-bit constant pool index to a runtime `String` or constant.
     - `invokevirtual` (`0xb6`): Dispatches virtual method calls (e.g. `PrintStream.println(String)`).
     - `invokespecial` (`0xb7`): Resolves instance initialization (`<init>()V`).
     - `invokestatic` (`0xb8`): Dispatches static method calls (e.g. `HelloAtoms.main(String[])`).
     - `aload_0` (`0x2a`) / `aload` (`0x19`): Pushes object reference from local variable table onto operand stack.
     - `return` (`0xb1`): Cleanly terminates void method execution.

3. **Heap & Object Model (`src/heap/heap.cpp`):**
   - Implements `allocateObject` and `allocateArray`.
   - **Forensic Finding:** The object layout stores an 8-byte aligned `ObjectHeader` holding object flags, class pointer, and byte size immediately preceding the object payload. This conforms directly to Java object reference semantics.

4. **Machine Lifecycle (`src/machine.cpp`):**
   - Implements `makeMachine`, `boot()`, `shutdown()`, and JNI invocation tables.
   - **Forensic Finding:** The machine structure requires a reference to the boot class finder and an execution entry point to load and invoke the target class `main` method.

---

### 3. Java Version & Bytecode Disassembly Analysis

Using the host Java compiler (`javac 25.0.1` targeting standard Java 8 classfile format via `--release 8`), `HelloAtoms.java` was compiled and disassembled with `javap -v -p -c`:

```text
Classfile /scratch/HelloAtoms.class
  size: 434 bytes
  magic: 0xCAFEBABE
  minor version: 0
  major version: 52 (Java SE 8)
  flags: ACC_PUBLIC, ACC_SUPER
  this_class: #21 (HelloAtoms)
  super_class: #2 (java/lang/Object)
  interfaces: 0, fields: 0, methods: 2, attributes: 1

Constant pool (28 entries):
   #1 = Methodref          #2.#3          // java/lang/Object."<init>":()V
   #2 = Class              #4             // java/lang/Object
   #3 = NameAndType        #5:#6          // "<init>":()V
   #4 = Utf8               java/lang/Object
   #5 = Utf8               <init>
   #6 = Utf8               ()V
   #7 = Fieldref           #8.#9          // java/lang/System.out:Ljava/io/PrintStream;
   #8 = Class              #10            // java/lang/System
   #9 = NameAndType        #11:#12        // out:Ljava/io/PrintStream;
  #10 = Utf8               java/lang/System
  #11 = Utf8               out
  #12 = Utf8               Ljava/io/PrintStream;
  #13 = String             #14            // Hello from ATOMS OS!
  #14 = Utf8               Hello from ATOMS OS!
  #15 = Methodref          #16.#17        // java/io/PrintStream.println:(Ljava/lang/String;)V
  #16 = Class              #18            // java/io/PrintStream
  #17 = NameAndType        #19:#20        // println:(Ljava/lang/String;)V
  #18 = Utf8               java/io/PrintStream
  #19 = Utf8               println
  #20 = Utf8               (Ljava/lang/String;)V
  #21 = Class              #22            // HelloAtoms
  #22 = Utf8               HelloAtoms
  #23 = Utf8               Code
  #24 = Utf8               LineNumberTable
  #25 = Utf8               main
  #26 = Utf8               ([Ljava/lang/String;)V
  #27 = Utf8               SourceFile
  #28 = Utf8               HelloAtoms.java

Method 1: public HelloAtoms()
  descriptor: ()V
  Code:
    stack=1, locals=1, args_size=1
       0: aload_0
       1: invokespecial #1   // Method java/lang/Object."<init>":()V
       4: return

Method 2: public static void main(java.lang.String[])
  descriptor: ([Ljava/lang/String;)V
  Code:
    stack=2, locals=1, args_size=1
       0: getstatic     #7   // Field java/lang/System.out:Ljava/io/PrintStream;
       3: ldc           #13  // String Hello from ATOMS OS!
       5: invokevirtual #15  // Method java/io/PrintStream.println:(Ljava/lang/String;)V
       8: return
```

#### Forensic Observations:
1. **Classfile Version:** Major version `52` (Java 8), minor version `0`. The JVM parser must accept major versions $45 \le V \le 52$ (and up to 65 for modern classfiles where compatible).
2. **Bytecode Stream:** The `main` method consists of exactly 9 bytes of bytecode:
   - `0xb2 0x00 0x07` (`getstatic #7`)
   - `0x12 0x0d` (`ldc #13`)
   - `0xb6 0x00 0x0f` (`invokevirtual #15`)
   - `0xb1` (`return`)
3. **Execution Frame Requirements:**
   - Stack depth: At least 2 operand stack slots.
   - Locals: Exactly 1 local variable slot (`locals[0]`), which holds the reference to the `String[] args` array object.

---

### 4. Class File Binary Parser Requirements

The binary class file format is defined by the JVM Specification (JVMS 8 §4):
- **Magic:** 4 bytes, `0xCAFEBABE`. Any other value must fail closed with `Invalid class file magic`.
- **Minor / Major Version:** 2 bytes each. Must be within supported bounds.
- **Constant Pool Count:** 2 bytes ($N$). There are $N-1$ constant pool items (index 1 to $N-1$).
- **Constant Pool Tags:**
  - `CONSTANT_Utf8` (tag 1): 2-byte length + UTF-8 byte payload.
  - `CONSTANT_Integer` (tag 3): 4-byte big-endian signed integer.
  - `CONSTANT_Float` (tag 4): 4-byte IEEE 754 float.
  - `CONSTANT_Long` (tag 5): 8-byte big-endian signed integer (takes 2 pool entries).
  - `CONSTANT_Double` (tag 6): 8-byte IEEE 754 double (takes 2 pool entries).
  - `CONSTANT_Class` (tag 7): 2-byte name index pointing to Utf8.
  - `CONSTANT_String` (tag 8): 2-byte string index pointing to Utf8.
  - `CONSTANT_Fieldref` (tag 9): 2-byte class index + 2-byte NameAndType index.
  - `CONSTANT_Methodref` (tag 10): 2-byte class index + 2-byte NameAndType index.
  - `CONSTANT_InterfaceMethodref` (tag 11): 2-byte class index + 2-byte NameAndType index.
  - `CONSTANT_NameAndType` (tag 12): 2-byte name index + 2-byte descriptor index.
- **Access Flags, This Class, Super Class:** 2 bytes each.
- **Interfaces Count & Array:** 2 bytes count + 2-byte indices.
- **Fields Count & Array:** 2 bytes count + field info structures.
- **Methods Count & Array:** 2 bytes count + method info structures (`access_flags`, `name_index`, `descriptor_index`, `attributes_count`, `attributes`).
- **Code Attribute Parsing:** Locates attribute with name `"Code"`, parses `max_stack`, `max_locals`, `code_length`, extracts raw code byte array, and bounds-checks all offsets.

---

### 5. Minimal Bootstrap Classpath & Object Model Architecture

To avoid massive standard library bloat while executing genuine Java bytecode, we establish the minimal bootstrap class library:

| Class | Status | Purpose & Required Bindings |
|:---|:---:|:---|
| `java/lang/Object` | **REQUIRED** | Root of class hierarchy. Provides `<init>()V` constructor. |
| `java/lang/Class` | **REQUIRED** | Metadata reference for object headers (`ObjectHeader::class_ptr`). |
| `java/lang/String` | **REQUIRED** | Encapsulates String literals (`"Hello from ATOMS OS!"`) and character payloads. |
| `[Ljava/lang/String;` | **REQUIRED** | Array class descriptor representing `String[] args` passed to `main()`. |
| `java/lang/System` | **REQUIRED** | Provides static field `out` referencing `java/io/PrintStream`. |
| `java/io/PrintStream` | **REQUIRED** | Provides `println(Ljava/lang/String;)V` method bound to `atoms_write(STDOUT)`. |
| `java/lang/Throwable` | **OPTIONAL** | Error and exception hierarchy (Phase 4+). |
| `java/lang/Thread` | **OPTIONAL** | Multithreading Java objects (Phase 4+). |

#### Native Dispatch Mechanism:
When `invokevirtual` targets `java/io/PrintStream.println:(Ljava/lang/String;)V`:
1. The interpreter pops the string argument reference and the target `PrintStream` receiver reference from the operand stack.
2. It verifies the receiver is the valid `System.out` instance.
3. It extracts the raw character/UTF bytes from the `java/lang/String` object.
4. It calls `system_->write(1, chars, len)` followed by `system_->write(1, "\n", 1)`, directly issuing the Ring 3 `atoms_write` syscall to standard output.

---

### 6. BOFS & Filesystem Integration

1. **Class File Provisioning:**
   - The compiled `HelloAtoms.class` must be placed in the filesystem so that the launcher can load it via `/apps/HelloAtoms.class`.
   - The disk builder (`tools/gpt_image_builder.c`) must embed the binary `HelloAtoms.class` directly into the disk image alongside `MEDIA.ELF`, `KERNEL.BIN`, etc.
   - The userspace VFS translates `/apps/HelloAtoms.class` to the underlying FAT32/BOFS volume via `SYS_OPEN` and `SYS_READ`.
2. **Path Resolution & Fallback:**
   - The JVM launcher will query:
     - Exact path provided in argv (`/apps/HelloAtoms.class` or `HelloAtoms.class`).
     - Root filesystem fallback (`/HELLO.CLS` or `/HelloAtoms.class` on FAT32 8.3 naming).
     - Cleanly handle file not found without kernel crash.

---

### 7. Negative Test Requirements

The JVM must handle all failure modes gracefully in Ring 3 userspace without triggering kernel panics:

1. **Missing Class:** `jvm /apps/Missing.class`  
   *Expected:* Emits `[JVM ERROR] Class not found: /apps/Missing.class` and terminates with exit code 1.
2. **Invalid Magic:** Class starting with `0xDEADBEEF` instead of `0xCAFEBABE`.  
   *Expected:* Emits `[JVM ERROR] Invalid class file magic: 0xDEADBEEF` and terminates with exit code 1.
3. **Unsupported Version:** Class with major version `999`.  
   *Expected:* Emits `[JVM ERROR] Unsupported class file version: 999.0` and terminates with exit code 1.
4. **Missing Main Method:** Class validly formatted but omitting `public static void main(String[])`.  
   *Expected:* Emits `[JVM ERROR] Main method not found in class` and terminates with exit code 1.

---

### 8. Risk Analysis & Mitigation

| Risk Item | Impact | Mitigation Strategy |
|:---|:---:|:---|
| **Memory Corruption via Corrupt Class** | High | Bounds-check every read in the classfile parser. If offset exceeds buffer size, abort parsing immediately with diagnostic. |
| **Stack Overflow in Interpreter** | Medium | Allocate fixed-size bounded execution frames; reject methods whose `max_stack` exceeds limit (e.g. 256). |
| **Kernel Crash on Bad Syscall** | High | Never bypass `atoms_runtime.h`; rely strictly on validated Ring 3 syscall gateway. |
| **Special-Case Fake Execution** | Rule 0 Violation | Parse actual classfile bytes, parse constant pool, execute interpreter opcode switch loop, verify `getstatic`, `ldc`, `invokevirtual` sequence. |

---

### 9. Suspected Fix & Files Involved (NO CODE)

The following components are identified for Task 2 Architectural Planning:
1. `third_party/avian/include/avian/classfile.h` (Classfile structures, constant pool definitions, method headers).
2. `third_party/avian/src/classfile.cpp` (Classfile binary parser, constant pool resolver, code attribute extractor).
3. `third_party/avian/src/processor.cpp` (Bytecode interpreter: implement `getstatic`, `ldc`, `invokevirtual`, `aload`, `return`).
4. `third_party/avian/src/machine.cpp` (Register bootstrap classes: `java/lang/Object`, `String`, `System.out`, `PrintStream`).
5. `userspace/apps/java/jvm_main.cpp` (Update launcher to accept command-line arguments, load class from BOFS, invoke `main()`, and run the test matrix).
6. `tools/gpt_image_builder.c` (Embed `HelloAtoms.class` and negative test assets into the UEFI disk image).
7. `BUILD.gn` & `build.ps1` (Compile `classfile.cpp` into `avian_core` and update image builder).
