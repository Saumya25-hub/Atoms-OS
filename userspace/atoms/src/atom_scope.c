#include "../include/atom_scope.h"
#include "../internal/atom_memory.h"
#include "../include/atom_value.h"

AtomScope* atom_scope_create(AtomScope* enclosing) {
    AtomScope* scope = (AtomScope*)atom_alloc(sizeof(AtomScope));
    if (!scope) return NULL;
    
    scope->locals = atom_table_create();
    if (!scope->locals) {
        atom_free(scope);
        return NULL;
    }
    
    scope->enclosing = enclosing;
    return scope;
}

void atom_scope_destroy(AtomScope* scope) {
    if (!scope) return;
    
    if (scope->locals) {
        atom_table_destroy(scope->locals);
    }
    
    atom_free(scope);
}

bool atom_scope_define(AtomScope* scope, AtomString* name, AtomValue value) {
    if (!scope || !name) return false;
    
    // We increment string ref count because it's used as a key
    atom_string_retain(name);
    AtomValue key = atom_value_string(name);
    
    return atom_table_set(scope->locals, key, value);
}

bool atom_scope_assign(AtomScope* scope, AtomString* name, AtomValue value) {
    if (!scope || !name) return false;
    
    AtomScope* current = scope;
    AtomValue key = atom_value_string(name); // Temporary value for lookup
    
    while (current) {
        AtomValue existing = atom_table_get(current->locals, key);
        if (!atom_is_nil(existing)) {
            // Found it, update it
            // atom_table_set takes ownership of the key, so we need to retain the string
            atom_string_retain(name);
            AtomValue new_key = atom_value_string(name);
            return atom_table_set(current->locals, new_key, value);
        }
        current = current->enclosing;
    }
    
    return false; // Undefined variable
}

AtomValue atom_scope_get(AtomScope* scope, AtomString* name) {
    if (!scope || !name) return atom_value_nil();
    
    AtomScope* current = scope;
    AtomValue key = atom_value_string(name); // Temporary value for lookup
    
    while (current) {
        AtomValue existing = atom_table_get(current->locals, key);
        if (!atom_is_nil(existing)) {
            // Found it, we return it but we don't transfer ownership! 
            // In a real VM we'd have to handle ref counts carefully on the stack.
            // For now, returning it directly implies the caller will use it or retain it if stored.
            return existing;
        }
        current = current->enclosing;
    }
    
    return atom_value_nil();
}
