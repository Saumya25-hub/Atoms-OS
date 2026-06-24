#ifndef ATOM_VALUE_H
#define ATOM_VALUE_H

#include "atom_types.h"

// Forward declarations for container types
typedef struct AtomString AtomString;
typedef struct AtomArray AtomArray;
typedef struct AtomTable AtomTable;

// The Core Tagged Value (16 Bytes)
typedef struct AtomValue {
    AtomType type;          // 4 bytes (default C enum size)
    uint32_t id;            // 4 bytes Unique Object ID (AtomID)
    union {                 // 8 bytes union
        bool       boolean;
        double     number;  // Unified number type
        AtomString* string;
        AtomArray*  array;
        AtomTable*  table;
        AtomFunction* function;
        void*      object;
    } as;
} AtomValue;

_Static_assert(sizeof(struct AtomValue) == 16, "AtomValue must be exactly 16 bytes");

// Constructors (Boxing)
AtomValue atom_value_nil(void);
AtomValue atom_value_bool(bool value);
AtomValue atom_value_number(double value);
AtomValue atom_value_error(void);

// For objects, the ID will be populated internally or during creation.
// Strings, Arrays, Tables will have their own boxing functions in their respective modules.
AtomValue atom_value_string(struct AtomString* string);

// Predicates
static inline bool atom_is_nil(AtomValue val)    { return val.type == ATOM_TYPE_NIL; }
static inline bool atom_is_bool(AtomValue val)   { return val.type == ATOM_TYPE_BOOL; }
static inline bool atom_is_number(AtomValue val) { return val.type == ATOM_TYPE_NUMBER; }
static inline bool atom_is_string(AtomValue val) { return val.type == ATOM_TYPE_STRING; }
static inline bool atom_is_array(AtomValue val)  { return val.type == ATOM_TYPE_ARRAY; }
static inline bool atom_is_table(AtomValue val)  { return val.type == ATOM_TYPE_TABLE; }
static inline bool atom_is_error(AtomValue val)  { return val.type == ATOM_TYPE_ERROR; }

// Unified Destruction (recursively releases containers and strings)
void atom_value_release(AtomValue val);

// Equality
bool atom_values_equal(AtomValue a, AtomValue b);

#endif // ATOM_VALUE_H
