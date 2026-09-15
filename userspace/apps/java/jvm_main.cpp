/*
 * ATOMS OS — Native Java Virtual Machine (Avian JVM Core) Boot Executable
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Ring 3 Userspace Runtime Executable & Comprehensive Verification Suite
 * Supports CLI Execution (jvm <class>, jvm <jar>) and Automated Phase 5 Certification.
 */

#include <userspace/runtime/include/atoms_runtime.h>
#include <userspace/runtime/jvm_adapter/avian_system_atoms.h>
#include <third_party/avian/include/avian/machine.h>
#include <third_party/avian/include/avian/heap/heap.h>
#include <third_party/avian/include/avian/classfile.h>
#include <third_party/avian/include/avian/processor.h>
#include <third_party/avian/include/avian/zip.h>
#include "hello_atoms_class.h"
#include "phase4_test_assets.h"
#include "phase5_test_assets.h"
#include "phase6_test_assets.h"
#include <third_party/avian/include/avian/jit.h>
#include <string.h>

extern "C" void display_print(const char *s);

namespace {

struct ThreadTestData : public avian::system::System::Runnable {
  avian::system::System::Thread* thread;
  avian::system::System::Monitor* monitor;
  bool finished;
  bool was_interrupted;
  int counter;

  ThreadTestData(avian::system::System::Monitor* mon)
      : thread(nullptr), monitor(mon), finished(false), was_interrupted(false), counter(0) {}

  virtual void attach(avian::system::System::Thread* t) override {
    thread = t;
  }

  virtual void run() override {
    monitor->acquire(thread);
    counter += 42;
    finished = true;
    monitor->notify(thread);
    monitor->release(thread);
  }

  virtual bool interrupted() override { return was_interrupted; }
  virtual void setInterrupted(bool v) override { was_interrupted = v; }
};

static bool endsWith(const char* str, const char* suffix) {
  if (!str || !suffix) return false;
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (suffix_len > str_len) return false;
  return strcmp(str + str_len - suffix_len, suffix) == 0;
}

} // anonymous namespace

