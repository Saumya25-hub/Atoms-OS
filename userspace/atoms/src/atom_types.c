#include "../include/atom_types.h"

const char* atom_type_to_string(AtomType type) {
    switch (type) {
        case ATOM_TYPE_NIL:    return "NIL";
        case ATOM_TYPE_BOOL:   return "BOOL";
        case ATOM_TYPE_NUMBER: return "NUMBER";
        case ATOM_TYPE_STRING: return "STRING";
        case ATOM_TYPE_ARRAY:  return "ARRAY";
        case ATOM_TYPE_TABLE:  return "TABLE";
        case ATOM_TYPE_OBJECT: return "OBJECT";
        case ATOM_TYPE_ERROR:  return "ERROR";
        default:               return "UNKNOWN";
    }
}
