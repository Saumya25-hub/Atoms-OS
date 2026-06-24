#ifndef ATOM_COMPILER_H
#define ATOM_COMPILER_H

#include "atom_bytecode.h"
#include "../../libbos/include/bos.h"

bool atom_compiler_compile(const char* source, AtomChunk* chunk);

#endif // ATOM_COMPILER_H
