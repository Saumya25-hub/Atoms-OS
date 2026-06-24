#ifndef ATOMS_H
#define ATOMS_H

// The single include file for the entire Atoms Library

#include "atom_types.h"
#include "atom_value.h"
#include "atom_string.h"

// Phase 2 will add:
// #include "atom_array.h"
// #include "atom_table.h"

// Phase 3 will add:
// #include "atom_debug.h"

// Initialization
void atoms_init(void);
void atoms_shutdown(void);

// Testing
void atom_self_test(void);

#endif // ATOMS_H
