#include "../include/atom_table.h"
#include "../include/atom_string.h"
#include "../internal/atom_memory.h"
#include "../internal/atom_hash.h"
#include "../../libbos/include/bos.h"

#define ATOM_TABLE_INIT_CAPACITY 8
#define ATOM_TABLE_MAX_LOAD 0.75

AtomTable* atom_table_create(void) {
    AtomTable* table = (AtomTable*)atom_alloc(sizeof(AtomTable));
    if (!table) return NULL;

    table->entries = (AtomTableEntry*)atom_alloc(sizeof(AtomTableEntry) * ATOM_TABLE_INIT_CAPACITY);
    if (!table->entries) {
        atom_free(table);
        return NULL;
    }

    for (uint32_t i = 0; i < ATOM_TABLE_INIT_CAPACITY; i++) {
        table->entries[i].key = atom_value_nil();
        table->entries[i].value = atom_value_nil();
    }

    table->magic = ATOM_TABLE_MAGIC;
    table->capacity = ATOM_TABLE_INIT_CAPACITY;
    table->count = 0;
    return table;
}

void atom_table_destroy(AtomTable* table) {
    if (!table || table->magic != ATOM_TABLE_MAGIC) return;

    for (uint32_t i = 0; i < table->capacity; i++) {
        if (!atom_is_nil(table->entries[i].key)) {
            atom_value_release(table->entries[i].key);
            atom_value_release(table->entries[i].value);
        }
    }

    table->magic = 0xDEADDEAD;

    if (table->entries) {
        atom_free(table->entries);
        table->entries = NULL;
    }

    atom_free(table);
}

static uint32_t hash_value(AtomValue val) {
    switch (val.type) {
        case ATOM_TYPE_NIL: return 0;
        case ATOM_TYPE_BOOL: return val.as.boolean ? 1 : 0;
        case ATOM_TYPE_NUMBER: {
            double d = val.as.number;
            return atom_hash_bytes((const char*)&d, sizeof(double));
        }
        case ATOM_TYPE_STRING:
            return atom_hash_string(val.as.string->chars);
        default:
            return 0; // Unsupported keys hash to 0
    }
}

static AtomTableEntry* find_entry(AtomTableEntry* entries, uint32_t capacity, AtomValue key) {
    uint32_t index = hash_value(key) % capacity;
    
    while (true) {
        AtomTableEntry* entry = &entries[index];
        if (atom_is_nil(entry->key)) {
            return entry; // Empty slot found
        } else if (atom_values_equal(entry->key, key)) {
            return entry; // Key found
        }
        index = (index + 1) % capacity;
    }
}

static bool adjust_capacity(AtomTable* table, uint32_t new_capacity) {
    AtomTableEntry* new_entries = (AtomTableEntry*)atom_alloc(sizeof(AtomTableEntry) * new_capacity);
    if (!new_entries) return false;

    for (uint32_t i = 0; i < new_capacity; i++) {
        new_entries[i].key = atom_value_nil();
        new_entries[i].value = atom_value_nil();
    }

    // Rehash entries
    for (uint32_t i = 0; i < table->capacity; i++) {
        AtomTableEntry* entry = &table->entries[i];
        if (!atom_is_nil(entry->key)) {
            AtomTableEntry* dest = find_entry(new_entries, new_capacity, entry->key);
            dest->key = entry->key;
            dest->value = entry->value;
        }
    }

    atom_free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    return true;
}

bool atom_table_set(AtomTable* table, AtomValue key, AtomValue value) {
    if (!table || table->magic != ATOM_TABLE_MAGIC) return false;

    // Reject complex keys for now
    if (atom_is_array(key) || atom_is_table(key) || atom_is_nil(key)) {
        return false;
    }

    if (table->count + 1 > table->capacity * ATOM_TABLE_MAX_LOAD) {
        if (!adjust_capacity(table, table->capacity * 2)) return false;
    }

    AtomTableEntry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new = atom_is_nil(entry->key);
    
    if (is_new) {
        table->count++;
        entry->key = key; // Take ownership
    } else {
        // We are replacing an existing key.
        // We only release the old value. 
        // We also must release the new key because we are not keeping it (we already have the old key).
        atom_value_release(entry->value);
        atom_value_release(key);
    }
    
    entry->value = value; // Take ownership
    return true;
}

AtomValue atom_table_get(AtomTable* table, AtomValue key) {
    if (!table || table->magic != ATOM_TABLE_MAGIC) return atom_value_nil();
    if (atom_is_nil(key)) return atom_value_nil();

    AtomTableEntry* entry = find_entry(table->entries, table->capacity, key);
    if (atom_is_nil(entry->key)) return atom_value_nil();
    
    return entry->value;
}

// Tombstone implementation omitted for V1 simplicity. 
// A full delete requires tombstones for linear probing.
// For V1, we simply do not support delete to keep it extremely simple, or we can just set to NIL (which breaks probe chains).
// Let's implement a clean delete that shifts following entries to prevent probe chain breakage.

bool atom_table_delete(AtomTable* table, AtomValue key) {
    if (!table || table->magic != ATOM_TABLE_MAGIC || atom_is_nil(key)) return false;

    uint32_t index = hash_value(key) % table->capacity;
    
    while (!atom_is_nil(table->entries[index].key)) {
        if (atom_values_equal(table->entries[index].key, key)) {
            // Found it. Release ownership
            atom_value_release(table->entries[index].key);
            atom_value_release(table->entries[index].value);
            
            table->entries[index].key = atom_value_nil();
            table->entries[index].value = atom_value_nil();
            table->count--;

            // Shift entries to close the gap in linear probe chain
            uint32_t next_index = (index + 1) % table->capacity;
            while (!atom_is_nil(table->entries[next_index].key)) {
                AtomTableEntry* next_entry = &table->entries[next_index];
                uint32_t proper_index = hash_value(next_entry->key) % table->capacity;
                
                // If the next_entry is displaced, we might need to move it back to `index`
                // Distance from proper index to current index
                bool displace = false;
                if (index <= next_index) {
                    if (proper_index <= index || proper_index > next_index) displace = true;
                } else {
                    if (proper_index <= index && proper_index > next_index) displace = true;
                }

                if (displace) {
                    table->entries[index] = *next_entry;
                    next_entry->key = atom_value_nil();
                    next_entry->value = atom_value_nil();
                    index = next_index;
                }
                next_index = (next_index + 1) % table->capacity;
            }
            return true;
        }
        index = (index + 1) % table->capacity;
    }
    return false;
}

AtomValue atom_value_table(AtomTable* table) {
    AtomValue v;
    v.type = ATOM_TYPE_TABLE;
    v.id = 0; // For future IDE debug handles
    v.as.table = table;
    return v;
}
