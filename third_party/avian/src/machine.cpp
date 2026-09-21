/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#include <avian/machine.h>
#include <avian/system/system.h>
#include <avian/heap/heap.h>
#include <avian/jni.h>
#include <avian/zip.h>
#include <stdio.h>
#include <string.h>
#include <userspace/runtime/cpp/include/new>

namespace avian {

extern int executeBytecodeMethod(system::System* s, heap::Heap* h, Machine* m, ClassFile* cls, const MethodInfo* method, void* argsArray);

// Structure for Java String object representation
struct JavaString {
  const char* utf8_bytes;
  uint16_t length;
};

// Structure for Java String[] array representation on heap
struct JavaStringArray {
  uint32_t length;
  intptr_t elements[1]; // dynamic array
};

namespace {

struct LoadedClassEntry {
  char name[128];
  ClassFile* class_file;
  bool clinit_executed;
};

struct EmbeddedClassEntry {
  char name[128];
  const uint8_t* data;
  size_t length;
};

struct StaticFieldEntry {
  char class_name[128];
  char field_name[64];
  intptr_t value;
};

class AvianMachine : public Machine {
 public:
  AvianMachine(system::System* s, size_t heapInit, size_t heapMax)
      : system_(s),
        heap_(nullptr),
        state_(StateUninitialized),
        heap_init_bytes_(heapInit),
        heap_max_bytes_(heapMax),
        loaded_class_count_(0),
        embedded_class_count_(0),
        static_field_count_(0),
        active_jar_archive_(nullptr)
  {
    for (size_t i = 0; i < 64; ++i) {
      loaded_classes_[i].name[0] = '\0';
      loaded_classes_[i].class_file = nullptr;
      loaded_classes_[i].clinit_executed = false;
    }
    for (size_t i = 0; i < 32; ++i) {
      embedded_classes_[i].name[0] = '\0';
      embedded_classes_[i].data = nullptr;
      embedded_classes_[i].length = 0;
    }
    for (size_t i = 0; i < 128; ++i) {
      static_fields_[i].class_name[0] = '\0';
      static_fields_[i].field_name[0] = '\0';
      static_fields_[i].value = 0;
    }
    initJni();
  }

  virtual ~AvianMachine() {
    if (state_ != StateTerminated) {
      shutdown();
    }
  }

  virtual bool boot() override {
    if (state_ != StateUninitialized) return false;

    state_ = StateBooting;
    if (system_) {
      system_->print("[JVM:Core] Bootstrapping Avian Virtual Machine...\n");
    }

    // Initialize Heap Subsystem
    heap_ = heap::makeHeap(system_, heap_init_bytes_, heap_max_bytes_);
    if (!heap_) {
      if (system_) {
        system_->print("[JVM:Core] Failed to allocate virtual machine heap!\n");
      }
      state_ = StateTerminated;
      return false;
    }

    state_ = StateRunning;
    if (system_) {
      system_->print("[JVM:Core] Avian VM successfully booted in pure interpreter mode.\n");
    }
    return true;
  }

  virtual bool shutdown() override {
    if (state_ == StateTerminated) return true;

    state_ = StateShuttingDown;
    if (system_) {
      system_->print("[JVM:Core] Shutting down Avian Virtual Machine...\n");
    }

    for (size_t i = 0; i < loaded_class_count_; ++i) {
      if (loaded_classes_[i].class_file) {
        loaded_classes_[i].class_file->dispose(system_);
        system_->free(loaded_classes_[i].class_file);
        loaded_classes_[i].class_file = nullptr;
      }
    }
    loaded_class_count_ = 0;

    if (heap_) {
      heap_->~Heap();
      if (system_) {
        system_->free(heap_);
      }
      heap_ = nullptr;
    }

    state_ = StateTerminated;
    if (system_) {
      system_->print("[JVM:Core] Virtual Machine shutdown complete. All resources reclaimed.\n");
    }
    return true;
  }

  virtual State state() const override { return state_; }
  virtual system::System* system() const override { return system_; }
  virtual heap::Heap* heap() const override { return heap_; }

  virtual JavaVM* javaVM() override { return &jvm_interface_; }
  virtual JNIEnv* jniEnv() override { return &env_interface_; }

