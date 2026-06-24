#ifndef ATOM_SCOPE_H
#define ATOM_SCOPE_H

#include "atom_types.h"
#include "atom_table.h"
#include "atom_string.h"

// AtomScope is the environment for variables.
// It maps string names to AtomValues.
// It optionally has a parent scope for lexical scoping.
typedef struct AtomScope {
    AtomTable* locals;
    struct AtomScope* enclosing;
} AtomScope;

AtomScope* atom_scope_create(AtomScope* enclosing);
void atom_scope_destroy(AtomScope* scope);

// Define a variable in the current scope
bool atom_scope_define(AtomScope* scope, AtomString* name, AtomValue value);

// Assign to an existing variable in this or an enclosing scope
bool atom_scope_assign(AtomScope* scope, AtomString* name, AtomValue value);

// Get a variable's value from this or an enclosing scope. Returns NIL if not found.
AtomValue atom_scope_get(AtomScope* scope, AtomString* name);

#endif // ATOM_SCOPE_H
