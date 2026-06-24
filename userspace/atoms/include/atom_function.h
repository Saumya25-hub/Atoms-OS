#ifndef ATOM_FUNCTION_H
#define ATOM_FUNCTION_H

#include "atom_types.h"
#include "atom_string.h"
#include "atom_bytecode.h"

// A BOSL Function Object
struct AtomFunction {
    uint32_t magic;
    uint32_t arity;      // Number of expected arguments
    AtomString* name;    // Name of the function (can be NULL for anonymous)
    AtomChunk* chunk;    // The bytecode instructions and constants
};

AtomFunction* atom_function_create(AtomString* name, uint32_t arity);
void atom_function_destroy(AtomFunction* function);

AtomValue atom_value_function(AtomFunction* function);
bool atom_is_function(AtomValue value);

#endif // ATOM_FUNCTION_H