  virtual void registerEmbeddedClass(const char* className, const uint8_t* data, size_t length) override {
    if (!className || !data || length == 0 || embedded_class_count_ >= 32) return;
    char normalized[128];
    normalizeClassName(className, normalized, sizeof(normalized));
    strncpy(embedded_classes_[embedded_class_count_].name, normalized, 127);
    embedded_classes_[embedded_class_count_].name[127] = '\0';
    embedded_classes_[embedded_class_count_].data = data;
    embedded_classes_[embedded_class_count_].length = length;
    ++embedded_class_count_;
  }

  virtual void setStaticField(const char* className, const char* fieldName, intptr_t value) override {
    if (!className || !fieldName) return;
    char norm[128];
    normalizeClassName(className, norm, sizeof(norm));

    for (size_t i = 0; i < static_field_count_; ++i) {
      if (strcmp(static_fields_[i].class_name, norm) == 0 &&
          strcmp(static_fields_[i].field_name, fieldName) == 0) {
        static_fields_[i].value = value;
        return;
      }
    }

    if (static_field_count_ < 128) {
      strncpy(static_fields_[static_field_count_].class_name, norm, 127);
      static_fields_[static_field_count_].class_name[127] = '\0';
      strncpy(static_fields_[static_field_count_].field_name, fieldName, 63);
      static_fields_[static_field_count_].field_name[63] = '\0';
      static_fields_[static_field_count_].value = value;
      ++static_field_count_;
    }
  }

  virtual bool getStaticField(const char* className, const char* fieldName, intptr_t* outValue) override {
    if (!className || !fieldName || !outValue) return false;
    char norm[128];
    normalizeClassName(className, norm, sizeof(norm));

    for (size_t i = 0; i < static_field_count_; ++i) {
      if (strcmp(static_fields_[i].class_name, norm) == 0 &&
          strcmp(static_fields_[i].field_name, fieldName) == 0) {
        *outValue = static_fields_[i].value;
        return true;
      }
    }
    return false;
  }

  virtual bool getResource(const char* resourcePath, const uint8_t** outData, size_t* outSize, bool* outAllocated) override {
    if (!resourcePath || !outData || !outSize) return false;
    if (outAllocated) *outAllocated = false;

    // 1. If an active JAR archive is loaded, try finding inside JAR
    if (active_jar_archive_) {
      const zip::ZipEntry* ze = active_jar_archive_->findEntry(resourcePath);
      if (!ze && resourcePath[0] == '/') {
        ze = active_jar_archive_->findEntry(resourcePath + 1);
      }
      if (ze) {
        return active_jar_archive_->extractEntry(system_, ze, outData, outSize, outAllocated);
      }
    }

    // 2. Search BOFS / VFS
    if (system_) {
      int fd = system_->open(resourcePath, 0);
      if (fd < 0 && resourcePath[0] != '/') {
        char fallback[256];
        snprintf(fallback, sizeof(fallback), "/apps/%s", resourcePath);
        fd = system_->open(fallback, 0);
      }
      if (fd >= 0) {
        const size_t max_res_bytes = 128 * 1024;
        uint8_t* buf = static_cast<uint8_t*>(system_->allocate(max_res_bytes));
        if (buf) {
          int bytes_read = system_->read(fd, buf, max_res_bytes);
          system_->close(fd);
          if (bytes_read >= 0) {
            *outData = buf;
            *outSize = static_cast<size_t>(bytes_read);
            if (outAllocated) *outAllocated = true;
            return true;
          }
          system_->free(buf);
        } else {
          system_->close(fd);
        }
      }
    }

    return false;
  }

  virtual ClassFile* loadClass(const char* className) override {
    if (!className || !system_) return nullptr;

    char normalized[128];
    normalizeClassName(className, normalized, sizeof(normalized));

    // Check if already loaded
    for (size_t i = 0; i < loaded_class_count_; ++i) {
      if (strcmp(loaded_classes_[i].name, normalized) == 0) {
        return loaded_classes_[i].class_file;
      }
    }

    // 1. Search Embedded Assets Registry
    for (size_t i = 0; i < embedded_class_count_; ++i) {
      if (strcmp(embedded_classes_[i].name, normalized) == 0) {
        ClassFile* cls = nullptr;
        ParseStatus status = ClassFile::parse(system_, embedded_classes_[i].data, embedded_classes_[i].length, &cls);
        if (status == ParseSuccess && cls) {
          registerLoadedClass(normalized, cls);
          triggerStaticInitializer(cls, normalized);
          return cls;
        }
      }
    }

    // 2. Search BOFS / VFS
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "/apps/%s.class", normalized);
    int fd = system_->open(filePath, 0);
    if (fd < 0) {
      snprintf(filePath, sizeof(filePath), "%s.class", normalized);
      fd = system_->open(filePath, 0);
    }
    if (fd < 0) {
      snprintf(filePath, sizeof(filePath), "build/%s.class", normalized);
      fd = system_->open(filePath, 0);
    }