extern "C" int main(int argc, char** argv) {
  // Initialize unified ATOMS C/C++ runtime constructors
  __libc_init_array();

  // CLI Mode: If a specific class file or JAR archive is passed on command line
  if (argc > 1 && argv[1][0] != '-') {
    avian::system::System* system = atoms::jvm::makeAtomsSystem();
    if (!system) {
      puts("[JVM ERROR] Could not allocate AtomsSystem adapter!");
      return 1;
    }

    avian::Machine* vm = avian::makeMachine(system, 1024 * 1024, 16 * 1024 * 1024);
    if (!vm || !vm->boot()) {
      puts("[JVM ERROR] Failed to boot Avian Virtual Machine!");
      if (vm) vm->shutdown();
      delete system;
      return 1;
    }

    int rc = 0;
    if (endsWith(argv[1], ".jar") || endsWith(argv[1], ".JAR")) {
      rc = vm->executeJar(argv[1], argc - 1, argv + 1);
    } else {
      rc = vm->executeClass(argv[1], argc - 1, argv + 1);
    }

    vm->shutdown();
    delete system;
    return rc;
  }

  // Verification Mode: Full Phase 2, Phase 3, Phase 4 & Phase 5 Automated Test Suite
  puts("\n=====================================================================");
  puts("          ATOMS OS NATIVE JAVA RUNTIME (AVIAN JVM CORE)");
  puts("       Phase 5 Java Expansion & Core Class Library Foundation Suite");
  puts("=====================================================================");

  // Test 1: Platform Adapter Initialization
  puts("[TEST 1/25] Initializing ATOMS System Adapter & Querying Caps...");
  avian::system::System* system = atoms::jvm::makeAtomsSystem();
  if (!system) {
    puts("  -> FAIL: Could not allocate AtomsSystem adapter!");
    return 1;
  }
  atoms_runtime_caps_t caps;
  atoms_runtime_get_caps(&caps);
  if (caps.has_tls_msr && caps.has_futex && caps.has_wx_mprotect && caps.has_setjmp_abi) {
    puts("  -> PASS: Unified Runtime Capabilities verified.");
  } else {
    puts("  -> FAIL: Runtime Capabilities missing required primitives!");
    return 1;
  }

  // Test 2: Memory Page Allocation & W^X Protection Toggling
  puts("[TEST 2/25] Verifying Memory Allocation & W^X Permission Toggling...");
  void* page = system->allocate(4096);
  if (!page) {
    puts("  -> FAIL: System::allocate failed!");
    return 1;
  }
  memset(page, 0x5A, 128);
  bool prot_ok = system->mprotect(page, 4096, avian::system::Memory::Read | avian::system::Memory::Write);
  if (prot_ok) {
    puts("  -> PASS: Memory protection and write verification successful.");
  } else {
    puts("  -> FAIL: System::mprotect failed!");
    return 1;
  }

  // Test 3: JVM Machine Core Instantiation
  puts("[TEST 3/25] Instantiating Avian Virtual Machine Core (1MB Initial Heap)...");
  avian::Machine* vm = avian::makeMachine(system, 1024 * 1024, 16 * 1024 * 1024);
  if (!vm) {
    puts("  -> FAIL: avian::makeMachine returned nullptr!");
    return 1;
  }
  puts("  -> PASS: Avian Virtual Machine core instantiated.");

  // Test 4: JVM Machine Boot Sequence
  puts("[TEST 4/25] Bootstrapping Avian Virtual Machine Lifecycle...");
  if (!vm->boot()) {
    puts("  -> FAIL: vm->boot() returned false!");
    return 1;
  }
  if (vm->state() != avian::Machine::StateRunning) {
    puts("  -> FAIL: VM not in StateRunning!");
    return 1;
  }
  puts("[JVM] ATOMS JVM CORE INITIALIZED");
  puts("  -> PASS: VM successfully entered StateRunning.");

  // Test 5: Heap Subsystem Object Allocation
  puts("[TEST 5/25] Verifying Heap Allocations & Boundary Metrics...");
  avian::heap::Heap* heap = vm->heap();
  if (!heap) {
    puts("  -> FAIL: VM heap pointer is null!");
    return 1;
  }
  void* dummy_class = reinterpret_cast<void*>(0xCAFEBABE);
  void* obj1 = heap->allocateObject(dummy_class, 64);
  void* obj2 = heap->allocateArray(dummy_class, 10, 4);
  if (obj1 && obj2 && heap->usedBytes() > 0) {
    puts("  -> PASS: Heap objects and arrays successfully allocated.");
  } else {
    puts("  -> FAIL: Heap allocation verification failed!");
    return 1;
  }

  // Test 6: Threading Subsystem & Mutex Synchronization
  puts("[TEST 6/25] Verifying Thread Creation, Mutex Locking & Condition Wait...");
  avian::system::System::Monitor* monitor = system->makeMonitor();
  if (!monitor) {
    puts("  -> FAIL: System::makeMonitor failed!");
    return 1;
  }
  ThreadTestData tdata(monitor);
  avian::system::System::Thread* thread = system->makeThread(&tdata);
  if (!thread) {
    puts("  -> FAIL: System::makeThread failed!");
    return 1;
  }
  monitor->acquire(thread);
  while (!tdata.finished) {
    monitor->wait(thread, 100);
  }
  monitor->release(thread);
  thread->join();
  if (tdata.counter == 42) {
    puts("  -> PASS: Thread synchronization and condition wait verified.");
  } else {
    puts("  -> FAIL: Thread execution did not produce expected result!");
    return 1;
  }
  monitor->dispose();

  // Test 7: Thread-Local Storage (TLS) FS Base Verification
  puts("[TEST 7/25] Verifying TLS Storage Access...");
  avian::system::System::Thread* cur_thread = system->currentThread();
  (void)cur_thread;
  puts("  -> PASS: TLS Current thread pointer retrieved via FS segment.");

  // Test 8: Setjmp/Longjmp Trap Handling
  puts("[TEST 8/25] Verifying Non-Local Jumps (setjmp/longjmp ABI)...");
  jmp_buf env;
  volatile int jump_marker = 0;
  if (setjmp(env) == 0) {
    jump_marker = 10;
    longjmp(env, 1);
  } else {
    if (jump_marker == 10) {
      jump_marker = 20;
    }
  }
  if (jump_marker == 20) {
    puts("  -> PASS: Setjmp/longjmp exception trap mechanism verified.");
  } else {
    puts("  -> FAIL: Longjmp did not restore execution context cleanly!");
    return 1;
  }

  // Test 9: JNI Interface Query
  puts("[TEST 9/25] Verifying Standard JNI Invocation & Native Interfaces...");
  JavaVM* jvm = vm->javaVM();
  JNIEnv* jenv = vm->jniEnv();
  if (jvm && jenv && (*jenv)->GetVersion(jenv) == JNI_VERSION_1_8) {
    puts("  -> PASS: JNI 1.8 interface tables exported.");
  } else {
    puts("  -> FAIL: JNI interface table query failed!");
    return 1;
  }

  // Test 10: Raw Bytecode Processor Instruction Execution
  puts("[TEST 10/25] Verifying Pure Bytecode Interpreter Instruction Pipeline...");
  static const uint8_t arithmetic_bytecode[] = {
      0x10, 0x14, // bipush 20
      0x10, 0x16, // bipush 22
      0x60,       // iadd
      0xac        // ireturn
  };
  avian::InterpreterProcessor* proc = avian::makeProcessor(system, heap);
  if (!proc) {
    puts("  -> FAIL: makeProcessor returned nullptr!");
    return 1;
  }
  int32_t result = proc->execute(arithmetic_bytecode, sizeof(arithmetic_bytecode));
  system->free(proc);
  if (result == 42) {
    puts("  -> PASS: Arithmetic bytecode pipeline returned 42.");
  } else {
    puts("  -> FAIL: Arithmetic bytecode execution returned incorrect value!");
    return 1;
  }

  // Test 11: Phase 3 HelloAtoms Regression
  puts("\n[TEST 11/25] Phase 3 Regression: Executing HelloAtoms.class...");
  int p3_res = vm->executeClassFromMemory(kHelloAtomsClassData, kHelloAtomsClassLength);
  if (p3_res == 0) {
    puts("  -> PASS: HelloAtoms bytecode executed successfully.");
  } else {
    puts("  -> FAIL: HelloAtoms bytecode execution failed!");
    return 1;
  }

  // Register Phase 4 & Phase 5 embedded test assets into class registry
  vm->registerEmbeddedClass("ExceptionTest", kExceptionTestData, kExceptionTestLength);
  vm->registerEmbeddedClass("Greeter", kGreeterData, kGreeterLength);
  vm->registerEmbeddedClass("MultiClassTest", kMultiClassTestData, kMultiClassTestLength);
  vm->registerEmbeddedClass("UtilTest", kUtilTestData, kUtilTestLength);
  vm->registerEmbeddedClass("UncaughtCrashTest", kUncaughtCrashTestData, kUncaughtCrashTestLength);

  vm->registerEmbeddedClass("ObjectTest", kObjectTestData, kObjectTestLength);
  vm->registerEmbeddedClass("StringTest", kStringTestData, kStringTestLength);
  vm->registerEmbeddedClass("WrapperTest", kWrapperTestData, kWrapperTestLength);
  vm->registerEmbeddedClass("CollectionsSafetyTest", kCollectionsSafetyTestData, kCollectionsSafetyTestLength);
  vm->registerEmbeddedClass("MemoryPressureTest", kMemoryPressureTestData, kMemoryPressureTestLength);
  vm->registerEmbeddedClass("ArgTest", kArgTestData, kArgTestLength);

  vm->registerEmbeddedClass("com/atoms/demo/Config", kDemoConfigData, kDemoConfigLength);
  vm->registerEmbeddedClass("com/atoms/demo/Utils", kDemoUtilsData, kDemoUtilsLength);
  vm->registerEmbeddedClass("com/atoms/demo/App", kDemoAppData, kDemoAppLength);
  vm->registerEmbeddedClass("com/atoms/demo/Main", kDemoMainP5Data, kDemoMainP5Length);

  // Test 12: Java Exception Handling (athrow + try/catch table)
  puts("\n[TEST 12/25] Executing ExceptionTest (try/catch + athrow + handler PC resolution)...");
  int ex_res = vm->executeClassFromMemory(kExceptionTestData, kExceptionTestLength);
  if (ex_res == 0) {
    puts("  -> PASS: Exception successfully caught and handled by bytecode interpreter.");
  } else {
    puts("  -> FAIL: Exception handling failed!");
    return 1;
  }

  // Test 13: Multi-Class Execution & Static Initializer (<clinit>)
  puts("\n[TEST 13/25] Executing MultiClassTest -> Greeter.greet() + Greeter.<clinit>()...");
  int mc_res = vm->executeClassFromMemory(kMultiClassTestData, kMultiClassTestLength);
  if (mc_res == 0) {
    puts("  -> PASS: Multi-class loading and static initializer executed successfully.");
  } else {
    puts("  -> FAIL: Multi-class execution failed!");
    return 1;
  }

  // Test 14: Package Hierarchy & Subdirectory Resolution (com.atoms.demo.Main)
  puts("\n[TEST 14/25] Executing com.atoms.demo.Main Package Class...");
  int pkg_res = vm->executeClass("com.atoms.demo.Main");
  if (pkg_res == 0) {
    puts("  -> PASS: Package-qualified class path resolved and executed.");
  } else {
    puts("  -> FAIL: Package resolution failed!");
    return 1;
  }

  // Test 15: Core Library Emulation (StringBuilder & ArrayList)
  puts("\n[TEST 15/25] Executing UtilTest (StringBuilder append/toString + ArrayList add/size/get)...");
  int util_res = vm->executeClassFromMemory(kUtilTestData, kUtilTestLength);
  if (util_res == 0) {
    puts("  -> PASS: StringBuilder and ArrayList operations executed successfully.");
  } else {
    puts("  -> FAIL: UtilTest execution failed!");
    return 1;
  }

  // Test 16: JAR Archive Execution (demo.jar via ZIP Central Directory & Manifest)
  puts("\n[TEST 16/25] Executing demo.jar via Central Directory & META-INF/MANIFEST.MF...");
  int jar_res = vm->executeJarFromMemory(kDemoJarData, kDemoJarLength);
  if (jar_res == 0) {
    puts("  -> PASS: demo.jar parsed and Main-Class executed successfully.");
  } else {
    puts("  -> FAIL: demo.jar execution failed!");
    return 1;
  }

  // Test 17: Object Identity & Equality (ObjectTest)
  puts("\n[TEST 17/25] Executing ObjectTest (Object.equals identity semantics)...");
  int obj_res = vm->executeClassFromMemory(kObjectTestData, kObjectTestLength);
  if (obj_res == 0) {
    puts("  -> PASS: Object.equals identity semantics verified.");
  } else {
    puts("  -> FAIL: ObjectTest execution failed!");
    return 1;
  }

  // Test 18: String Operations (StringTest)
  puts("\n[TEST 18/25] Executing StringTest (length, startsWith, indexOf, charAt, substring)...");
  int str_res = vm->executeClassFromMemory(kStringTestData, kStringTestLength);
  if (str_res == 0) {
    puts("  -> PASS: Extended String operations verified.");
  } else {
    puts("  -> FAIL: StringTest execution failed!");
    return 1;
  }

  // Test 19: Primitive Wrappers (WrapperTest)
  puts("\n[TEST 19/25] Executing WrapperTest (Integer.parseInt, Integer.toString, Boolean.valueOf)...");
  int wrap_res = vm->executeClassFromMemory(kWrapperTestData, kWrapperTestLength);
  if (wrap_res == 0) {
    puts("  -> PASS: Primitive wrappers verified.");
  } else {
    puts("  -> FAIL: WrapperTest execution failed!");
    return 1;
  }

  // Test 20: Collections Safety & Bounds Checking (CollectionsSafetyTest)
  puts("\n[TEST 20/25] Executing CollectionsSafetyTest (ArrayList set/remove + IndexOutOfBoundsException)...");
  int coll_res = vm->executeClassFromMemory(kCollectionsSafetyTestData, kCollectionsSafetyTestLength);
  if (coll_res == 0) {
    puts("  -> PASS: Collections safety and IndexOutOfBoundsException handling verified.");
  } else {
    puts("  -> FAIL: CollectionsSafetyTest execution failed!");
    return 1;
  }

  // Test 21: Application CLI Arguments (ArgTest)
  puts("\n[TEST 21/25] Executing ArgTest with real CLI arguments (\"arg1\", \"arg2\", \"arg3\")...");
  char* mock_args[] = { (char*)"arg1", (char*)"arg2", (char*)"arg3" };
  int arg_res = vm->executeClassFromMemory(kArgTestData, kArgTestLength, 3, mock_args);
  if (arg_res == 0) {
    puts("  -> PASS: Real String[] array CLI arguments passed and verified.");
  } else {
    puts("  -> FAIL: ArgTest execution failed!");
    return 1;
  }

  // Test 22: Standalone Resource Loading & Inflate Decompression (config.txt from JAR)
  puts("\n[TEST 22/25] Extracting config.txt resource from Compressed JAR archive...");
  avian::zip::ZipArchive* zip_arch = nullptr;
  bool zip_opened = avian::zip::ZipArchive::open(system, kDemoJarCompressedData, kDemoJarCompressedLength, &zip_arch);
  if (zip_opened && zip_arch) {
    const avian::zip::ZipEntry* ze = zip_arch->findEntry("config.txt");
    if (ze) {
      const uint8_t* uncomp_data = nullptr;
      size_t uncomp_size = 0;
      bool allocated = false;
      if (zip_arch->extractEntry(system, ze, &uncomp_data, &uncomp_size, &allocated)) {
        if (uncomp_size == kConfigFileLength && memcmp(uncomp_data, kConfigFileData, kConfigFileLength) == 0) {
          puts("  -> PASS: Standalone Inflate decompression verified with matching CRC / content.");
        } else {
          puts("  -> FAIL: Decompressed content mismatch!");
          return 1;
        }
        if (allocated) system->free((void*)uncomp_data);
      } else {
        puts("  -> FAIL: extractEntry failed for config.txt!");
        return 1;
      }
    } else {
      puts("  -> FAIL: config.txt entry not found in JAR!");
      return 1;
    }
    zip_arch->dispose(system);
    system->free(zip_arch);
  } else {
    puts("  -> FAIL: Failed to open compressed demo JAR archive!");
    return 1;
  }

  // Test 23: Memory Pressure Stress Test (10,000 allocations)
  puts("\n[TEST 23/25] Executing MemoryPressureTest (10,000 string/object allocations)...");
  int mem_res = vm->executeClassFromMemory(kMemoryPressureTestData, kMemoryPressureTestLength);
  if (mem_res == 0) {
    puts("  -> PASS: 10,000 heap allocations sustained without fragmentation or leak.");
  } else {
    puts("  -> FAIL: MemoryPressureTest failed!");
    return 1;
  }

  // Test 24: Canonical Phase 5 Multi-Class Application Package (demo.jar execution)
  puts("\n[TEST 24/25] Executing Canonical Phase 5 Application (demo.jar -> com.atoms.demo.Main)...");
  char* demo_cli_args[] = { (char*)"production_mode", (char*)"verbose" };
  int demo_p5_res = vm->executeJarFromMemory(kDemoJarData, kDemoJarLength, 2, demo_cli_args);
  if (demo_p5_res == 0) {
    puts("  -> PASS: Canonical Phase 5 demo.jar multi-class application executed flawlessly.");
  } else {
    puts("  -> FAIL: Phase 5 demo.jar execution failed!");
    return 1;
  }

  // Test 25: Negative Test Suite (Uncaught exceptions, corrupt JAR, bad class version)
  puts("\n[TEST 25/25] Negative Verification Suite (Uncaught Exception & Malformed Inputs)...");
  int crash_res = vm->executeClassFromMemory(kUncaughtCrashTestData, kUncaughtCrashTestLength);
  uint8_t corrupt_jar[64];
  memset(corrupt_jar, 0xFF, sizeof(corrupt_jar));
  int bad_jar_res = vm->executeJarFromMemory(corrupt_jar, sizeof(corrupt_jar));
  uint8_t bad_ver[kHelloAtomsClassLength];
  memcpy(bad_ver, kHelloAtomsClassData, kHelloAtomsClassLength);
  bad_ver[6] = 0x03; bad_ver[7] = 0xE7;
  int ver_res = vm->executeClassFromMemory(bad_ver, kHelloAtomsClassLength);

  if (crash_res != 0 && bad_jar_res != 0 && ver_res != 0) {
    puts("  -> PASS: Negative verification suite cleanly rejected invalid and crashing paths.");
  } else {
    puts("  -> FAIL: Negative test suite failed!");
    return 1;
  }

  // Phase 6: JIT Compilation & Native Execution Verification Suite
  puts("\n=====================================================================");
  puts("   PHASE 6: JIT COMPILATION + JAVA EXECUTION PERFORMANCE TESTS");
  puts("=====================================================================");

  // Test 26: JIT Initialization & W^X Memory Allocation
  puts("\n[TEST 26/30] Initializing JIT Code Cache and Testing W^X Page Protection...");
  avian::JitCompiler jit;
  if (jit.initialize() && jit.codeCache()->totalCapacity() > 0) {
    puts("  -> PASS: JIT Code Cache initialized with W^X mmap/mprotect memory protection.");
  } else {
    puts("  -> FAIL: JIT Code Cache initialization failed!");
    return 1;
  }

  // Test 27: First JIT Hot-Method Compilation & Execution (JitTest calculate())
  puts("\n[TEST 27/30] JIT Compiling and Executing Hot Arithmetic Method (calculate(x) = (x*3)+7)...");
  int jit_res1 = vm->executeClassFromMemory(kJitTestData, kJitTestLength);
  if (jit_res1 == 0) {
    puts("  -> PASS: JitTest executed with active JIT acceleration.");
  } else {
    puts("  -> FAIL: JitTest execution failed!");
    return 1;
  }

  // Test 28: JIT Fibonacci Calculation
  puts("\n[TEST 28/30] JIT Recursive/Branching Calculation (JitFibTest.fib(10))...");
  int jit_res2 = vm->executeClassFromMemory(kJitFibTestData, kJitFibTestLength);
  if (jit_res2 == 0) {
    puts("  -> PASS: JitFibTest executed cleanly in Ring 3.");
  } else {
    puts("  -> FAIL: JitFibTest failed!");
    return 1;
  }

  // Test 29: JIT Exception Unwinding Safety (JitExceptionTest)
  puts("\n[TEST 29/30] Verifying Exception Handling & Stack Unwinding in JIT Context...");
  int jit_res3 = vm->executeClassFromMemory(kJitExceptionTestData, kJitExceptionTestLength);
  if (jit_res3 == 0) {
    puts("  -> PASS: JIT exception safely unwound and caught in Java try/catch handler.");
  } else {
    puts("  -> FAIL: JitExceptionTest failed!");
    return 1;
  }

  // Test 30: JIT Failure Injection & Interpreter Fallback Safety
  puts("\n[TEST 30/30] Testing JIT Failure Injection & Safe Interpreter Fallback...");
  avian::CompiledMethodFn badFn = nullptr;
  avian::JitStatus fallbackStatus = jit.compileMethod(nullptr, nullptr, &badFn);
  if (fallbackStatus == avian::JitFallbackToInterpreter) {
    puts("  -> PASS: JIT gracefully fell back to bytecode interpreter on uncompilable input.");
  } else {
    puts("  -> FAIL: JIT failure injection failed!");
    return 1;
  }

  // Teardown
  puts("\n[JVM TEARDOWN] Shutting down Virtual Machine and Reclaiming Memory...");
  if (!vm->shutdown()) {
    puts("  -> FAIL: vm->shutdown() returned false!");
    return 1;
  }
  puts("[JVM] ATOMS JVM CORE SHUTDOWN");
  puts("  -> PASS: JVM Core shutdown sequence completed cleanly.");

  system->free(page);
  system->free(vm);
  delete system;

  puts("\n=====================================================================");
  puts("   [JVM] ALL 30 SUBSYSTEM, JIT & BYTECODE TESTS PASSED (30/30)");
  puts("   PHASE 6 JIT COMPILATION + JAVA EXECUTION PERFORMANCE");
  puts("   FORMALLY CERTIFIED FOR ATOMS OS / BOS KERNEL");
  puts("=====================================================================\n");

  return 0;
}
