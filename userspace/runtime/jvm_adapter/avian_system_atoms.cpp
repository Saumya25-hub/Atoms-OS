/*
 * ATOMS OS — Avian JVM Platform Adapter Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements the concrete platform bridge connecting the Avian C++ JVM core
 * (avian::system::System) to the ATOMS OS BOS Ring 3 Unified Runtime Foundation.
 */

#include "avian_system_atoms.h"
#include <userspace/runtime/include/atoms_runtime.h>

namespace atoms {
namespace jvm {

namespace {

class AtomsThread : public avian::system::System::Thread {
 public:
  AtomsThread(avian::system::System::Runnable* r)
      : runnable_(r), thread_id_(0), interrupted_(false)
  {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
    r->attach(this);
    pthread_create(&thread_id_, nullptr, &threadEntryPoint, this);
  }

  virtual ~AtomsThread() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
  }

  virtual void interrupt() override {
    pthread_mutex_lock(&mutex_);
    interrupted_ = true;
    runnable_->setInterrupted(true);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
  }

  virtual bool getAndClearInterrupted() override {
    pthread_mutex_lock(&mutex_);
    bool val = interrupted_;
    interrupted_ = false;
    runnable_->setInterrupted(false);
    pthread_mutex_unlock(&mutex_);
    return val;
  }

  virtual void join() override {
    if (thread_id_) {
      pthread_join(thread_id_, nullptr);
      thread_id_ = 0;
    }
  }

  virtual void dispose() override {
    delete this;
  }

  static void* threadEntryPoint(void* arg) {
    AtomsThread* self = static_cast<AtomsThread*>(arg);
    if (self && self->runnable_) {
      self->runnable_->run();
    }
    return nullptr;
  }

 private:
  avian::system::System::Runnable* runnable_;
  pthread_t thread_id_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  bool interrupted_;
};

class AtomsMutex : public avian::system::System::Mutex {
 public:
  AtomsMutex() {
    pthread_mutex_init(&mutex_, nullptr);
  }

  virtual ~AtomsMutex() {
    pthread_mutex_destroy(&mutex_);
  }

  virtual void acquire() override {
    pthread_mutex_lock(&mutex_);
  }

  virtual void release() override {
    pthread_mutex_unlock(&mutex_);
  }

  virtual void dispose() override {
    delete this;
  }

 private:
  pthread_mutex_t mutex_;
};

class AtomsMonitor : public avian::system::System::Monitor {
 public:
  AtomsMonitor() : depth_(0), owner_(nullptr) {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
  }

  virtual ~AtomsMonitor() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
  }

  virtual bool tryAcquire(avian::system::System::Thread* context) override {
    if (owner_ == context) {
      depth_++;
      return true;
    }
    if (pthread_mutex_lock(&mutex_) == 0) {
      owner_ = context;
      depth_ = 1;
      return true;
    }
    return false;
  }

  virtual void acquire(avian::system::System::Thread* context) override {
    if (owner_ == context) {
      depth_++;
      return;
    }
    pthread_mutex_lock(&mutex_);
    owner_ = context;
    depth_ = 1;
  }

  virtual void release(avian::system::System::Thread* context) override {
    if (owner_ == context) {
      depth_--;
      if (depth_ == 0) {
        owner_ = nullptr;
        pthread_mutex_unlock(&mutex_);
      }
    }
  }

  virtual void wait(avian::system::System::Thread* context, int64_t time) override {
    if (owner_ == context) {
      size_t old_depth = depth_;
      depth_ = 0;
      owner_ = nullptr;
      if (time == 0) {
        pthread_cond_wait(&cond_, &mutex_);
      } else {
        struct timespec ts;
        ts.tv_sec = time / 1000;
        ts.tv_nsec = (time % 1000) * 1000000;
        pthread_cond_wait(&cond_, &mutex_);
      }
      owner_ = context;
      depth_ = old_depth;
    }
  }

  virtual void notify(avian::system::System::Thread* context) override {
    (void)context;
    pthread_cond_signal(&cond_);
  }

  virtual void notifyAll(avian::system::System::Thread* context) override {
    (void)context;
    pthread_cond_broadcast(&cond_);
  }