    if (fd >= 0) {
      const size_t max_class_bytes = 64 * 1024;
      uint8_t* class_buf = static_cast<uint8_t*>(system_->allocate(max_class_bytes));
      if (class_buf) {
        int bytes_read = system_->read(fd, class_buf, max_class_bytes);
        system_->close(fd);
        if (bytes_read > 0) {
          ClassFile* cls = nullptr;
          ParseStatus status = ClassFile::parse(system_, class_buf, static_cast<size_t>(bytes_read), &cls);
          system_->free(class_buf);
          if (status == ParseSuccess && cls) {
            registerLoadedClass(normalized, cls);
            triggerStaticInitializer(cls, normalized);
            return cls;
          }
        } else {
          system_->free(class_buf);
        }
      } else {
        system_->close(fd);
      }
    }

    return nullptr;
  }

  void* buildJavaStringArray(int argc, char** argv) {
    if (!heap_) return nullptr;

    size_t array_len = (argc > 0 && argv) ? static_cast<size_t>(argc) : 0;
    void* str_array_class = reinterpret_cast<void*>(0xCAFE0003);
    void* str_class = reinterpret_cast<void*>(0xCAFE0002);

    size_t alloc_size = sizeof(JavaStringArray) + (array_len > 0 ? (array_len - 1) * sizeof(intptr_t) : 0);
    JavaStringArray* arr = static_cast<JavaStringArray*>(heap_->allocateObject(str_array_class, alloc_size));
    if (!arr) return nullptr;

    arr->length = static_cast<uint32_t>(array_len);
    for (size_t i = 0; i < array_len; ++i) {
      if (argv[i]) {
        size_t slen = strlen(argv[i]);
        JavaString* jstr = static_cast<JavaString*>(heap_->allocateObject(str_class, sizeof(JavaString)));
        if (jstr) {
          jstr->utf8_bytes = argv[i];
          jstr->length = static_cast<uint16_t>(slen);
          arr->elements[i] = reinterpret_cast<intptr_t>(jstr);
        } else {
          arr->elements[i] = 0;
        }
      } else {
        arr->elements[i] = 0;
      }
    }

    return arr;
  }

  virtual int executeClass(const char* classPath, int argc = 0, char** argv = nullptr) override {
    if (!classPath || !system_) {
      if (system_) system_->print("[JVM ERROR] Invalid class path\n");
      return 1;
    }

    // Check if class is embedded first
    char norm[128];
    normalizeClassName(classPath, norm, sizeof(norm));
    for (size_t i = 0; i < embedded_class_count_; ++i) {
      if (strcmp(embedded_classes_[i].name, norm) == 0) {
        return executeClassFromMemory(embedded_classes_[i].data, embedded_classes_[i].length, argc, argv);
      }
    }

    int fd = system_->open(classPath, 0);
    if (fd < 0) {
      char fallback[256];
      snprintf(fallback, sizeof(fallback), "/apps/%s", classPath);
      fd = system_->open(fallback, 0);
    }
    if (fd < 0) {
      char fallback[256];
      snprintf(fallback, sizeof(fallback), "/%s", classPath);
      fd = system_->open(fallback, 0);
    }

    if (fd < 0) {
      if (system_) {
        system_->print("[JVM ERROR] Class not found: ");
        system_->print(classPath);
        system_->print("\n");
      }
      return 1;
    }

    const size_t max_class_bytes = 64 * 1024;
    uint8_t* class_buf = static_cast<uint8_t*>(system_->allocate(max_class_bytes));
    if (!class_buf) {
      system_->close(fd);
      return 1;
    }

    int bytes_read = system_->read(fd, class_buf, max_class_bytes);
    system_->close(fd);

    if (bytes_read <= 0) {
      system_->free(class_buf);
      if (system_) system_->print("[JVM ERROR] Empty or unreadable class file\n");
      return 1;
    }

    int res = executeClassFromMemory(class_buf, static_cast<size_t>(bytes_read), argc, argv);
    system_->free(class_buf);
    return res;
  }

