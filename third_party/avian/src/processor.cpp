/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#include <avian/common.h>
#include <avian/system/system.h>
#include <avian/heap/heap.h>
#include <avian/classfile.h>
#include <avian/machine.h>
#include <avian/processor.h>
#include <userspace/runtime/cpp/include/new>
#include <string.h>

namespace avian {

// Standard JVM Bytecode Opcodes (JVMS §6)
enum Opcode {
  op_nop          = 0x00,
  op_aconst_null  = 0x01,
  op_iconst_m1    = 0x02,
  op_iconst_0     = 0x03,
  op_iconst_1     = 0x04,
  op_iconst_2     = 0x05,
  op_iconst_3     = 0x06,
  op_iconst_4     = 0x07,
  op_iconst_5     = 0x08,
  op_bipush       = 0x10,
  op_sipush       = 0x11,
  op_ldc          = 0x12,
  op_ldc_w        = 0x13,
  op_iload        = 0x15,
  op_aload        = 0x19,
  op_iload_0      = 0x1a,
  op_iload_1      = 0x1b,
  op_iload_2      = 0x1c,
  op_iload_3      = 0x1d,
  op_aload_0      = 0x2a,
  op_aload_1      = 0x2b,
  op_aload_2      = 0x2c,
  op_aload_3      = 0x2d,
  op_iaload       = 0x2e,
  op_aaload       = 0x32,
  op_istore       = 0x36,
  op_astore       = 0x3a,
  op_istore_0     = 0x3b,
  op_istore_1     = 0x3c,
  op_istore_2     = 0x3d,
  op_istore_3     = 0x3e,
  op_astore_0     = 0x4b,
  op_astore_1     = 0x4c,
  op_astore_2     = 0x4d,
  op_astore_3     = 0x4e,
  op_iastore      = 0x4f,
  op_aastore      = 0x53,
  op_pop          = 0x57,
  op_dup          = 0x59,
  op_iadd         = 0x60,
  op_isub         = 0x64,
  op_imul         = 0x68,
  op_idiv         = 0x6c,
  op_iinc         = 0x84,
  op_ifeq         = 0x99,
  op_ifne         = 0x9a,
  op_iflt         = 0x9b,
  op_ifge         = 0x9c,
  op_ifgt         = 0x9d,
  op_ifle         = 0x9e,
  op_if_icmpeq    = 0x9f,
  op_if_icmpne    = 0xa0,
  op_if_icmplt    = 0xa1,
  op_if_icmpge    = 0xa2,
  op_if_icmpgt    = 0xa3,
  op_if_icmple    = 0xa4,
  op_if_acmpeq    = 0xa5,
  op_if_acmpne    = 0xa6,
  op_goto         = 0xa7,
  op_ireturn      = 0xac,
  op_areturn      = 0xb0,
  op_return       = 0xb1,
  op_getstatic    = 0xb2,
  op_putstatic    = 0xb3,
  op_getfield     = 0xb4,
  op_putfield     = 0xb5,
  op_invokevirtual= 0xb6,
  op_invokespecial= 0xb7,
  op_invokestatic = 0xb8,
  op_new          = 0xbb,
  op_arraylength  = 0xbe,
  op_athrow       = 0xbf,
  op_checkcast    = 0xc0,
  op_instanceof   = 0xc1,
  op_ifnull       = 0xc6,
  op_ifnonnull    = 0xc7
};

// Structures for minimal bootstrap classpath & runtime objects
struct JavaString {
  const char* utf8_bytes;
  uint16_t length;
};

struct JavaStringArray {
  uint32_t length;
  intptr_t elements[1];
};

struct JavaPrintStream {
  int fd;
};

struct JavaThrowable {
  const char* class_name;
  const char* message;
};

struct JavaStringBuilder {
  char buffer[1024];
  size_t length;

  void appendStr(const char* s, size_t len) {
    if (!s) return;
    for (size_t i = 0; i < len && length + 1 < sizeof(buffer); ++i) {
      buffer[length++] = s[i];
    }
    buffer[length] = '\0';
  }

  void appendInt(int32_t val) {
    char num_buf[32];
    int nlen = 0;
    if (val == 0) {
      num_buf[nlen++] = '0';
    } else {
      bool neg = val < 0;
      uint32_t uval = neg ? -val : val;
      char tmp[16];
      int tlen = 0;
      while (uval > 0) {
        tmp[tlen++] = '0' + (uval % 10);
        uval /= 10;
      }
      if (neg) num_buf[nlen++] = '-';
      while (tlen > 0) {
        num_buf[nlen++] = tmp[--tlen];
      }
    }
    num_buf[nlen] = '\0';
    appendStr(num_buf, nlen);
  }
};

struct JavaArrayList {
  intptr_t elements[256];
  size_t size;

  void add(intptr_t item) {
    if (size < 256) {
      elements[size++] = item;
    }
  }

  intptr_t get(size_t index) const {
    if (index < size) return elements[index];
    return 0;
  }

  void set(size_t index, intptr_t item) {
    if (index < size) {
      elements[index] = item;
    }
  }

  intptr_t remove(size_t index) {
    if (index >= size) return 0;
    intptr_t old = elements[index];
    for (size_t i = index; i + 1 < size; ++i) {
      elements[i] = elements[i + 1];
    }
    --size;
    return old;
  }

  void clear() {
    size = 0;
  }

  bool isEmpty() const {
    return size == 0;
  }
};

// Generic Custom Instance Object Layout: ObjectHeader followed by 16 field slots
struct JavaGenericObject {
  intptr_t fields[16];
};

struct ExecutionFrame {
  intptr_t stack[128];
  size_t   sp;
  intptr_t locals[32];

  ExecutionFrame() : sp(0) {
    for (size_t i = 0; i < 128; ++i) stack[i] = 0;
    for (size_t i = 0; i < 32; ++i) locals[i] = 0;
  }

  void push(intptr_t val) {
    if (sp < 128) stack[sp++] = val;
  }

  intptr_t pop() {
    return (sp > 0) ? stack[--sp] : 0;
  }

