#include "../include/atom_function.h"
#include "../internal/atom_memory.h"

AtomFunction* atom_function_create(AtomString* name, uint32_t arity) {
    AtomFunction* func = (AtomFunction*)atom_alloc(sizeof(AtomFunction));
    if (!func) return NULL;
    
    func->magic = ATOM_FUNC_MAGIC;
    func->arity = arity;
    func->name = name;
    
    // Increment string ref count since the function holds it
    if (name) {
        atom_string_retain(name);
    }
    
    for (int i = 0; i < 8; i++) func->param_names[i] = NULL;
    
    func->chunk = atom_chunk_create();
    if (!func->chunk) {
        if (name) atom_string_release(name);
        atom_free(func);
        return NULL;
    }
    
    return func;
}

void atom_function_destroy(AtomFunction* function) {
    if (!function || function->magic != ATOM_FUNC_MAGIC) return;
    
    if (function->name) {
        atom_string_release(function->name);
    }
    
    if (function->chunk) {
        atom_chunk_destroy(function->chunk);
    }
    
    function->magic = ATOM_FREED_MAGIC;
    atom_free(function);
}
