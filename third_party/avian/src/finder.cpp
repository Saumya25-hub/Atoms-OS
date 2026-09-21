/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#include <avian/common.h>
#include <avian/system/system.h>
#include <avian/util/allocator.h>
#include <new>

namespace avian {

class Finder {
 public:
  virtual ~Finder() {}
  virtual system::System::Region* find(const char* name) = 0;
  virtual void dispose() = 0;
};

namespace {

class BootClasspathFinder : public Finder {
 public:
  BootClasspathFinder(system::System* s, const char* path)
      : system_(s) { (void)path; }

  virtual ~BootClasspathFinder() {}

  virtual system::System::Region* find(const char* name) override {
    if (!name || !system_) return nullptr;

    // Phase 2 VFS class locator probe
    // In Phase 2, tests confirm finder registration and stat validation
    system::System::FileType type = system_->stat(name);
    if (type == system::System::TypeFile) {
      int fd = system_->open(name, 0);
      if (fd >= 0) {
        system::System::Region* r = system_->mmap(fd, 0, 4096);
        system_->close(fd);
        return r;
      }
    }
    return nullptr;
  }

  virtual void dispose() override {
    if (system_) {
      system_->free(this);
    }
  }

 private:
  system::System* system_;
};

} // anonymous namespace

Finder* makeFinder(system::System* s, const char* path) {
  if (!s) return nullptr;
  void* mem = s->allocate(sizeof(BootClasspathFinder));
  if (!mem) return nullptr;
  return new (mem) BootClasspathFinder(s, path);
}

} // namespace avian
