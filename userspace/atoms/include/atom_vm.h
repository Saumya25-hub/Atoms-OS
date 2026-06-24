#ifndef ATOM_VM_H
#define ATOM_VM_H

#include "atom_types.h"
#include "atom_function.h"
#include "atom_scope.h"

#define STACK_MAX 256

// The Execution Environment State
typedef struct {
    AtomFunction* current_function;
    uint8_t* ip; // Instruction pointer
    
    AtomValue stack[STACK_MAX];
    AtomValue* stack_top;
    
    AtomScope* globals; // Global environment
} AtomVM;

// Initialize the VM state
void atom_vm_init(AtomVM* vm);

// Clean up the VM state
void atom_vm_free(AtomVM* vm);

// Push a value onto the VM stack
bool atom_vm_push(AtomVM* vm, AtomValue value);

// Pop a value from the VM stack
AtomValue atom_vm_pop(AtomVM* vm);

// A simple interpreter skeleton for testing bytecode logic
bool atom_vm_execute(AtomVM* vm, AtomFunction* function);

#endif // ATOM_VM_H