  virtual int executeClassFromMemory(const uint8_t* data, size_t length, int argc = 0, char** argv = nullptr) override {
    if (!data || length == 0 || !system_) return 1;

    ClassFile* cls = nullptr;
    ParseStatus status = ClassFile::parse(system_, data, length, &cls);

    if (status != ParseSuccess) {
      switch (status) {
        case ParseErrorInvalidMagic:
          system_->print("[JVM ERROR] Invalid class file magic\n");
          break;
        case ParseErrorUnsupportedVersion:
          system_->print("[JVM ERROR] Unsupported class file version\n");
          break;
        case ParseErrorTruncated:
        case ParseErrorInvalidConstantPool:
          system_->print("[JVM ERROR] Malformed class file\n");
          break;
        default:
          system_->print("[JVM ERROR] Class parsing failed\n");
          break;
      }
      return 1;
    }

    const char* cname = cls->getClassName(cls->thisClass());
    if (!cname) cname = "UnknownClass";

    if (system_) {
      system_->print("[JVM] Loading ");
      system_->print(cname);
      system_->print("\n");
      system_->print("[JVM] Resolving main()\n");
    }

    // Register class in loaded table
    char norm[128];
    normalizeClassName(cname, norm, sizeof(norm));
    registerLoadedClass(norm, cls);

    // Trigger static initializer if present
    triggerStaticInitializer(cls, norm);

    const MethodInfo* main_method = cls->findMethod("main", "([Ljava/lang/String;)V");
    if (!main_method || !main_method->has_code) {
      system_->print("[JVM ERROR] Main method not found in class\n");
      return 1;
    }

    void* args_array = buildJavaStringArray(argc, argv);

    int res = executeBytecodeMethod(system_, heap_, this, cls, main_method, args_array);

    if (system_ && res == 0) {
      system_->print("[JVM] Java application exited with status 0\n");
    }

    return res;
  }

  virtual int executeJar(const char* jarPath, int argc = 0, char** argv = nullptr) override {
    if (!jarPath || !system_) {
      if (system_) system_->print("[JVM ERROR] Invalid jar path\n");
      return 1;
    }

    int fd = system_->open(jarPath, 0);
    if (fd < 0) {
      char fallback[256];
      snprintf(fallback, sizeof(fallback), "/apps/%s", jarPath);
      fd = system_->open(fallback, 0);
    }

    if (fd < 0) {
      if (system_) {
        system_->print("[JVM ERROR] JAR archive not found: ");
        system_->print(jarPath);
        system_->print("\n");
      }
      return 1;
    }

    const size_t max_jar_bytes = 512 * 1024;
    uint8_t* jar_buf = static_cast<uint8_t*>(system_->allocate(max_jar_bytes));
    if (!jar_buf) {
      system_->close(fd);
      return 1;
    }

    int bytes_read = system_->read(fd, jar_buf, max_jar_bytes);
    system_->close(fd);

    if (bytes_read <= 0) {
      system_->free(jar_buf);
      if (system_) system_->print("[JVM ERROR] Empty or unreadable JAR archive\n");
      return 1;
    }

    int res = executeJarFromMemory(jar_buf, static_cast<size_t>(bytes_read), argc, argv);
    system_->free(jar_buf);
    return res;
  }