  intptr_t peek() const {
    return (sp > 0) ? stack[sp - 1] : 0;
  }
};

class AvianInterpreterProcessor : public InterpreterProcessor {
 public:
  AvianInterpreterProcessor(system::System* s, heap::Heap* h = nullptr, Machine* m = nullptr)
      : system_(s), heap_(h), machine_(m), stdout_stream_(nullptr)
  {
    if (heap_) {
      void* dummy_class = reinterpret_cast<void*>(0xCAFE0001);
      stdout_stream_ = static_cast<JavaPrintStream*>(
          heap_->allocateObject(dummy_class, sizeof(JavaPrintStream)));
      if (stdout_stream_) {
        stdout_stream_->fd = 1; // standard output
      }
    }
  }

  int executeMethod(ClassFile* cls, const MethodInfo* method, intptr_t* args, size_t numArgs, intptr_t* outResult = nullptr, JavaThrowable** outPendingException = nullptr) {
    if (!cls || !method || !method->has_code) return -1;

    ExecutionFrame frame;
    for (size_t i = 0; i < numArgs && i < 32; ++i) {
      frame.locals[i] = args[i];
    }

    const uint8_t* code = method->code.code;
    size_t length = method->code.code_length;
    size_t pc = 0;

    while (pc < length) {
      size_t current_instruction_pc = pc;
      uint8_t op = code[pc++];
      switch (op) {
        case op_nop:
          break;

        case op_aconst_null:
        case op_iconst_0:
          frame.push(0);
          break;
        case op_iconst_m1:
          frame.push(-1);
          break;
        case op_iconst_1:
          frame.push(1);
          break;
        case op_iconst_2:
          frame.push(2);
          break;
        case op_iconst_3:
          frame.push(3);
          break;
        case op_iconst_4:
          frame.push(4);
          break;
        case op_iconst_5:
          frame.push(5);
          break;

        case op_bipush:
          if (pc < length) {
            frame.push(static_cast<int8_t>(code[pc++]));
          }
          break;

        case op_sipush:
          if (pc + 1 < length) {
            int16_t sval = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            frame.push(sval);
          }
          break;

        case op_ldc: {
          if (pc < length) {
            uint8_t cidx = code[pc++];
            pushConstant(cls, cidx, &frame);
          }
          break;
        }

        case op_ldc_w: {
          if (pc + 1 < length) {
            uint16_t cidx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            pushConstant(cls, cidx, &frame);
          }
          break;
        }

        case op_aload_0:
        case op_iload_0:
          frame.push(frame.locals[0]);
          break;
        case op_aload_1:
        case op_iload_1:
          frame.push(frame.locals[1]);
          break;
        case op_aload_2:
        case op_iload_2:
          frame.push(frame.locals[2]);
          break;
        case op_aload_3:
        case op_iload_3:
          frame.push(frame.locals[3]);
          break;

        case op_aload:
        case op_iload:
          if (pc < length) {
            uint8_t lidx = code[pc++];
            if (lidx < 32) frame.push(frame.locals[lidx]);
          }
          break;

        case op_aaload:
        case op_iaload: {
          int32_t idx = static_cast<int32_t>(frame.pop());
          intptr_t arr_ref = frame.pop();
          if (arr_ref == 0) {
            raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "Array access on null reference", &frame, &pc, outPendingException);
            if (outPendingException && *outPendingException) return 1;
            break;
          }
          JavaStringArray* arr = reinterpret_cast<JavaStringArray*>(arr_ref);
          if (idx < 0 || static_cast<uint32_t>(idx) >= arr->length) {
            raiseException(cls, method, current_instruction_pc, "java/lang/ArrayIndexOutOfBoundsException", "Array index out of bounds", &frame, &pc, outPendingException);
            if (outPendingException && *outPendingException) return 1;
            break;
          }
          frame.push(arr->elements[idx]);
          break;
        }

        case op_astore_0:
        case op_istore_0:
          frame.locals[0] = frame.pop();
          break;
        case op_astore_1:
        case op_istore_1:
          frame.locals[1] = frame.pop();
          break;
        case op_astore_2:
        case op_istore_2:
          frame.locals[2] = frame.pop();
          break;
        case op_astore_3:
        case op_istore_3:
          frame.locals[3] = frame.pop();
          break;

        case op_astore:
        case op_istore:
          if (pc < length) {
            uint8_t lidx = code[pc++];
            if (lidx < 32) frame.locals[lidx] = frame.pop();
          }
          break;

        case op_aastore:
        case op_iastore: {
          intptr_t val = frame.pop();
          int32_t idx = static_cast<int32_t>(frame.pop());
          intptr_t arr_ref = frame.pop();
          if (arr_ref == 0) {
            raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "Array store on null reference", &frame, &pc, outPendingException);
            if (outPendingException && *outPendingException) return 1;
            break;
          }
          JavaStringArray* arr = reinterpret_cast<JavaStringArray*>(arr_ref);
          if (idx < 0 || static_cast<uint32_t>(idx) >= arr->length) {
            raiseException(cls, method, current_instruction_pc, "java/lang/ArrayIndexOutOfBoundsException", "Array index out of bounds", &frame, &pc, outPendingException);
            if (outPendingException && *outPendingException) return 1;
            break;
          }
          arr->elements[idx] = val;
          break;
        }

        case op_pop:
          frame.pop();
          break;

        case op_dup:
          frame.push(frame.peek());
          break;

        case op_iinc: {
          if (pc + 1 < length) {
            uint8_t lidx = code[pc++];
            int8_t const_val = static_cast<int8_t>(code[pc++]);
            if (lidx < 32) {
              frame.locals[lidx] += const_val;
            }
          }
          break;
        }

        case op_arraylength: {
          intptr_t arr_ref = frame.pop();
          if (arr_ref == 0) {
            raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "arraylength on null reference", &frame, &pc, outPendingException);
            if (outPendingException && *outPendingException) return 1;
            break;
          }
          JavaStringArray* arr = reinterpret_cast<JavaStringArray*>(arr_ref);
          frame.push(arr->length);
          break;
        }

        case op_checkcast:
        case op_instanceof:
          if (pc + 1 < length) {
            pc += 2;
            if (op == op_instanceof) {
              intptr_t obj = frame.pop();
              frame.push(obj != 0 ? 1 : 0);
            }
          }
          break;

