#include "../include/atom_vm.h"
#include "../../libbos/include/bos.h"
#include "../internal/atom_memory.h"

static void print_dec(uint32_t num) {
    if (num == 0) { bos_print("0"); return; }
    char buf[16]; int i = 14; buf[15] = '\0';
    while (num > 0) { buf[i--] = (num % 10) + '0'; num /= 10; }
    bos_print(&buf[i+1]);
}

static bool is_falsey(AtomValue value) {
    if (atom_is_nil(value)) return true;
    if (atom_is_bool(value)) return !value.as.boolean;
    return false;
}

// --- Helper: Build absolute path ---
static void build_abs_path(const char* input, char* out, int max_len) {
    int i = 0, j = 0;
    if (input[0] != '/') {
        out[j++] = '/';
    }
    while (input[i] && j < max_len - 1) {
        out[j++] = input[i++];
    }
    out[j] = '\0';
}

// --- NATIVE SYSTEM APIs ---
static AtomValue bosl_file_read(int arg_count, AtomValue* args) {
    if (arg_count != 1 || !atom_is_string(args[0])) return atom_value_nil();
    const char* raw_path = args[0].as.string->chars;
    
    char path[256];
    build_abs_path(raw_path, path, 256);
    
    int fd = bos_open(path);
    if (fd < 0) return atom_value_nil();
    
    char* buffer = (char*)atom_alloc(4096);
    if (!buffer) { bos_close(fd); return atom_value_nil(); }
    
    int total = 0;
    int bytes;
    while ((bytes = bos_read(fd, buffer + total, 4095 - total)) > 0) {
        total += bytes;
        if (total >= 4095) break;
    }
    buffer[total] = '\0';
    bos_close(fd);
    
    // atom_string_create returns AtomValue
    AtomValue result = atom_string_create(buffer);
    atom_free(buffer);
    return result;
}

static AtomValue bosl_file_write(int arg_count, AtomValue* args) {
    if (arg_count != 2 || !atom_is_string(args[0]) || !atom_is_string(args[1])) return atom_value_bool(false);
    
    const char* raw_path = args[0].as.string->chars;
    const char* content = args[1].as.string->chars;
    
    char path[256];
    build_abs_path(raw_path, path, 256);
    
    // Create the file if it doesn't exist
    bos_create(path); 
    
    int fd = bos_open(path);
    if (fd < 0) return atom_value_bool(false);
    
    int len = 0;
    while(content[len]) len++;
    
    bos_write(fd, content, len);
    bos_close(fd);
    return atom_value_bool(true);
}
// ------------------------

void atom_vm_init(AtomVM* vm) {
    if (!vm) return;
    
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    
    vm->globals = atom_scope_create(NULL);
    
    // Register standard library
    atom_vm_define_native(vm, "file_read", bosl_file_read);
    atom_vm_define_native(vm, "file_write", bosl_file_write);
}

void atom_vm_define_native(AtomVM* vm, const char* name, AtomNativeFn function) {
    if (!vm || !vm->globals) return;
    AtomValue str_val = atom_string_create(name);
    atom_scope_define(vm->globals, str_val.as.string, atom_value_native(function));
}

void atom_vm_free(AtomVM* vm) {
    if (!vm) return;
    
    // Clear the stack
    while (vm->stack_top > vm->stack) {
        atom_vm_pop(vm); // Does not actually call atom_value_release automatically yet, but we will in the loop
    }
    
    if (vm->globals) {
        atom_scope_destroy(vm->globals);
        vm->globals = NULL;
    }
}

bool atom_vm_push(AtomVM* vm, AtomValue value) {
    if (!vm) return false;
    
    if ((size_t)(vm->stack_top - vm->stack) >= STACK_MAX) {
        bos_print("VM Error: Stack Overflow\n");
        return false;
    }
    
    *vm->stack_top = value;
    vm->stack_top++;
    return true;
}

AtomValue atom_vm_pop(AtomVM* vm) {
    if (!vm || vm->stack_top == vm->stack) {
        return atom_value_error();
    }
    
    vm->stack_top--;
    return *vm->stack_top;
}

// Helper to read a byte from the instruction stream
static uint8_t read_byte(CallFrame* frame) {
    return (*frame->ip++);
}

