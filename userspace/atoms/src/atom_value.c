#include "../include/atom_value.h"
#include "../include/atom_string.h"
#include "../include/atom_array.h"
#include "../include/atom_table.h"
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

AtomValue atom_value_string(struct AtomString* string) {
    AtomValue v;
    v.type = ATOM_TYPE_STRING;
    v.id = 0;
    v.as.string = string;
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
        case ATOM_TYPE_FUNCTION:
            return a.as.function == b.as.function; // Reference equality
        default:
            return false;
    }
}

void atom_value_release(AtomValue val) {
    switch (val.type) {
        case ATOM_TYPE_STRING:
            if (val.as.string) atom_string_release(val.as.string);
            break;
        case ATOM_TYPE_ARRAY:
            if (val.as.array) atom_array_destroy(val.as.array);
            break;
        case ATOM_TYPE_TABLE:
            if (val.as.table) atom_table_destroy(val.as.table);
            break;
        case ATOM_TYPE_FUNCTION:
            if (val.as.function) {
                extern void atom_function_destroy(struct AtomFunction*);
                atom_function_destroy(val.as.function);
            }
            break;
        // Primitive types and Object/Error require no release action
        default:
            break;
    }
}