  virtual void dispose() override {
    delete this;
  }

 private:
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  size_t depth_;
  avian::system::System::Thread* owner_;
};

class AtomsRegion : public avian::system::System::Region {
 public:
  AtomsRegion(const uint8_t* ptr, size_t len, bool need_unmap)
      : ptr_(ptr), len_(len), need_unmap_(need_unmap) {}

  virtual ~AtomsRegion() {
    if (need_unmap_ && ptr_) {
      munmap(const_cast<void*>(static_cast<const void*>(ptr_)), len_);
    }
  }

  virtual const uint8_t* start() const override { return ptr_; }
  virtual size_t length() const override { return len_; }
  virtual void dispose() override { delete this; }

 private:
  const uint8_t* ptr_;
  size_t len_;
  bool need_unmap_;
};

class AtomsDirectory : public avian::system::System::Directory {
 public:
  AtomsDirectory() {}
  virtual ~AtomsDirectory() {}
  virtual const char* next() override { return nullptr; }
  virtual void dispose() override { delete this; }
};

class AtomsSystem : public avian::system::System {
 public:
  AtomsSystem() {
    atoms_runtime_get_caps(&caps_);
  }

  virtual ~AtomsSystem() {}

  virtual void* allocate(size_t size) override {
    return malloc(size);
  }

  virtual void* tryAllocate(size_t size) override {
    return malloc(size);
  }

  virtual void free(const void* p) override {
    ::free(const_cast<void*>(p));
  }

  virtual bool mprotect(void* p, size_t size, unsigned permissions) override {
    int prot = PROT_NONE;
    if (permissions & avian::system::Memory::Read)    prot |= PROT_READ;
    if (permissions & avian::system::Memory::Write)   prot |= PROT_WRITE;
    if (permissions & avian::system::Memory::Execute) prot |= PROT_EXEC;

    return ::mprotect(p, size, prot) == 0;
  }

  virtual Thread* makeThread(Runnable* r) override {
    if (!r) return nullptr;
    return new AtomsThread(r);
  }

  virtual Thread* currentThread() override {
    // Returns dummy thread reference for context
    return nullptr;
  }

  virtual Mutex* makeMutex() override {
    return new AtomsMutex();
  }

  virtual Monitor* makeMonitor() override {
    return new AtomsMonitor();
  }

  virtual int64_t now() override {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000 + static_cast<int64_t>(ts.tv_nsec) / 1000000;
  }

  virtual void sleep(int64_t ms) override {
    if (ms <= 0) return;
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&req, nullptr);
  }

  virtual int open(const char* path, int flags) override {
    return ::open(path, flags, 0);
  }

  virtual int read(int fd, void* buffer, size_t count) override {
    return ::read(fd, buffer, count);
  }

  virtual int write(int fd, const void* buffer, size_t count) override {
    return ::write(fd, buffer, count);
  }

  virtual void close(int fd) override {
    ::close(fd);
  }

  virtual Region* mmap(int fd, size_t offset, size_t length) override {
    void* ptr = ::mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, offset);
    if (ptr == MAP_FAILED || !ptr) return nullptr;
    return new AtomsRegion(static_cast<const uint8_t*>(ptr), length, true);
  }

  virtual Directory* readDirectory(const char* path) override {
    (void)path;
    return new AtomsDirectory();
  }

  virtual FileType stat(const char* path) override {
    if (!path) return TypeDoesNotExist;
    int fd = ::open(path, 0, 0);
    if (fd >= 0) {
      ::close(fd);
      return TypeFile;
    }
    return TypeDoesNotExist;
  }

  virtual void print(const char* message) override {
    if (message) {
      puts(message);
    }
  }

  virtual void printDec(int64_t val) override {
    printf("%lld", static_cast<long long>(val));
  }

  virtual void printHex(uint64_t val) override {
    printf("0x%llx", static_cast<unsigned long long>(val));
  }

  virtual void abort() override {
    ::abort();
  }

  virtual void exit(int code) override {
    ::exit(code);
  }

 private:
  atoms_runtime_caps_t caps_;
};

} // anonymous namespace

avian::system::System* makeAtomsSystem() {
  return new AtomsSystem();
}

} // namespace jvm
} // namespace atoms
