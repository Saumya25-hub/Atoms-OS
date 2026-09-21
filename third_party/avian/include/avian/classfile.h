/*
 * ATOMS OS — Java Classfile Parser & Metadata Architecture
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Conforms to Java Virtual Machine Specification (JVMS 8 §4).
 */

#ifndef AVIAN_CLASSFILE_H
#define AVIAN_CLASSFILE_H

#include <avian/common.h>
#include <avian/system/system.h>

namespace avian {

// Constant Pool Tags (JVMS §4.4)
enum ConstantPoolTag {
  ConstantUtf8               = 1,
  ConstantInteger            = 3,
  ConstantFloat              = 4,
  ConstantLong               = 5,
  ConstantDouble             = 6,
  ConstantClass              = 7,
  ConstantString             = 8,
  ConstantFieldref           = 9,
  ConstantMethodref          = 10,
  ConstantInterfaceMethodref = 11,
  ConstantNameAndType        = 12,
  ConstantMethodHandle       = 15,
  ConstantMethodType         = 16,
  ConstantInvokeDynamic      = 18
};

// Access Flags (JVMS §4.1, §4.5, §4.6)
enum AccessFlags {
  AccPublic    = 0x0001,
  AccPrivate   = 0x0002,
  AccProtected = 0x0004,
  AccStatic    = 0x0008,
  AccFinal     = 0x0010,
  AccSuper     = 0x0020,
  AccNative    = 0x0100,
  AccAbstract  = 0x0400
};

// Classfile Parse Status
enum ParseStatus {
  ParseSuccess                 = 0,
  ParseErrorTruncated          = 1,
  ParseErrorInvalidMagic       = 2,
  ParseErrorUnsupportedVersion = 3,
  ParseErrorInvalidConstantPool= 4,
  ParseErrorAllocationFailed   = 5
};

struct ConstantPoolEntry {
  uint8_t tag;
  union {
    struct {
      const char* bytes;
      uint16_t length;
    } utf8;
    struct {
      uint16_t name_index;
    } class_info;
    struct {
      uint16_t string_index;
    } string_info;
    struct {
      uint16_t class_index;
      uint16_t name_and_type_index;
    } ref;
    struct {
      uint16_t name_index;
      uint16_t descriptor_index;
    } name_and_type;
    int32_t int_val;
    int64_t long_val;
  };
};

struct ExceptionTableEntry {
  uint16_t start_pc;
  uint16_t end_pc;
  uint16_t handler_pc;
  uint16_t catch_type; // Constant pool index to Class, or 0 for any/finally
};

struct CodeAttribute {
  uint16_t max_stack;
  uint16_t max_locals;
  uint32_t code_length;
  const uint8_t* code;
  uint16_t exception_table_length;
  ExceptionTableEntry* exception_table;
};

struct MethodInfo {
  uint16_t access_flags;
  uint16_t name_index;
  uint16_t descriptor_index;
  bool has_code;
  CodeAttribute code;
};

class ClassFile {
 public:
  ClassFile();
  ~ClassFile();

  static ParseStatus parse(system::System* sys, const uint8_t* data, size_t length, ClassFile** out_class);

  uint32_t magic() const { return magic_; }
  uint16_t minorVersion() const { return minor_version_; }
  uint16_t majorVersion() const { return major_version_; }
  uint16_t constantPoolCount() const { return cp_count_; }
  uint16_t methodsCount() const { return methods_count_; }
  uint16_t thisClass() const { return this_class_; }

  const ConstantPoolEntry* getConstantPoolEntry(uint16_t index) const;
  const char* getUtf8(uint16_t index, uint16_t* out_len = nullptr) const;
  const char* getClassName(uint16_t class_index) const;

  const MethodInfo* findMethod(const char* name, const char* descriptor) const;
  const ExceptionTableEntry* findExceptionHandler(const MethodInfo* method, uint16_t pc, const char* exceptionClassName) const;
  void dispose(system::System* sys);

 private:
  uint32_t magic_;
  uint16_t minor_version_;
  uint16_t major_version_;
  uint16_t cp_count_;
  ConstantPoolEntry* cp_entries_;
  uint16_t access_flags_;
  uint16_t this_class_;
  uint16_t super_class_;
  uint16_t interfaces_count_;
  uint16_t fields_count_;
  uint16_t methods_count_;
  MethodInfo* methods_;
};

} // namespace avian

#endif // AVIAN_CLASSFILE_H
