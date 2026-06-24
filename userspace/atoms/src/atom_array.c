#include "../include/atom_array.h"
#include "../internal/atom_memory.h"
#include "../../libbos/include/bos.h"

#define ATOM_ARRAY_INIT_CAPACITY 8

AtomArray* atom_array_create(void) {
    AtomArray* arr = (AtomArray*)atom_alloc(sizeof(AtomArray));
    if (!arr) return NULL;

    arr->data = (AtomValue*)atom_alloc(sizeof(AtomValue) * ATOM_ARRAY_INIT_CAPACITY);
    if (!arr->data) {
        atom_free(arr);
        return NULL;
    }

    arr->magic = ATOM_ARRAY_MAGIC;
    arr->capacity = ATOM_ARRAY_INIT_CAPACITY;
    arr->count = 0;
    return arr;
}

void atom_array_destroy(AtomArray* array) {
    if (!array || array->magic != ATOM_ARRAY_MAGIC) return;

    // Release all contained values
    for (uint32_t i = 0; i < array->count; i++) {
        atom_value_release(array->data[i]);
    }

    // Zero magic to catch use-after-free
    array->magic = 0xDEADDEAD;

    if (array->data) {
        atom_free(array->data);
        array->data = NULL;
    }

    atom_free(array);
}

bool atom_array_push(AtomArray* array, AtomValue value) {
    if (!array || array->magic != ATOM_ARRAY_MAGIC) return false;

    if (array->count >= array->capacity) {
        uint32_t new_cap = array->capacity * 2;
        AtomValue* new_data = (AtomValue*)atom_alloc(sizeof(AtomValue) * new_cap);
        if (!new_data) return false;

        // Copy old data
        for (uint32_t i = 0; i < array->count; i++) {
            new_data[i] = array->data[i];
        }

        atom_free(array->data);
        array->data = new_data;
        array->capacity = new_cap;
    }

    array->data[array->count++] = value;
    return true;
}

AtomValue atom_array_pop(AtomArray* array) {
    if (!array || array->magic != ATOM_ARRAY_MAGIC || array->count == 0) {
        return atom_value_error();
    }
    return array->data[--array->count];
}

AtomValue atom_array_get(AtomArray* array, uint32_t index) {
    if (!array || array->magic != ATOM_ARRAY_MAGIC || index >= array->count) {
        return atom_value_error();
    }
    return array->data[index];
}

bool atom_array_set(AtomArray* array, uint32_t index, AtomValue value) {
    if (!array || array->magic != ATOM_ARRAY_MAGIC || index >= array->count) {
        return false;
    }
    // Release old value
    atom_value_release(array->data[index]);
    
    // Take ownership of new value
    array->data[index] = value;
    return true;
}

AtomValue atom_value_array(AtomArray* array) {
    AtomValue v;
    v.type = ATOM_TYPE_ARRAY;
    v.id = 0; // Handled later if we implement handles or object IDs for debugger
    v.as.array = array;
    return v;
}