        case op_new: {
          if (pc + 1 < length) {
            uint16_t cidx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            const char* class_name = cls->getClassName(cidx);
            intptr_t new_obj = 0;
            if (class_name && heap_) {
              if (strcmp(class_name, "java/lang/StringBuilder") == 0) {
                void* sb_cls = reinterpret_cast<void*>(0xCAFE0010);
                JavaStringBuilder* sb = static_cast<JavaStringBuilder*>(
                    heap_->allocateObject(sb_cls, sizeof(JavaStringBuilder)));
                if (sb) {
                  sb->length = 0;
                  sb->buffer[0] = '\0';
                }
                new_obj = reinterpret_cast<intptr_t>(sb);
              } else if (strcmp(class_name, "java/util/ArrayList") == 0) {
                void* list_cls = reinterpret_cast<void*>(0xCAFE0020);
                JavaArrayList* list = static_cast<JavaArrayList*>(
                    heap_->allocateObject(list_cls, sizeof(JavaArrayList)));
                if (list) {
                  list->size = 0;
                }
                new_obj = reinterpret_cast<intptr_t>(list);
              } else if (strstr(class_name, "Exception") != nullptr || strstr(class_name, "Throwable") != nullptr || strstr(class_name, "Error") != nullptr) {
                void* ex_cls = reinterpret_cast<void*>(0xCAFE0030);
                JavaThrowable* ex = static_cast<JavaThrowable*>(
                    heap_->allocateObject(ex_cls, sizeof(JavaThrowable)));
                if (ex) {
                  ex->class_name = class_name;
                  ex->message = "";
                }
                new_obj = reinterpret_cast<intptr_t>(ex);
              } else {
                // Generic Object / User-defined class instance (e.g. com/atoms/demo/App or java/lang/Object)
                void* generic_cls = reinterpret_cast<void*>(0xCAFE0040);
                JavaGenericObject* obj = static_cast<JavaGenericObject*>(
                    heap_->allocateObject(generic_cls, sizeof(JavaGenericObject)));
                if (obj) {
                  memset(obj->fields, 0, sizeof(obj->fields));
                }
                new_obj = reinterpret_cast<intptr_t>(obj);
              }
            }
            frame.push(new_obj);
          }
          break;
        }

