#include "../include/atom_value.h"
#include "../../libbos/include/bos.h"

// Global Object ID counter (Starts at 1000)
uint32_t next_atom_id = 1000;

static uint32_t get_next_id(void) {
    return next_atom_id++;
}

AtomValue atom_value_nil(void) {
    AtomValue v;
    v.type = ATOM_TYPE_NIL;
    v.id = 0; // NIL has no unique ID
    v.as.object = NULL;
    return v;
}

AtomValue atom_value_bool(bool value) {
    AtomValue v;
    v.type = ATOM_TYPE_BOOL;
    v.id = 0; // Primitives don't really need unique IDs
    v.as.boolean = value;
    return v;
}

AtomValue atom_value_number(double value) {
    AtomValue v;
    v.type = ATOM_TYPE_NUMBER;
    v.id = 0; // Primitives don't need unique IDs
    v.as.number = value;
    return v;
}

AtomValue atom_value_error(void) {
    AtomValue v;
    v.type = ATOM_TYPE_ERROR;
    v.id = 0;
    v.as.object = NULL;
    return v;
}

bool atom_values_equal(AtomValue a, AtomValue b) {
    if (a.type != b.type) return false;
    
    switch (a.type) {
        case ATOM_TYPE_NIL:
        case ATOM_TYPE_ERROR:
            return true;
        case ATOM_TYPE_BOOL:
            return a.as.boolean == b.as.boolean;
        case ATOM_TYPE_NUMBER:
            return a.as.number == b.as.number; // Note: Double equality, careful with NaNs in future
        case ATOM_TYPE_STRING:
            return a.as.string == b.as.string; // Pointer comparison! (Interned)
        case ATOM_TYPE_ARRAY:
            return a.as.array == b.as.array;   // Reference equality
        case ATOM_TYPE_TABLE:
            return a.as.table == b.as.table;   // Reference equality
        case ATOM_TYPE_OBJECT:
            return a.as.object == b.as.object; // Reference equality
        default:
            return false;
    }
}
