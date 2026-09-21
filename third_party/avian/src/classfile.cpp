/*
 * ATOMS OS — Java Classfile Binary Parser Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements strict, bounds-checked binary parsing of JVMS 8 class files.
 */

#include <avian/classfile.h>
#include <avian/util/allocator.h>
#include <string.h>
#include <new>

namespace avian {

namespace {

struct ByteReader {
  const uint8_t* ptr;
  size_t rem;

  ByteReader(const uint8_t* p, size_t r) : ptr(p), rem(r) {}

  bool readU8(uint8_t* val) {
    if (rem < 1) return false;
    *val = *ptr++;
    rem -= 1;
    return true;
  }

  bool readU16(uint16_t* val) {
    if (rem < 2) return false;
    *val = (static_cast<uint16_t>(ptr[0]) << 8) | static_cast<uint16_t>(ptr[1]);
    ptr += 2;
    rem -= 2;
    return true;
  }

  bool readU32(uint32_t* val) {
    if (rem < 4) return false;
    *val = (static_cast<uint32_t>(ptr[0]) << 24) |
           (static_cast<uint32_t>(ptr[1]) << 16) |
           (static_cast<uint32_t>(ptr[2]) << 8)  |
           static_cast<uint32_t>(ptr[3]);
    ptr += 4;
    rem -= 4;
    return true;
  }

  bool readBytes(const uint8_t** out_ptr, size_t count) {
    if (rem < count) return false;
    *out_ptr = ptr;
    ptr += count;
    rem -= count;
    return true;
  }