  virtual int executeJarFromMemory(const uint8_t* jarData, size_t jarLength, int argc = 0, char** argv = nullptr) override {
    if (!jarData || jarLength == 0 || !system_) return 1;

    zip::ZipArchive* archive = nullptr;
    if (!zip::ZipArchive::open(system_, jarData, jarLength, &archive)) {
      if (system_) system_->print("[JVM ERROR] Failed to parse ZIP / JAR Central Directory\n");
      return 1;
    }

    active_jar_archive_ = archive;

    char main_class_name[128];
    if (!archive->getMainClass(main_class_name, sizeof(main_class_name))) {
      if (system_) system_->print("[JVM ERROR] Main-Class attribute not found in META-INF/MANIFEST.MF\n");
      active_jar_archive_ = nullptr;
      archive->dispose(system_);
      system_->free(archive);
      return 1;
    }

    if (system_) {
      system_->print("[JVM] JAR Main-Class resolved: ");
      system_->print(main_class_name);
      system_->print("\n");
    }

    // Register all class entries inside JAR into embedded class registry
    for (uint16_t i = 0; i < archive->entryCount(); ++i) {
      const zip::ZipEntry* ze = archive->getEntry(i);
      if (ze && ze->filename && ze->filename_length > 6) {
        if (strncmp(ze->filename + ze->filename_length - 6, ".class", 6) == 0) {
          char cname[128];
          size_t clen = ze->filename_length - 6;
          if (clen >= sizeof(cname)) clen = sizeof(cname) - 1;
          strncpy(cname, ze->filename, clen);
          cname[clen] = '\0';

          const uint8_t* cdata = nullptr;
          size_t csize = 0;
          bool allocated = false;
          if (archive->extractEntry(system_, ze, &cdata, &csize, &allocated)) {
            registerEmbeddedClass(cname, cdata, csize);
            // If extracted into dynamic buffer, we don't immediately free if registered as embedded reference
          }
        }
      }
    }

    // Load and execute main class
    ClassFile* main_cls = loadClass(main_class_name);
    if (!main_cls) {
      if (system_) system_->print("[JVM ERROR] Failed to load main class from JAR\n");
      active_jar_archive_ = nullptr;
      archive->dispose(system_);
      system_->free(archive);
      return 1;
    }

    const MethodInfo* main_method = main_cls->findMethod("main", "([Ljava/lang/String;)V");
    if (!main_method || !main_method->has_code) {
      if (system_) system_->print("[JVM ERROR] Main method not found in JAR main class\n");
      active_jar_archive_ = nullptr;
      archive->dispose(system_);
      system_->free(archive);
      return 1;
    }

    void* args_array = buildJavaStringArray(argc, argv);

    int res = executeBytecodeMethod(system_, heap_, this, main_cls, main_method, args_array);

    active_jar_archive_ = nullptr;
    archive->dispose(system_);
    system_->free(archive);

    if (system_ && res == 0) {
      system_->print("[JVM] JAR application exited with status 0\n");
    }

    return res;
  }

 private:
  void normalizeClassName(const char* in, char* out, size_t maxLen) {
    if (!in || !out || maxLen == 0) return;
    size_t out_idx = 0;
    size_t in_len = strlen(in);

    // Strip leading / or ./
    size_t start = 0;
    if (in_len >= 2 && in[0] == '.' && in[1] == '/') start = 2;
    else if (in[0] == '/') start = 1;

    // Strip trailing .class if present
    size_t end = in_len;
    if (end >= 6 && strcmp(in + end - 6, ".class") == 0) {
      end -= 6;
    }

    for (size_t i = start; i < end && out_idx + 1 < maxLen; ++i) {
      if (in[i] == '.') {
        out[out_idx++] = '/';
      } else {
        out[out_idx++] = in[i];
      }
    }
    out[out_idx] = '\0';
  }

  void registerLoadedClass(const char* name, ClassFile* cls) {
    for (size_t i = 0; i < loaded_class_count_; ++i) {
      if (strcmp(loaded_classes_[i].name, name) == 0) {
        loaded_classes_[i].class_file = cls;
        return;
      }
    }
    if (loaded_class_count_ < 64) {
      strncpy(loaded_classes_[loaded_class_count_].name, name, 127);
      loaded_classes_[loaded_class_count_].name[127] = '\0';
      loaded_classes_[loaded_class_count_].class_file = cls;
      loaded_classes_[loaded_class_count_].clinit_executed = false;
      ++loaded_class_count_;
    }
  }

  void triggerStaticInitializer(ClassFile* cls, const char* name) {
    for (size_t i = 0; i < loaded_class_count_; ++i) {
      if (strcmp(loaded_classes_[i].name, name) == 0) {
        if (!loaded_classes_[i].clinit_executed) {
          loaded_classes_[i].clinit_executed = true;
          const MethodInfo* clinit = cls->findMethod("<clinit>", "()V");
          if (clinit && clinit->has_code) {
            executeBytecodeMethod(system_, heap_, this, cls, clinit, nullptr);
          }
        }
        break;
      }
    }
  }

