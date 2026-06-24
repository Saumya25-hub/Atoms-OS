#ifndef ATOM_TABLE_H
#define ATOM_TABLE_H

#include "atom_value.h"

typedef struct {
    AtomValue key;     // If key.type == ATOM_TYPE_NIL, the slot is empty
    AtomValue value;
} AtomTableEntry;

struct AtomTable {
    uint32_t magic;      // Magic number for safety (0xA70FA6A2)
    AtomTableEntry* entries;
    uint32_t capacity;   // Total allocated capacity (must be a power of 2 for fast modulo)
    uint32_t count;      // Number of active elements
};

AtomTable* atom_table_create(void);
void atom_table_destroy(AtomTable* table);

// Table API
bool atom_table_set(AtomTable* table, AtomValue key, AtomValue value);
AtomValue atom_table_get(AtomTable* table, AtomValue key);
bool atom_table_delete(AtomTable* table, AtomValue key);

// Box table into AtomValue
AtomValue atom_value_table(AtomTable* table);

#endif // ATOM_TABLE_H