// Helper to read a constant from the chunk
static AtomValue read_constant(CallFrame* frame, uint32_t index) {
    if (!frame || !frame->function || !frame->function->chunk) return atom_value_error();
    return atom_array_get(frame->function->chunk->constants, index);
}

bool atom_vm_execute(AtomVM* vm, AtomFunction* function) {
    if (!vm || !function || !function->chunk) return false;
    
    vm->frame_count = 0;
    CallFrame* frame = &vm->frames[vm->frame_count++];
    frame->function = function;
    frame->ip = function->chunk->code;
    frame->scope = vm->globals;
    
    while (true) {
        frame = &vm->frames[vm->frame_count - 1];
        
        // Bounds checking
        if ((uint32_t)(frame->ip - frame->function->chunk->code) >= frame->function->chunk->count) {
            bos_print("VM Error: IP out of bounds\n");
            return false;
        }
        
        uint8_t instruction = read_byte(frame);
        
        switch (instruction) {
            case OP_CONSTANT: {
                uint8_t constant_index = read_byte(frame);
                AtomValue constant = read_constant(frame, constant_index);
                
                // If it's a string, we should probably retain it before pushing onto stack
                if (atom_is_string(constant)) {
                    atom_string_retain(constant.as.string);
                }
                
                if (!atom_vm_push(vm, constant)) return false;
                break;
            }
            case OP_NIL:
                if (!atom_vm_push(vm, atom_value_nil())) return false;
                break;
            case OP_TRUE:
                if (!atom_vm_push(vm, atom_value_bool(true))) return false;
                break;
            case OP_FALSE:
                if (!atom_vm_push(vm, atom_value_bool(false))) return false;
                break;
            case OP_POP: {
                AtomValue v = atom_vm_pop(vm);
                atom_value_release(v);
                break;
            }
            case OP_ADD: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                
                if (atom_is_number(a) && atom_is_number(b)) {
                    atom_vm_push(vm, atom_value_number(a.as.number + b.as.number));
                } else {
                    bos_print("VM Error: Operands must be numbers for OP_ADD\n");
                    atom_value_release(a);
                    atom_value_release(b);
                    return false;
                }
                break;
            }
            case OP_SUBTRACT: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                
                if (atom_is_number(a) && atom_is_number(b)) {
                    atom_vm_push(vm, atom_value_number(a.as.number - b.as.number));
                } else {
                    bos_print("VM Error: Operands must be numbers for OP_SUBTRACT\n");
                    atom_value_release(a);
                    atom_value_release(b);
                    return false;
                }
                break;
            }
            case OP_EQUAL: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                atom_vm_push(vm, atom_value_bool(atom_values_equal(a, b)));
                atom_value_release(a);
                atom_value_release(b);
                break;
            }
            case OP_GREATER: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                if (atom_is_number(a) && atom_is_number(b)) {
                    atom_vm_push(vm, atom_value_bool(a.as.number > b.as.number));
                } else {
                    bos_print("VM Error: Operands must be numbers for >\n");
                    return false;
                }
                atom_value_release(a);
                atom_value_release(b);
                break;
            }
            case OP_LESS: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                if (atom_is_number(a) && atom_is_number(b)) {
                    atom_vm_push(vm, atom_value_bool(a.as.number < b.as.number));
                } else {
                    bos_print("VM Error: Operands must be numbers for <\n");
                    return false;
                }
                atom_value_release(a);
                atom_value_release(b);
                break;
            }
            case OP_NOT: {
                AtomValue a = atom_vm_pop(vm);
                atom_vm_push(vm, atom_value_bool(is_falsey(a)));
                atom_value_release(a);
                break;
            }
            case OP_AND: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                bool res = !is_falsey(a) && !is_falsey(b);
                atom_vm_push(vm, atom_value_bool(res));
                atom_value_release(a);
                atom_value_release(b);
                break;
            }
            case OP_OR: {
                AtomValue b = atom_vm_pop(vm);
                AtomValue a = atom_vm_pop(vm);
                bool res = !is_falsey(a) || !is_falsey(b);
                atom_vm_push(vm, atom_value_bool(res));
                atom_value_release(a);
                atom_value_release(b);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = (uint16_t)((read_byte(frame) << 8) | read_byte(frame));
                if (is_falsey(*(vm->stack_top - 1))) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = (uint16_t)((read_byte(frame) << 8) | read_byte(frame));
                frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = (uint16_t)((read_byte(frame) << 8) | read_byte(frame));
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                uint8_t arg_count = read_byte(frame);
                AtomValue callee = *(vm->stack_top - 1 - arg_count);
                
                if (atom_is_native(callee)) {
                    AtomNativeFn native_fn = callee.as.native_fn;
                    AtomValue args[32]; // Max 32 args
                    for (int i = arg_count - 1; i >= 0; i--) {
                        args[i] = atom_vm_pop(vm);
                    }
                    atom_vm_pop(vm); // Pop the native function
                    
                    AtomValue result = native_fn(arg_count, args);
                    atom_vm_push(vm, result);
                    break;
                }
                
                if (!atom_is_function(callee)) { bos_print("VM Error: Can only call functions\n"); return false; }
                
                AtomFunction* func = callee.as.function;
                if (arg_count != func->arity) { bos_print("VM Error: Incorrect arg count\n"); return false; }
                if (vm->frame_count >= FRAMES_MAX) { bos_print("VM Error: Stack overflow\n"); return false; }
                
                CallFrame* new_frame = &vm->frames[vm->frame_count++];
                new_frame->function = func;
                new_frame->ip = func->chunk->code;
                new_frame->scope = atom_scope_create(vm->globals);
                
                for (int i = arg_count - 1; i >= 0; i--) {
                    AtomValue arg = atom_vm_pop(vm);
                    if (func->param_names[i]) {
                        atom_scope_define(new_frame->scope, func->param_names[i], arg);
                    }
                }
                atom_vm_pop(vm); // Pop the function from the stack, but DO NOT release it since it's currently executing!
                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t constant_index = read_byte(frame);
                AtomValue name = read_constant(frame, constant_index);
                if (!atom_is_string(name)) { 
                    bos_print("VM Error: Global name must be string (DEFINE). Index: ");
                    print_dec(constant_index); bos_print(", Type: "); print_dec(name.type);
                    bos_print("\n"); return false; 
                }
                
                AtomValue value = atom_vm_pop(vm);
                atom_scope_define(frame->scope, name.as.string, value);
                break;
            }
            case OP_GET_GLOBAL: {
                uint8_t constant_index = read_byte(frame);
                AtomValue name = read_constant(frame, constant_index);
                if (!atom_is_string(name)) { 
                    bos_print("VM Error: Global name must be string (GET). Index: ");
                    print_dec(constant_index); bos_print(", Type: "); print_dec(name.type);
                    bos_print(", IP: "); print_dec(frame->ip - frame->function->chunk->code);
                    bos_print("\n"); return false; 
                }
                
                AtomValue value = atom_scope_get(frame->scope, name.as.string);
                if (atom_is_nil(value)) {
                    bos_print("VM Error: Undefined global variable\n");
                    return false;
                }
                
                if (atom_is_string(value)) atom_string_retain(value.as.string);
                
                atom_vm_push(vm, value);
                break;
            }
            case OP_BUILD_ARRAY: {
                uint8_t count = read_byte(frame);
                AtomArray* arr = atom_array_create();
                // Pop elements in reverse order
                AtomValue elements[256];
                for (int i = count - 1; i >= 0; i--) {
                    elements[i] = atom_vm_pop(vm);
                }
                for (int i = 0; i < count; i++) {
                    atom_array_push(arr, elements[i]);
                }
                atom_vm_push(vm, atom_value_array(arr));
                break;
            }
            case OP_BUILD_TABLE: {
                uint8_t count = read_byte(frame);
                AtomTable* tbl = atom_table_create();
                // Pop elements in reverse order (value, then key)
                AtomValue elements[256 * 2]; // Max 255 pairs, so 510 elements
                for (int i = (count * 2) - 1; i >= 0; i--) {
                    elements[i] = atom_vm_pop(vm);
                }
                for (int i = 0; i < count; i++) {
                    AtomValue key = elements[i * 2];
                    AtomValue val = elements[i * 2 + 1];
                    if (atom_is_string(key)) {
                        atom_table_set(tbl, key, val);
                    } else {
                        // In BOSL, object keys must be strings currently.
                    }
                }
                atom_vm_push(vm, atom_value_table(tbl));
                break;
            }
            case OP_INDEX_GET: {
                AtomValue index = atom_vm_pop(vm);
                AtomValue array_val = atom_vm_pop(vm);
                
                if (atom_is_array(array_val) && atom_is_number(index)) {
                    AtomArray* arr = array_val.as.array;
                    uint32_t idx = (uint32_t)index.as.number;
                    AtomValue res = atom_array_get(arr, idx);
                    if (res.type == ATOM_TYPE_ERROR) {
                        bos_print("VM Error: Index out of bounds\n");
                        return false;
                    }
                    if (atom_is_string(res)) atom_string_retain(res.as.string);
                    atom_vm_push(vm, res);
                } else if (atom_is_table(array_val) && atom_is_string(index)) {
                    AtomTable* tbl = array_val.as.table;
                    AtomValue res = atom_table_get(tbl, index);
                    if (atom_is_string(res)) atom_string_retain(res.as.string);
                    atom_vm_push(vm, res);
                } else {
                    bos_print("VM Error: Invalid indexing operation\n");
                    return false;
                }
                break;
            }
            case OP_INDEX_SET: {
                AtomValue value = atom_vm_pop(vm);
                AtomValue index = atom_vm_pop(vm);
                AtomValue array_val = atom_vm_pop(vm);
                
                if (atom_is_array(array_val) && atom_is_number(index)) {
                    AtomArray* arr = array_val.as.array;
                    uint32_t idx = (uint32_t)index.as.number;
                    if (idx >= arr->count) {
                        bos_print("VM Error: Index out of bounds for assignment\n");
                        return false;
                    }
                    if (atom_is_string(value)) atom_string_retain(value.as.string);
                    atom_array_set(arr, idx, value);
                    atom_vm_push(vm, value); // Push value back as result of expression
                } else if (atom_is_table(array_val) && atom_is_string(index)) {
                    AtomTable* tbl = array_val.as.table;
                    if (atom_is_string(value)) atom_string_retain(value.as.string);
                    atom_table_set(tbl, index, value);
                    atom_vm_push(vm, value); // Push value back
                } else {
                    bos_print("VM Error: Invalid indexing operation\n");
                    return false;
                }
                break;
            }
            case OP_METHOD_CALL: {
                uint8_t arg_count = read_byte(frame);
                AtomValue method_name = atom_vm_pop(vm);
                AtomValue receiver = *(vm->stack_top - 1 - arg_count);
                
                if (atom_is_array(receiver)) {
                    if (atom_is_string(method_name)) {
                        const char* name = method_name.as.string->chars;
                        if (name[0] == 'p' && name[1] == 'u' && name[2] == 's' && name[3] == 'h' && name[4] == '\0') {
                            if (arg_count != 1) { bos_print("VM Error: push() expects 1 argument\n"); return false; }
                            AtomValue arg = atom_vm_pop(vm);
                            AtomValue array_val = atom_vm_pop(vm); // Pop receiver
                            
                            AtomArray* arr = array_val.as.array;
                            if (atom_is_string(arg)) atom_string_retain(arg.as.string);
                            atom_array_push(arr, arg);
                            
                            atom_vm_push(vm, atom_value_nil());
                            atom_value_release(method_name);
                            break;
                        }
                    }
                }
                
                bos_print("VM Error: Undefined method or receiver type\n");
                return false;
            }
            case OP_PRINT: {
                AtomValue value = atom_vm_pop(vm);
                if (atom_is_number(value)) {
                    print_dec((int)value.as.number);
                    bos_print("\n");
                } else if (atom_is_string(value)) {
                    bos_print(value.as.string->chars);
                    bos_print("\n");
                } else if (atom_is_bool(value)) {
                    bos_print(value.as.boolean ? "1\n" : "0\n");
                } else if (atom_is_nil(value)) {
                    bos_print("nil\n");
                } else {
                    bos_print("VM Print: Unsupported type\n");
                }
                atom_value_release(value);
                break;
            }
            case OP_RETURN: {
                AtomValue result = atom_value_nil();
                // We only pop if there is something to pop!
                if (vm->stack_top > vm->stack) {
                    result = atom_vm_pop(vm);
                }
                
                vm->frame_count--;
                if (vm->frame_count == 0) {
                    atom_vm_push(vm, result);
                    return true;
                }
                
                atom_scope_destroy(frame->scope);
                atom_vm_push(vm, result);
                break;
            }
            default:
                bos_print("VM Error: Unknown opcode\n");
                return false;
        }
    }
    
    return true;
}