  void initJni() {
    jvm_invoke_table_.reserved0 = nullptr;
    jvm_invoke_table_.reserved1 = nullptr;
    jvm_invoke_table_.reserved2 = nullptr;
    jvm_invoke_table_.DestroyJavaVM = &jniDestroyJavaVM;
    jvm_invoke_table_.AttachCurrentThread = &jniAttachCurrentThread;
    jvm_invoke_table_.DetachCurrentThread = &jniDetachCurrentThread;
    jvm_invoke_table_.GetEnv = &jniGetEnv;
    jvm_invoke_table_.AttachCurrentThreadAsDaemon = &jniAttachCurrentThread;

    jvm_interface_ = &jvm_invoke_table_;

    env_native_table_.reserved0 = nullptr;
    env_native_table_.reserved1 = nullptr;
    env_native_table_.reserved2 = nullptr;
    env_native_table_.reserved3 = nullptr;
    env_native_table_.GetVersion = &jniGetVersion;
    env_native_table_.FindClass = &jniFindClass;
    env_native_table_.Throw = &jniThrow;
    env_native_table_.ThrowNew = &jniThrowNew;
    env_native_table_.ExceptionOccurred = &jniExceptionOccurred;
    env_native_table_.ExceptionDescribe = &jniExceptionDescribe;
    env_native_table_.ExceptionClear = &jniExceptionClear;
    env_native_table_.FatalError = &jniFatalError;

    env_interface_ = &env_native_table_;
  }

  static jint jniDestroyJavaVM(JavaVM* vm) {
    if (!vm) return JNI_ERR;
    return JNI_OK;
  }

  static jint jniAttachCurrentThread(JavaVM* vm, void** penv, void* args) {
    (void)vm; (void)penv; (void)args;
    return JNI_OK;
  }

  static jint jniDetachCurrentThread(JavaVM* vm) {
    (void)vm;
    return JNI_OK;
  }

  static jint jniGetEnv(JavaVM* vm, void** penv, jint version) {
    (void)vm; (void)version;
    if (penv) *penv = nullptr;
    return JNI_OK;
  }

  static jint jniGetVersion(JNIEnv* env) {
    (void)env;
    return JNI_VERSION_1_8;
  }

  static jclass jniFindClass(JNIEnv* env, const char* name) {
    (void)env; (void)name;
    return nullptr;
  }

  static jint jniThrow(JNIEnv* env, jthrowable obj) {
    (void)env; (void)obj;
    return JNI_OK;
  }

  static jint jniThrowNew(JNIEnv* env, jclass clazz, const char* message) {
    (void)env; (void)clazz; (void)message;
    return JNI_OK;
  }

  static jthrowable jniExceptionOccurred(JNIEnv* env) {
    (void)env;
    return nullptr;
  }

  static void jniExceptionDescribe(JNIEnv* env) {
    (void)env;
  }

  static void jniExceptionClear(JNIEnv* env) {
    (void)env;
  }

  static void jniFatalError(JNIEnv* env, const char* message) {
    (void)env; (void)message;
  }

  system::System* system_;
  heap::Heap* heap_;
  State state_;
  size_t heap_init_bytes_;
  size_t heap_max_bytes_;

  LoadedClassEntry loaded_classes_[64];
  size_t loaded_class_count_;

  EmbeddedClassEntry embedded_classes_[32];
  size_t embedded_class_count_;

  StaticFieldEntry static_fields_[128];
  size_t static_field_count_;

  zip::ZipArchive* active_jar_archive_;

  struct JNIInvokeInterface_ jvm_invoke_table_;
  struct JNINativeInterface_ env_native_table_;
  JavaVM jvm_interface_;
  JNIEnv env_interface_;
};

} // anonymous namespace

Machine* makeMachine(system::System* s, size_t heapInitialBytes, size_t heapMaxBytes) {
  if (!s) return nullptr;
  void* mem = s->allocate(sizeof(AvianMachine));
  if (!mem) return nullptr;
  return new (mem) AvianMachine(s, heapInitialBytes, heapMaxBytes);
}

} // namespace avian

extern "C" {

jint JNI_CreateJavaVM(JavaVM** pvm, void** penv, void* args) {
  (void)args;
  if (!pvm || !penv) return JNI_ERR;
  return JNI_OK;
}

jint JNI_GetDefaultJavaVMInitArgs(void* args) {
  if (!args) return JNI_ERR;
  JavaVMInitArgs* init = static_cast<JavaVMInitArgs*>(args);
  init->version = JNI_VERSION_1_8;
  init->nOptions = 0;
  init->options = nullptr;
  init->ignoreUnrecognized = JNI_TRUE;
  return JNI_OK;
}

jint JNI_GetCreatedJavaVMs(JavaVM** vmBuf, jsize bufLen, jsize* nVMs) {
  if (!vmBuf || bufLen <= 0 || !nVMs) return JNI_ERR;
  *nVMs = 0;
  return JNI_OK;
}

} // extern "C"
