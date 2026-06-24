#include "../include/atom_vm.h"
#include "../../libbos/include/bos.h"

void atom_vm_init(AtomVM* vm) {
    if (!vm) return;
    
    vm->stack_top = vm->stack;
    vm->current_function = NULL;
    vm->ip = NULL;
    
    vm->globals = atom_scope_create(NULL);
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
static uint8_t read_byte(AtomVM* vm) {
    return (*vm->ip++);
}

// Helper to read a constant from the chunk
static AtomValue read_constant(AtomVM* vm, uint32_t index) {
    if (!vm || !vm->current_function || !vm->current_function->chunk) return atom_value_error();
    return atom_array_get(vm->current_function->chunk->constants, index);
}

bool atom_vm_execute(AtomVM* vm, AtomFunction* function) {
    if (!vm || !function || !function->chunk) return false;
    
    vm->current_function = function;
    vm->ip = function->chunk->code;
    
    while (true) {
        // Bounds checking
        if ((uint32_t)(vm->ip - function->chunk->code) >= function->chunk->count) {
            bos_print("VM Error: IP out of bounds\n");
            return false;
        }
        
        uint8_t instruction = read_byte(vm);
        
        switch (instruction) {
            case OP_CONSTANT: {
                uint8_t constant_index = read_byte(vm);
                AtomValue constant = read_constant(vm, constant_index);
                
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
            case OP_RETURN: {
                // Return from function execution
                return true;
            }
            default:
                bos_print("VM Error: Unknown opcode\n");
                return false;
        }
    }
    
    return true;
}