  bool skip(size_t count) {
    if (rem < count) return false;
    ptr += count;
    rem -= count;
    return true;
  }
};

} // anonymous namespace

ClassFile::ClassFile()
    : magic_(0),
      minor_version_(0),
      major_version_(0),
      cp_count_(0),
      cp_entries_(nullptr),
      access_flags_(0),
      this_class_(0),
      super_class_(0),
      interfaces_count_(0),
      fields_count_(0),
      methods_count_(0),
      methods_(nullptr) {}

ClassFile::~ClassFile() {}

ParseStatus ClassFile::parse(system::System* sys, const uint8_t* data, size_t length, ClassFile** out_class) {
  if (!sys || !data || !out_class) return ParseErrorTruncated;

  ByteReader r(data, length);

  uint32_t magic = 0;
  if (!r.readU32(&magic)) return ParseErrorTruncated;
  if (magic != 0xCAFEBABE) {
    return ParseErrorInvalidMagic;
  }

  uint16_t minor_version = 0;
  uint16_t major_version = 0;
  if (!r.readU16(&minor_version) || !r.readU16(&major_version)) return ParseErrorTruncated;

  // Support Java 1.1 (45) up to Java 21 (65); canonical Java 8 is 52
  if (major_version < 45 || major_version > 65) {
    return ParseErrorUnsupportedVersion;
  }

  uint16_t cp_count = 0;
  if (!r.readU16(&cp_count) || cp_count == 0) return ParseErrorInvalidConstantPool;

  ConstantPoolEntry* cp_entries = static_cast<ConstantPoolEntry*>(
      sys->allocate(cp_count * sizeof(ConstantPoolEntry)));
  if (!cp_entries) return ParseErrorAllocationFailed;
  memset(cp_entries, 0, cp_count * sizeof(ConstantPoolEntry));

  for (uint16_t i = 1; i < cp_count; ++i) {
    uint8_t tag = 0;
    if (!r.readU8(&tag)) {
      sys->free(cp_entries);
      return ParseErrorTruncated;
    }
    cp_entries[i].tag = tag;

    switch (tag) {
      case ConstantUtf8: {
        uint16_t ulen = 0;
        if (!r.readU16(&ulen)) {
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }
        const uint8_t* ubytes = nullptr;
        if (!r.readBytes(&ubytes, ulen)) {
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }
        char* str_buf = static_cast<char*>(sys->allocate(ulen + 1));
        if (str_buf) {
          memcpy(str_buf, ubytes, ulen);
          str_buf[ulen] = '\0';
          cp_entries[i].utf8.bytes = str_buf;
          cp_entries[i].utf8.length = ulen;
        }
        break;
      }
      case ConstantInteger:
      case ConstantFloat: {
        uint32_t val = 0;
        if (!r.readU32(&val)) { sys->free(cp_entries); return ParseErrorTruncated; }
        cp_entries[i].int_val = static_cast<int32_t>(val);
        break;
      }
      case ConstantLong:
      case ConstantDouble: {
        uint32_t hi = 0, lo = 0;
        if (!r.readU32(&hi) || !r.readU32(&lo)) { sys->free(cp_entries); return ParseErrorTruncated; }
        cp_entries[i].long_val = (static_cast<int64_t>(hi) << 32) | lo;
        i++; // 8-byte constants take two constant pool entries
        break;
      }
      case ConstantClass: {
        uint16_t nidx = 0;
        if (!r.readU16(&nidx)) { sys->free(cp_entries); return ParseErrorTruncated; }
        cp_entries[i].class_info.name_index = nidx;
        break;
      }
      case ConstantString: {
        uint16_t sidx = 0;
        if (!r.readU16(&sidx)) { sys->free(cp_entries); return ParseErrorTruncated; }
        cp_entries[i].string_info.string_index = sidx;
        break;
      }
      case ConstantFieldref:
      case ConstantMethodref:
      case ConstantInterfaceMethodref: {
        uint16_t cidx = 0, ntidx = 0;
        if (!r.readU16(&cidx) || !r.readU16(&ntidx)) { sys->free(cp_entries); return ParseErrorTruncated; }
        cp_entries[i].ref.class_index = cidx;
        cp_entries[i].ref.name_and_type_index = ntidx;
        break;
      }
      case ConstantNameAndType: {
        uint16_t nidx = 0, didx = 0;
        if (!r.readU16(&nidx) || !r.readU16(&didx)) { sys->free(cp_entries); return ParseErrorTruncated; }
        cp_entries[i].name_and_type.name_index = nidx;
        cp_entries[i].name_and_type.descriptor_index = didx;
        break;
      }
      case ConstantMethodHandle: {
        if (!r.skip(3)) { sys->free(cp_entries); return ParseErrorTruncated; }
        break;
      }
      case ConstantMethodType: {
        if (!r.skip(2)) { sys->free(cp_entries); return ParseErrorTruncated; }
        break;
      }
      case ConstantInvokeDynamic: {
        if (!r.skip(4)) { sys->free(cp_entries); return ParseErrorTruncated; }
        break;
      }
      default:
        break;
    }
  }

  uint16_t access_flags = 0, this_class = 0, super_class = 0;
  if (!r.readU16(&access_flags) || !r.readU16(&this_class) || !r.readU16(&super_class)) {
    sys->free(cp_entries);
    return ParseErrorTruncated;
  }

  // Interfaces
  uint16_t interfaces_count = 0;
  if (!r.readU16(&interfaces_count)) { sys->free(cp_entries); return ParseErrorTruncated; }
  if (interfaces_count > 0 && !r.skip(interfaces_count * 2)) {
    sys->free(cp_entries);
    return ParseErrorTruncated;
  }

  // Fields
  uint16_t fields_count = 0;
  if (!r.readU16(&fields_count)) { sys->free(cp_entries); return ParseErrorTruncated; }
  for (uint16_t f = 0; f < fields_count; ++f) {
    uint16_t facc = 0, fname = 0, fdesc = 0, fattr_count = 0;
    if (!r.readU16(&facc) || !r.readU16(&fname) || !r.readU16(&fdesc) || !r.readU16(&fattr_count)) {
      sys->free(cp_entries);
      return ParseErrorTruncated;
    }
    for (uint16_t a = 0; a < fattr_count; ++a) {
      uint16_t aname = 0;
      uint32_t alen = 0;
      if (!r.readU16(&aname) || !r.readU32(&alen) || !r.skip(alen)) {
        sys->free(cp_entries);
        return ParseErrorTruncated;
      }
    }
  }

  // Methods
  uint16_t methods_count = 0;
  if (!r.readU16(&methods_count)) { sys->free(cp_entries); return ParseErrorTruncated; }

  MethodInfo* methods = static_cast<MethodInfo*>(sys->allocate(methods_count * sizeof(MethodInfo)));
  if (methods_count > 0 && !methods) {
    sys->free(cp_entries);
    return ParseErrorAllocationFailed;
  }
  memset(methods, 0, methods_count * sizeof(MethodInfo));

  for (uint16_t m = 0; m < methods_count; ++m) {
    if (!r.readU16(&methods[m].access_flags) ||
        !r.readU16(&methods[m].name_index) ||
        !r.readU16(&methods[m].descriptor_index)) {
      sys->free(methods);
      sys->free(cp_entries);
      return ParseErrorTruncated;
    }

    uint16_t mattr_count = 0;
    if (!r.readU16(&mattr_count)) {
      sys->free(methods);
      sys->free(cp_entries);
      return ParseErrorTruncated;
    }

    for (uint16_t a = 0; a < mattr_count; ++a) {
      uint16_t aname_idx = 0;
      uint32_t alen = 0;
      if (!r.readU16(&aname_idx) || !r.readU32(&alen)) {
        sys->free(methods);
        sys->free(cp_entries);
        return ParseErrorTruncated;
      }

      // Check if attribute is "Code"
      const char* aname = nullptr;
      if (aname_idx > 0 && aname_idx < cp_count && cp_entries[aname_idx].tag == ConstantUtf8) {
        aname = cp_entries[aname_idx].utf8.bytes;
      }

      if (aname && strcmp(aname, "Code") == 0) {
        uint16_t max_stack = 0, max_locals = 0;
        uint32_t code_len = 0;
        if (!r.readU16(&max_stack) || !r.readU16(&max_locals) || !r.readU32(&code_len)) {
          sys->free(methods);
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }
        const uint8_t* code_bytes = nullptr;
        if (!r.readBytes(&code_bytes, code_len)) {
          sys->free(methods);
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }

        methods[m].has_code = true;
        methods[m].code.max_stack = max_stack;
        methods[m].code.max_locals = max_locals;
        methods[m].code.code_length = code_len;
        methods[m].code.code = code_bytes;

        // Parse exception table
        uint16_t ex_count = 0;
        if (!r.readU16(&ex_count)) {
          sys->free(methods);
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }

        methods[m].code.exception_table_length = ex_count;
        if (ex_count > 0) {
          methods[m].code.exception_table = static_cast<ExceptionTableEntry*>(
              sys->allocate(ex_count * sizeof(ExceptionTableEntry)));
          if (!methods[m].code.exception_table) {
            sys->free(methods);
            sys->free(cp_entries);
            return ParseErrorAllocationFailed;
          }
          for (uint16_t e = 0; e < ex_count; ++e) {
            if (!r.readU16(&methods[m].code.exception_table[e].start_pc) ||
                !r.readU16(&methods[m].code.exception_table[e].end_pc) ||
                !r.readU16(&methods[m].code.exception_table[e].handler_pc) ||
                !r.readU16(&methods[m].code.exception_table[e].catch_type)) {
              sys->free(methods[m].code.exception_table);
              sys->free(methods);
              sys->free(cp_entries);
              return ParseErrorTruncated;
            }
          }
        } else {
          methods[m].code.exception_table = nullptr;
        }

        // Skip nested code attributes
        uint16_t sub_attrs = 0;
        if (!r.readU16(&sub_attrs)) {
          if (methods[m].code.exception_table) sys->free(methods[m].code.exception_table);
          sys->free(methods);
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }
        for (uint16_t sa = 0; sa < sub_attrs; ++sa) {
          uint16_t saname = 0;
          uint32_t salen = 0;
          if (!r.readU16(&saname) || !r.readU32(&salen) || !r.skip(salen)) {
            if (methods[m].code.exception_table) sys->free(methods[m].code.exception_table);
            sys->free(methods);
            sys->free(cp_entries);
            return ParseErrorTruncated;
          }
        }
      } else {
        if (!r.skip(alen)) {
          sys->free(methods);
          sys->free(cp_entries);
          return ParseErrorTruncated;
        }
      }
    }
  }

  void* cls_mem = sys->allocate(sizeof(ClassFile));
  if (!cls_mem) {
    for (uint16_t m = 0; m < methods_count; ++m) {
      if (methods[m].has_code && methods[m].code.exception_table) {
        sys->free(methods[m].code.exception_table);
      }
    }
    sys->free(methods);
    sys->free(cp_entries);
    return ParseErrorAllocationFailed;
  }

  ClassFile* cls = new (cls_mem) ClassFile();
  cls->magic_ = magic;
  cls->minor_version_ = minor_version;
  cls->major_version_ = major_version;
  cls->cp_count_ = cp_count;
  cls->cp_entries_ = cp_entries;
  cls->access_flags_ = access_flags;
  cls->this_class_ = this_class;
  cls->super_class_ = super_class;
  cls->interfaces_count_ = interfaces_count;
  cls->fields_count_ = fields_count;
  cls->methods_count_ = methods_count;
  cls->methods_ = methods;

  *out_class = cls;
  return ParseSuccess;
}

const ConstantPoolEntry* ClassFile::getConstantPoolEntry(uint16_t index) const {
  if (index > 0 && index < cp_count_) {
    return &cp_entries_[index];
  }
  return nullptr;
}

const char* ClassFile::getUtf8(uint16_t index, uint16_t* out_len) const {
  if (index > 0 && index < cp_count_ && cp_entries_[index].tag == ConstantUtf8) {
    if (out_len) *out_len = cp_entries_[index].utf8.length;
    return cp_entries_[index].utf8.bytes;
  }
  return nullptr;
}

const char* ClassFile::getClassName(uint16_t class_index) const {
  if (class_index > 0 && class_index < cp_count_ && cp_entries_[class_index].tag == ConstantClass) {
    uint16_t nidx = cp_entries_[class_index].class_info.name_index;
    return getUtf8(nidx);
  }
  return nullptr;
}

const MethodInfo* ClassFile::findMethod(const char* name, const char* descriptor) const {
  if (!name || !descriptor) return nullptr;

  for (uint16_t m = 0; m < methods_count_; ++m) {
    const char* mname = getUtf8(methods_[m].name_index);
    const char* mdesc = getUtf8(methods_[m].descriptor_index);
    if (mname && mdesc && strcmp(mname, name) == 0 && strcmp(mdesc, descriptor) == 0) {
      return &methods_[m];
    }
  }
  return nullptr;
}

const ExceptionTableEntry* ClassFile::findExceptionHandler(
    const MethodInfo* method, uint16_t pc, const char* exceptionClassName) const {
  if (!method || !method->has_code || !method->code.exception_table) return nullptr;

  for (uint16_t e = 0; e < method->code.exception_table_length; ++e) {
    const ExceptionTableEntry* entry = &method->code.exception_table[e];
    if (pc >= entry->start_pc && pc < entry->end_pc) {
      if (entry->catch_type == 0) {
        return entry; // catch-all / finally
      }
      const char* catchName = getClassName(entry->catch_type);
      if (catchName) {
        if (!exceptionClassName || strcmp(catchName, exceptionClassName) == 0 ||
            strcmp(catchName, "java/lang/Throwable") == 0 ||
            strcmp(catchName, "java/lang/Exception") == 0 ||
            strcmp(catchName, "java/lang/RuntimeException") == 0) {
          return entry;
        }
      }
    }
  }
  return nullptr;
}

void ClassFile::dispose(system::System* sys) {
  if (!sys) return;
  if (cp_entries_) {
    for (uint16_t i = 1; i < cp_count_; ++i) {
      if (cp_entries_[i].tag == ConstantUtf8 && cp_entries_[i].utf8.bytes) {
        sys->free(const_cast<char*>(cp_entries_[i].utf8.bytes));
      }
    }
    sys->free(cp_entries_);
    cp_entries_ = nullptr;
  }
  if (methods_) {
    for (uint16_t m = 0; m < methods_count_; ++m) {
      if (methods_[m].has_code && methods_[m].code.exception_table) {
        sys->free(methods_[m].code.exception_table);
      }
    }
    sys->free(methods_);
    methods_ = nullptr;
  }
  sys->free(this);
}

} // namespace avian