        case op_iadd: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          frame.push(a + b);
          break;
        }
        case op_isub: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          frame.push(a - b);
          break;
        }
        case op_imul: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          frame.push(a * b);
          break;
        }
        case op_idiv: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          if (b == 0) {
            raiseException(cls, method, current_instruction_pc, "java/lang/ArithmeticException", "/ by zero", &frame, &pc, outPendingException);
            if (outPendingException && *outPendingException) return 1;
            break;
          }
          frame.push(a / b);
          break;
        }

        // Branching Instructions
        case op_ifeq: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v = static_cast<int32_t>(frame.pop());
            if (v == 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_ifne: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v = static_cast<int32_t>(frame.pop());
            if (v != 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_iflt: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v = static_cast<int32_t>(frame.pop());
            if (v < 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_ifge: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v = static_cast<int32_t>(frame.pop());
            if (v >= 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_ifgt: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v = static_cast<int32_t>(frame.pop());
            if (v > 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_ifle: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v = static_cast<int32_t>(frame.pop());
            if (v <= 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_icmpeq: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v2 = static_cast<int32_t>(frame.pop());
            int32_t v1 = static_cast<int32_t>(frame.pop());
            if (v1 == v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_icmpne: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v2 = static_cast<int32_t>(frame.pop());
            int32_t v1 = static_cast<int32_t>(frame.pop());
            if (v1 != v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_icmplt: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v2 = static_cast<int32_t>(frame.pop());
            int32_t v1 = static_cast<int32_t>(frame.pop());
            if (v1 < v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_icmpge: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v2 = static_cast<int32_t>(frame.pop());
            int32_t v1 = static_cast<int32_t>(frame.pop());
            if (v1 >= v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_icmpgt: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v2 = static_cast<int32_t>(frame.pop());
            int32_t v1 = static_cast<int32_t>(frame.pop());
            if (v1 > v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_icmple: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            int32_t v2 = static_cast<int32_t>(frame.pop());
            int32_t v1 = static_cast<int32_t>(frame.pop());
            if (v1 <= v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_acmpeq: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            intptr_t v2 = frame.pop();
            intptr_t v1 = frame.pop();
            if (v1 == v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_if_acmpne: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            intptr_t v2 = frame.pop();
            intptr_t v1 = frame.pop();
            if (v1 != v2) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_ifnull: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            intptr_t v = frame.pop();
            if (v == 0) pc = current_instruction_pc + offset;
          }
          break;
        }
        case op_ifnonnull: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            intptr_t v = frame.pop();
            if (v != 0) pc = current_instruction_pc + offset;
          }
          break;
        }

        case op_goto: {
          if (pc + 1 < length) {
            int16_t offset = (static_cast<int16_t>(code[pc]) << 8) | code[pc + 1];
            pc = current_instruction_pc + offset;
          }
          break;
        }

        case op_athrow: {
          intptr_t ex_val = frame.pop();
          JavaThrowable* ex = reinterpret_cast<JavaThrowable*>(ex_val);
          if (!ex) {
            void* ex_cls = reinterpret_cast<void*>(0xCAFE0030);
            ex = static_cast<JavaThrowable*>(
                heap_->allocateObject(ex_cls, sizeof(JavaThrowable)));
            if (ex) {
              ex->class_name = "java/lang/NullPointerException";
              ex->message = "athrow null pointer";
            }
          }
          bool handled = handleException(cls, method, current_instruction_pc, ex, &frame, &pc, outPendingException);
          if (!handled) {
            if (outPendingException) *outPendingException = ex;
            return 1;
          }
          break;
        }

        case op_getstatic: {
          if (pc + 1 < length) {
            uint16_t fidx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            const ConstantPoolEntry* cp = cls->getConstantPoolEntry(fidx);
            if (cp && cp->tag == ConstantFieldref) {
              const char* cname = cls->getClassName(cp->ref.class_index);
              const ConstantPoolEntry* nt = cls->getConstantPoolEntry(cp->ref.name_and_type_index);
              if (nt && nt->tag == ConstantNameAndType) {
                const char* fname = cls->getUtf8(nt->name_and_type.name_index);
                // System.out -> PrintStream
                if (cname && fname && strcmp(cname, "java/lang/System") == 0 && strcmp(fname, "out") == 0) {
                  frame.push(reinterpret_cast<intptr_t>(stdout_stream_));
                } else {
                  // Retrieve from Machine static field table
                  intptr_t val = 0;
                  if (machine_) {
                    machine_->loadClass(cname); // Ensure class initialized
                    if (machine_->getStaticField(cname, fname, &val)) {
                      frame.push(val);
                    } else {
                      frame.push(0);
                    }
                  } else {
                    frame.push(0);
                  }
                }
              }
            }
          }
          break;
        }

        case op_putstatic: {
          if (pc + 1 < length) {
            uint16_t fidx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            const ConstantPoolEntry* cp = cls->getConstantPoolEntry(fidx);
            if (cp && cp->tag == ConstantFieldref) {
              const char* cname = cls->getClassName(cp->ref.class_index);
              const ConstantPoolEntry* nt = cls->getConstantPoolEntry(cp->ref.name_and_type_index);
              if (nt && nt->tag == ConstantNameAndType) {
                const char* fname = cls->getUtf8(nt->name_and_type.name_index);
                intptr_t val = frame.pop();
                if (machine_) {
                  machine_->setStaticField(cname, fname, val);
                }
              }
            }
          }
          break;
        }

        case op_getfield: {
          if (pc + 1 < length) {
            uint16_t fidx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            intptr_t obj_ref = frame.pop();
            if (obj_ref == 0) {
              raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "getfield on null reference", &frame, &pc, outPendingException);
              if (outPendingException && *outPendingException) return 1;
              break;
            }
            // Resolve field index slot (0 to 15)
            uint16_t slot = fidx % 16;
            JavaGenericObject* obj = reinterpret_cast<JavaGenericObject*>(obj_ref);
            frame.push(obj->fields[slot]);
          }
          break;
        }

        case op_putfield: {
          if (pc + 1 < length) {
            uint16_t fidx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            intptr_t val = frame.pop();
            intptr_t obj_ref = frame.pop();
            if (obj_ref == 0) {
              raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "putfield on null reference", &frame, &pc, outPendingException);
              if (outPendingException && *outPendingException) return 1;
              break;
            }
            uint16_t slot = fidx % 16;
            JavaGenericObject* obj = reinterpret_cast<JavaGenericObject*>(obj_ref);
            obj->fields[slot] = val;
          }
          break;
        }

        case op_invokevirtual: {
          if (pc + 1 < length) {
            uint16_t midx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            const ConstantPoolEntry* cp = cls->getConstantPoolEntry(midx);
            if (cp && cp->tag == ConstantMethodref) {
              const char* cname = cls->getClassName(cp->ref.class_index);
              const ConstantPoolEntry* nt = cls->getConstantPoolEntry(cp->ref.name_and_type_index);
              if (nt && nt->tag == ConstantNameAndType) {
                const char* mname = cls->getUtf8(nt->name_and_type.name_index);
                const char* mdesc = cls->getUtf8(nt->name_and_type.descriptor_index);

                // --- java/io/PrintStream ---
                if (cname && mname && mdesc &&
                    strcmp(cname, "java/io/PrintStream") == 0 &&
                    strcmp(mname, "println") == 0) {
                  if (strcmp(mdesc, "(Ljava/lang/String;)V") == 0 ||
                      strcmp(mdesc, "(Ljava/lang/Object;)V") == 0) {
                    intptr_t arg_val = frame.pop();
                    frame.pop(); // pop stream receiver

                    if (arg_val != 0 && system_) {
                      JavaString* jstr = reinterpret_cast<JavaString*>(arg_val);
                      if (jstr->utf8_bytes && jstr->length > 0) {
                        system_->write(1, jstr->utf8_bytes, jstr->length);
                      }
                      system_->write(1, "\n", 1);
                    } else if (system_) {
                      system_->write(1, "null\n", 5);
                    }
                  } else if (strcmp(mdesc, "(I)V") == 0) {
                    int32_t val = static_cast<int32_t>(frame.pop());
                    frame.pop(); // pop receiver

                    char num_buf[32];
                    int nlen = 0;
                    if (val == 0) {
                      num_buf[nlen++] = '0';
                    } else {
                      bool neg = val < 0;
                      uint32_t uval = neg ? -val : val;
                      char tmp[16];
                      int tlen = 0;
                      while (uval > 0) {
                        tmp[tlen++] = '0' + (uval % 10);
                        uval /= 10;
                      }
                      if (neg) num_buf[nlen++] = '-';
                      while (tlen > 0) {
                        num_buf[nlen++] = tmp[--tlen];
                      }
                    }
                    num_buf[nlen] = '\0';
                    if (system_) {
                      system_->write(1, num_buf, nlen);
                      system_->write(1, "\n", 1);
                    }
                  } else if (strcmp(mdesc, "(Z)V") == 0) {
                    int32_t val = static_cast<int32_t>(frame.pop());
                    frame.pop(); // pop receiver
                    if (system_) {
                      if (val != 0) system_->write(1, "true\n", 5);
                      else system_->write(1, "false\n", 6);
                    }
                  } else if (strcmp(mdesc, "(C)V") == 0) {
                    int32_t val = static_cast<int32_t>(frame.pop());
                    frame.pop(); // pop receiver
                    char ch = static_cast<char>(val);
                    if (system_) {
                      system_->write(1, &ch, 1);
                      system_->write(1, "\n", 1);
                    }
                  }
                }
                // --- java/lang/Object ---
                else if (cname && mname && strcmp(cname, "java/lang/Object") == 0 && strcmp(mname, "equals") == 0) {
                  intptr_t other = frame.pop();
                  intptr_t self = frame.pop();
                  frame.push(self == other ? 1 : 0);
                }
                else if (cname && mname && strcmp(cname, "java/lang/Object") == 0 && strcmp(mname, "hashCode") == 0) {
                  intptr_t self = frame.pop();
                  frame.push(static_cast<int32_t>(self));
                }
                else if (cname && mname && strcmp(cname, "java/lang/Object") == 0 && strcmp(mname, "toString") == 0) {
                  intptr_t self = frame.pop();
                  frame.push(self); // Returns string ref if self is string
                }
                // --- java/lang/String ---
                else if (cname && mname && strcmp(cname, "java/lang/String") == 0 && strcmp(mname, "length") == 0) {
                  intptr_t recv = frame.pop();
                  if (recv == 0) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "length on null String", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  JavaString* js = reinterpret_cast<JavaString*>(recv);
                  frame.push(js->length);
                }
                else if (cname && mname && strcmp(cname, "java/lang/String") == 0 && strcmp(mname, "charAt") == 0) {
                  int32_t idx = static_cast<int32_t>(frame.pop());
                  intptr_t recv = frame.pop();
                  if (recv == 0) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "charAt on null String", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  JavaString* js = reinterpret_cast<JavaString*>(recv);
                  if (idx < 0 || idx >= js->length) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/StringIndexOutOfBoundsException", "String index out of bounds", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  frame.push(static_cast<uint8_t>(js->utf8_bytes[idx]));
                }
                else if (cname && mname && strcmp(cname, "java/lang/String") == 0 && strcmp(mname, "startsWith") == 0) {
                  intptr_t prefix_ref = frame.pop();
                  intptr_t recv = frame.pop();
                  if (recv == 0 || prefix_ref == 0) {
                    frame.push(0);
                    break;
                  }
                  JavaString* js = reinterpret_cast<JavaString*>(recv);
                  JavaString* pfx = reinterpret_cast<JavaString*>(prefix_ref);
                  if (pfx->length > js->length) {
                    frame.push(0);
                  } else {
                    bool match = (strncmp(js->utf8_bytes, pfx->utf8_bytes, pfx->length) == 0);
                    frame.push(match ? 1 : 0);
                  }
                }
                else if (cname && mname && strcmp(cname, "java/lang/String") == 0 && strcmp(mname, "indexOf") == 0) {
                  intptr_t sub_ref = frame.pop();
                  intptr_t recv = frame.pop();
                  if (recv == 0 || sub_ref == 0) {
                    frame.push(-1);
                    break;
                  }
                  JavaString* js = reinterpret_cast<JavaString*>(recv);
                  JavaString* sub = reinterpret_cast<JavaString*>(sub_ref);
                  if (sub->length == 0) {
                    frame.push(0);
                  } else if (sub->length > js->length) {
                    frame.push(-1);
                  } else {
                    int found = -1;
                    for (size_t i = 0; i + sub->length <= js->length; ++i) {
                      if (strncmp(js->utf8_bytes + i, sub->utf8_bytes, sub->length) == 0) {
                        found = static_cast<int>(i);
                        break;
                      }
                    }
                    frame.push(found);
                  }
                }
                else if (cname && mname && strcmp(cname, "java/lang/String") == 0 && strcmp(mname, "substring") == 0) {
                  int32_t end_idx = static_cast<int32_t>(frame.pop());
                  int32_t start_idx = static_cast<int32_t>(frame.pop());
                  intptr_t recv = frame.pop();
                  if (recv == 0) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/NullPointerException", "substring on null String", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  JavaString* js = reinterpret_cast<JavaString*>(recv);
                  if (start_idx < 0 || end_idx > js->length || start_idx > end_idx) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/StringIndexOutOfBoundsException", "substring index out of bounds", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  size_t sub_len = end_idx - start_idx;
                  JavaString* new_js = allocateJavaString(js->utf8_bytes + start_idx, sub_len);
                  frame.push(reinterpret_cast<intptr_t>(new_js));
                }
                else if (cname && mname && strcmp(cname, "java/lang/String") == 0 && strcmp(mname, "equals") == 0) {
                  intptr_t other_ref = frame.pop();
                  intptr_t recv = frame.pop();
                  if (recv == other_ref) {
                    frame.push(1);
                  } else if (recv == 0 || other_ref == 0) {
                    frame.push(0);
                  } else {
                    JavaString* s1 = reinterpret_cast<JavaString*>(recv);
                    JavaString* s2 = reinterpret_cast<JavaString*>(other_ref);
                    if (s1->length != s2->length) {
                      frame.push(0);
                    } else {
                      bool match = (strncmp(s1->utf8_bytes, s2->utf8_bytes, s1->length) == 0);
                      frame.push(match ? 1 : 0);
                    }
                  }
                }
                // --- java/lang/StringBuilder ---
                else if (cname && mname && strcmp(cname, "java/lang/StringBuilder") == 0 && strcmp(mname, "append") == 0) {
                  intptr_t arg_val = frame.pop();
                  intptr_t recv_val = frame.pop();
                  JavaStringBuilder* sb = reinterpret_cast<JavaStringBuilder*>(recv_val);
                  if (sb) {
                    if (strcmp(mdesc, "(I)Ljava/lang/StringBuilder;") == 0) {
                      sb->appendInt(static_cast<int32_t>(arg_val));
                    } else if (arg_val != 0) {
                      JavaString* jstr = reinterpret_cast<JavaString*>(arg_val);
                      if (jstr->utf8_bytes) {
                        sb->appendStr(jstr->utf8_bytes, jstr->length);
                      }
                    } else {
                      sb->appendStr("null", 4);
                    }
                  }
                  frame.push(recv_val); // returns this
                }
                else if (cname && mname && strcmp(cname, "java/lang/StringBuilder") == 0 && strcmp(mname, "length") == 0) {
                  intptr_t recv_val = frame.pop();
                  JavaStringBuilder* sb = reinterpret_cast<JavaStringBuilder*>(recv_val);
                  frame.push(sb ? sb->length : 0);
                }
                else if (cname && mname && strcmp(cname, "java/lang/StringBuilder") == 0 && strcmp(mname, "toString") == 0) {
                  intptr_t recv_val = frame.pop();
                  JavaStringBuilder* sb = reinterpret_cast<JavaStringBuilder*>(recv_val);
                  intptr_t res_str = 0;
                  if (sb) {
                    JavaString* jstr = allocateJavaString(sb->buffer, sb->length);
                    res_str = reinterpret_cast<intptr_t>(jstr);
                  }
                  frame.push(res_str);
                }
                // --- java/util/ArrayList ---
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "add") == 0) {
                  intptr_t arg_val = frame.pop();
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  if (list) list->add(arg_val);
                  frame.push(1); // boolean true
                }
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "size") == 0) {
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  frame.push(list ? list->size : 0);
                }
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "get") == 0) {
                  int32_t idx = static_cast<int32_t>(frame.pop());
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  if (!list || idx < 0 || static_cast<size_t>(idx) >= list->size) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/IndexOutOfBoundsException", "Index out of bounds in ArrayList.get", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  frame.push(list->get(idx));
                }
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "set") == 0) {
                  intptr_t val = frame.pop();
                  int32_t idx = static_cast<int32_t>(frame.pop());
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  if (!list || idx < 0 || static_cast<size_t>(idx) >= list->size) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/IndexOutOfBoundsException", "Index out of bounds in ArrayList.set", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  intptr_t old = list->get(idx);
                  list->set(idx, val);
                  frame.push(old);
                }
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "remove") == 0) {
                  int32_t idx = static_cast<int32_t>(frame.pop());
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  if (!list || idx < 0 || static_cast<size_t>(idx) >= list->size) {
                    raiseException(cls, method, current_instruction_pc, "java/lang/IndexOutOfBoundsException", "Index out of bounds in ArrayList.remove", &frame, &pc, outPendingException);
                    if (outPendingException && *outPendingException) return 1;
                    break;
                  }
                  frame.push(list->remove(idx));
                }
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "clear") == 0) {
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  if (list) list->clear();
                }
                else if (cname && mname && strcmp(cname, "java/util/ArrayList") == 0 && strcmp(mname, "isEmpty") == 0) {
                  intptr_t recv_val = frame.pop();
                  JavaArrayList* list = reinterpret_cast<JavaArrayList*>(recv_val);
                  frame.push(list ? (list->isEmpty() ? 1 : 0) : 1);
                }
                // --- Virtual Method Dispatch for User Application Classes (e.g. App.addTask) ---
                else {
                  size_t callee_argc = countArguments(mdesc) + 1; // +1 for receiver `this`
                  intptr_t callee_args[16];
                  for (size_t i = 0; i < callee_argc; ++i) {
                    callee_args[callee_argc - 1 - i] = frame.pop();
                  }

                  ClassFile* target_cls = cls;
                  if (cname && machine_) {
                    ClassFile* loaded = machine_->loadClass(cname);
                    if (loaded) target_cls = loaded;
                  }

                  const MethodInfo* target_method = target_cls->findMethod(mname, mdesc);
                  if (target_method && target_method->has_code) {
                    intptr_t sub_res = 0;
                    JavaThrowable* sub_ex = nullptr;
                    int sub_rc = executeMethod(target_cls, target_method, callee_args, callee_argc, &sub_res, &sub_ex);

                    if (sub_rc != 0 || sub_ex != nullptr) {
                      bool handled = handleException(cls, method, current_instruction_pc, sub_ex, &frame, &pc, outPendingException);
                      if (!handled) {
                        if (outPendingException) *outPendingException = sub_ex;
                        return 1;
                      }
                    } else {
                      if (mdesc[strlen(mdesc) - 1] != 'V') {
                        frame.push(sub_res);
                      }
                    }
                  } else {
                    if (system_) {
                      system_->print("[JVM ERROR] Virtual method not found: ");
                      system_->print(cname ? cname : "?");
                      system_->print(".");
                      system_->print(mname ? mname : "?");
                      system_->print("\n");
                    }
                    return 1;
                  }
                }
              }
            }
          }
          break;
        }

        case op_invokespecial: {
          if (pc + 1 < length) {
            uint16_t midx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            const ConstantPoolEntry* cp = cls->getConstantPoolEntry(midx);
            if (cp && cp->tag == ConstantMethodref) {
              const char* cname = cls->getClassName(cp->ref.class_index);
              const ConstantPoolEntry* nt = cls->getConstantPoolEntry(cp->ref.name_and_type_index);
              if (nt && nt->tag == ConstantNameAndType) {
                const char* mname = cls->getUtf8(nt->name_and_type.name_index);
                const char* mdesc = cls->getUtf8(nt->name_and_type.descriptor_index);

                // Object constructor <init>()V
                if (cname && strcmp(cname, "java/lang/Object") == 0 && strcmp(mname, "<init>") == 0) {
                  frame.pop(); // pop receiver
                }
                // Throwable constructor <init>(String)
                else if (cname && mname && mdesc && strcmp(mname, "<init>") == 0 &&
                         (strstr(cname, "Exception") != nullptr || strstr(cname, "Throwable") != nullptr || strstr(cname, "Error") != nullptr)) {
                  if (strcmp(mdesc, "(Ljava/lang/String;)V") == 0) {
                    intptr_t msg_arg = frame.pop();
                    intptr_t recv_obj = frame.pop();
                    JavaThrowable* ex = reinterpret_cast<JavaThrowable*>(recv_obj);
                    if (ex && msg_arg != 0) {
                      JavaString* jstr = reinterpret_cast<JavaString*>(msg_arg);
                      ex->message = jstr->utf8_bytes ? jstr->utf8_bytes : "";
                    }
                  } else {
                    frame.pop();
                  }
                }
                // Custom constructor dispatch (e.g. App.<init>(String))
                else if (cname && mname && strcmp(mname, "<init>") == 0) {
                  size_t callee_argc = countArguments(mdesc) + 1;
                  intptr_t callee_args[16];
                  for (size_t i = 0; i < callee_argc; ++i) {
                    callee_args[callee_argc - 1 - i] = frame.pop();
                  }

                  ClassFile* target_cls = cls;
                  if (machine_) {
                    ClassFile* loaded = machine_->loadClass(cname);
                    if (loaded) target_cls = loaded;
                  }

                  const MethodInfo* target_method = target_cls->findMethod(mname, mdesc);
                  if (target_method && target_method->has_code) {
                    intptr_t sub_res = 0;
                    JavaThrowable* sub_ex = nullptr;
                    executeMethod(target_cls, target_method, callee_args, callee_argc, &sub_res, &sub_ex);
                  }
                }
              }
            }
          }
          break;
        }

        case op_invokestatic: {
          if (pc + 1 < length) {
            uint16_t midx = (static_cast<uint16_t>(code[pc]) << 8) | code[pc + 1];
            pc += 2;
            const ConstantPoolEntry* cp = cls->getConstantPoolEntry(midx);
            if (cp && cp->tag == ConstantMethodref) {
              const char* cname = cls->getClassName(cp->ref.class_index);
              const ConstantPoolEntry* nt = cls->getConstantPoolEntry(cp->ref.name_and_type_index);
              if (nt && nt->tag == ConstantNameAndType) {
                const char* mname = cls->getUtf8(nt->name_and_type.name_index);
                const char* mdesc = cls->getUtf8(nt->name_and_type.descriptor_index);

                // --- java/lang/System.getProperty(String) ---
                if (cname && mname && strcmp(cname, "java/lang/System") == 0 && strcmp(mname, "getProperty") == 0) {
                  intptr_t prop_ref = frame.pop();
                  intptr_t res_str = 0;
                  if (prop_ref != 0) {
                    JavaString* js = reinterpret_cast<JavaString*>(prop_ref);
                    const char* val = getSystemProperty(js->utf8_bytes, js->length);
                    if (val) {
                      JavaString* res_js = allocateJavaString(val, strlen(val));
                      res_str = reinterpret_cast<intptr_t>(res_js);
                    }
                  }
                  frame.push(res_str);
                }
                // --- java/lang/System.currentTimeMillis() ---
                else if (cname && mname && strcmp(cname, "java/lang/System") == 0 && strcmp(mname, "currentTimeMillis") == 0) {
                  frame.push(2026091500ULL);
                }
                // --- java/lang/Integer.parseInt(String) ---
                else if (cname && mname && strcmp(cname, "java/lang/Integer") == 0 && strcmp(mname, "parseInt") == 0) {
                  intptr_t str_ref = frame.pop();
                  int32_t val = 0;
                  if (str_ref != 0) {
                    JavaString* js = reinterpret_cast<JavaString*>(str_ref);
                    bool neg = false;
                    size_t i = 0;
                    if (js->length > 0 && js->utf8_bytes[0] == '-') { neg = true; i = 1; }
                    for (; i < js->length; ++i) {
                      char c = js->utf8_bytes[i];
                      if (c >= '0' && c <= '9') {
                        val = val * 10 + (c - '0');
                      }
                    }
                    if (neg) val = -val;
                  }
                  frame.push(val);
                }
                // --- java/lang/Integer.toString(int) ---
                else if (cname && mname && strcmp(cname, "java/lang/Integer") == 0 && strcmp(mname, "toString") == 0) {
                  int32_t val = static_cast<int32_t>(frame.pop());
                  char num_buf[32];
                  int nlen = 0;
                  if (val == 0) {
                    num_buf[nlen++] = '0';
                  } else {
                    bool neg = val < 0;
                    uint32_t uval = neg ? -val : val;
                    char tmp[16];
                    int tlen = 0;
                    while (uval > 0) {
                      tmp[tlen++] = '0' + (uval % 10);
                      uval /= 10;
                    }
                    if (neg) num_buf[nlen++] = '-';
                    while (tlen > 0) {
                      num_buf[nlen++] = tmp[--tlen];
                    }
                  }
                  num_buf[nlen] = '\0';
                  JavaString* js = allocateJavaString(num_buf, nlen);
                  frame.push(reinterpret_cast<intptr_t>(js));
                }
                // --- java/lang/Boolean.valueOf(String) ---
                else if (cname && mname && strcmp(cname, "java/lang/Boolean") == 0 && strcmp(mname, "valueOf") == 0) {
                  intptr_t str_ref = frame.pop();
                  bool bval = false;
                  if (str_ref != 0) {
                    JavaString* js = reinterpret_cast<JavaString*>(str_ref);
                    if (js->length == 4 &&
                        (js->utf8_bytes[0] == 't' || js->utf8_bytes[0] == 'T') &&
                        (js->utf8_bytes[1] == 'r' || js->utf8_bytes[1] == 'R') &&
                        (js->utf8_bytes[2] == 'u' || js->utf8_bytes[2] == 'U') &&
                        (js->utf8_bytes[3] == 'e' || js->utf8_bytes[3] == 'E')) {
                      bval = true;
                    }
                  }
                  frame.push(bval ? 1 : 0);
                }
                // --- General Static Method Call ---
                else {
                  ClassFile* target_cls = cls;
                  if (cname && machine_) {
                    ClassFile* loaded = machine_->loadClass(cname);
                    if (loaded) target_cls = loaded;
                  }

                  const MethodInfo* target_method = target_cls->findMethod(mname, mdesc);
                  if (target_method && target_method->has_code) {
                    size_t callee_argc = countArguments(mdesc);
                    intptr_t callee_args[16];
                    for (size_t i = 0; i < callee_argc; ++i) {
                      callee_args[callee_argc - 1 - i] = frame.pop();
                    }

                    intptr_t sub_res = 0;
                    JavaThrowable* sub_ex = nullptr;
                    int sub_rc = executeMethod(target_cls, target_method, callee_args, callee_argc, &sub_res, &sub_ex);

                    if (sub_rc != 0 || sub_ex != nullptr) {
                      bool handled = handleException(cls, method, current_instruction_pc, sub_ex, &frame, &pc, outPendingException);
                      if (!handled) {
                        if (outPendingException) *outPendingException = sub_ex;
                        return 1;
                      }
                    } else {
                      if (mdesc[strlen(mdesc) - 1] != 'V') {
                        frame.push(sub_res);
                      }
                    }
                  } else {
                    if (system_) {
                      system_->print("[JVM ERROR] Static method not found: ");
                      system_->print(cname ? cname : "?");
                      system_->print(".");
                      system_->print(mname ? mname : "?");
                      system_->print("\n");
                    }
                    return 1;
                  }
                }
              }
            }
          }
          break;
        }

        case op_areturn:
        case op_ireturn: {
          intptr_t val = frame.pop();
          if (outResult) *outResult = val;
          return static_cast<int>(val);
        }

        case op_return:
          return 0;

        default:
          break;
      }
    }
    return 0;
  }

  // Backwards-compatible raw bytecode execution for Phase 2 test suite
  virtual int32_t execute(const uint8_t* code, size_t length) override {
    if (!code || length == 0) return 0;

    ExecutionFrame frame;
    size_t pc = 0;

    while (pc < length) {
      uint8_t op = code[pc++];
      switch (op) {
        case op_nop:
          break;
        case op_iconst_0:
          frame.push(0);
          break;
        case op_iconst_1:
          frame.push(1);
          break;
        case op_iconst_2:
          frame.push(2);
          break;
        case op_iconst_3:
          frame.push(3);
          break;
        case op_iconst_4:
          frame.push(4);
          break;
        case op_iconst_5:
          frame.push(5);
          break;
        case op_bipush:
          if (pc < length) {
            frame.push(static_cast<int8_t>(code[pc++]));
          }
          break;
        case op_iload_0:
          frame.push(frame.locals[0]);
          break;
        case op_iload_1:
          frame.push(frame.locals[1]);
          break;
        case op_istore_0:
          frame.locals[0] = frame.pop();
          break;
        case op_istore_1:
          frame.locals[1] = frame.pop();
          break;
        case op_iadd: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          frame.push(a + b);
          break;
        }
        case op_isub: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          frame.push(a - b);
          break;
        }
        case op_imul: {
          int32_t b = static_cast<int32_t>(frame.pop());
          int32_t a = static_cast<int32_t>(frame.pop());
          frame.push(a * b);
          break;
        }
        case op_ireturn:
          return static_cast<int32_t>(frame.pop());
        case op_return:
          return 0;
        default:
          break;
      }
    }
    return frame.sp > 0 ? static_cast<int32_t>(frame.pop()) : 0;
  }

 private:
  JavaString* allocateJavaString(const char* text, size_t len) {
    if (!heap_) return nullptr;
    void* str_cls = reinterpret_cast<void*>(0xCAFE0002);
    JavaString* jstr = static_cast<JavaString*>(
        heap_->allocateObject(str_cls, sizeof(JavaString)));
    if (jstr) {
      char* dup = static_cast<char*>(heap_->allocateObject(reinterpret_cast<void*>(0xCAFE0004), len + 1));
      if (dup) {
        if (text && len > 0) memcpy(dup, text, len);
        dup[len] = '\0';
        jstr->utf8_bytes = dup;
        jstr->length = static_cast<uint16_t>(len);
      } else {
        jstr->utf8_bytes = "";
        jstr->length = 0;
      }
    }
    return jstr;
  }

  void pushConstant(ClassFile* cls, uint16_t cidx, ExecutionFrame* frame) {
    const ConstantPoolEntry* cp = cls->getConstantPoolEntry(cidx);
    if (cp) {
      if (cp->tag == ConstantString) {
        uint16_t slen = 0;
        const char* sbytes = cls->getUtf8(cp->string_info.string_index, &slen);
        JavaString* jstr = allocateJavaString(sbytes, slen);
        frame->push(reinterpret_cast<intptr_t>(jstr));
      } else if (cp->tag == ConstantInteger) {
        frame->push(cp->int_val);
      }
    }
  }

  const char* getSystemProperty(const char* key, size_t key_len) {
    if (!key) return nullptr;
    if (key_len == 7 && strncmp(key, "os.name", 7) == 0) return "ATOMS OS (BOS Kernel)";
    if (key_len == 7 && strncmp(key, "os.arch", 7) == 0) return "x86_64";
    if (key_len == 12 && strncmp(key, "java.version", 12) == 0) return "1.8.0-atoms";
    if (key_len == 11 && strncmp(key, "java.vendor", 11) == 0) return "ATOMS Project";
    if (key_len == 8 && strncmp(key, "user.dir", 8) == 0) return "/apps";
    return nullptr;
  }

  void raiseException(const ClassFile* cls, const MethodInfo* method, uint16_t current_pc, const char* ex_name, const char* message, ExecutionFrame* frame, size_t* pc, JavaThrowable** outPendingException) {
    void* ex_cls = reinterpret_cast<void*>(0xCAFE0030);
    JavaThrowable* ex = static_cast<JavaThrowable*>(
        heap_->allocateObject(ex_cls, sizeof(JavaThrowable)));
    if (ex) {
      ex->class_name = ex_name;
      ex->message = message;
    }
    bool handled = handleException(cls, method, current_pc, ex, frame, pc, outPendingException);
    if (!handled) {
      if (outPendingException) *outPendingException = ex;
    }
  }

  bool handleException(const ClassFile* cls, const MethodInfo* method, uint16_t current_pc, JavaThrowable* ex, ExecutionFrame* frame, size_t* pc, JavaThrowable** outPendingException) {
    (void)outPendingException;
    const char* ex_class_name = (ex && ex->class_name) ? ex->class_name : "java/lang/Throwable";
    const ExceptionTableEntry* handler = cls->findExceptionHandler(method, current_pc, ex_class_name);
    if (handler) {
      // Clear operand stack and push exception object reference
      frame->sp = 0;
      frame->push(reinterpret_cast<intptr_t>(ex));
      *pc = handler->handler_pc;
      return true;
    }
    return false;
  }

  size_t countArguments(const char* desc) {
    if (!desc || desc[0] != '(') return 0;
    size_t count = 0;
    size_t idx = 1;
    while (desc[idx] && desc[idx] != ')') {
      if (desc[idx] == 'L') {
        while (desc[idx] && desc[idx] != ';') ++idx;
        if (desc[idx] == ';') ++idx;
        ++count;
      } else if (desc[idx] == '[') {
        ++idx;
      } else {
        ++idx;
        ++count;
      }
    }
    return count;
  }

  system::System* system_;
  heap::Heap* heap_;
  Machine* machine_;
  JavaPrintStream* stdout_stream_;
};

InterpreterProcessor* makeProcessor(system::System* s, heap::Heap* h) {
  if (!s) return nullptr;
  void* mem = s->allocate(sizeof(AvianInterpreterProcessor));
  if (!mem) return nullptr;
  return new (mem) AvianInterpreterProcessor(s, h);
}

int executeBytecodeMethod(system::System* s, heap::Heap* h, Machine* m, ClassFile* cls, const MethodInfo* method, void* argsArray) {
  AvianInterpreterProcessor proc(s, h, m);
  intptr_t args[1] = { reinterpret_cast<intptr_t>(argsArray) };
  JavaThrowable* pending_ex = nullptr;
  int rc = proc.executeMethod(cls, method, args, 1, nullptr, &pending_ex);
  if (pending_ex != nullptr) {
    if (s) {
      s->print("[JVM ERROR] Uncaught Java exception: ");
      s->print(pending_ex->class_name ? pending_ex->class_name : "java/lang/Throwable");
      if (pending_ex->message && pending_ex->message[0] != '\0') {
        s->print(" (");
        s->print(pending_ex->message);
        s->print(")");
      }
      s->print("\n");
    }
    return 1;
  }
  return rc;
}

} // namespace avian
