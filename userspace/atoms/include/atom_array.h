#ifndef ATOM_ARRAY_H
#define ATOM_ARRAY_H

#include "atom_value.h"

// AtomArray structure
struct AtomArray {
    uint32_t magic;      // Magic number for safety (0xA70FA6A1)
    AtomValue* data;     // Buffer for elements
    uint32_t capacity;   // Total allocated capacity
    uint32_t count;      // Number of active elements
};

// Array API
AtomArray* atom_array_create(void);
void atom_array_destroy(AtomArray* array);

bool atom_array_push(AtomArray* array, AtomValue value);
AtomValue atom_array_pop(AtomArray* array);
AtomValue atom_array_get(AtomArray* array, uint32_t index);
bool atom_array_set(AtomArray* array, uint32_t index, AtomValue value);

// Box array into AtomValue
AtomValue atom_value_array(AtomArray* array);

#endif // ATOM_ARRAY_H
