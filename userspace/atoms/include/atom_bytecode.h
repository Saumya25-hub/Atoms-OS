#ifndef ATOM_BYTECODE_H
#define ATOM_BYTECODE_H

#include "atom_types.h"
#include "atom_array.h"

// Opcode instructions for the BOSL VM
typedef enum {
    OP_CONSTANT,      // Load constant from chunk. Argument: index (1 byte)
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_GLOBAL,    // Get global var. Argument: string index (1 byte)
    OP_DEFINE_GLOBAL, // Define global var. Argument: string index (1 byte)
    OP_SET_GLOBAL,    // Set global var. Argument: string index (1 byte)
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_AND,
    OP_OR,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_METHOD_CALL,
    OP_BUILD_ARRAY,
    OP_BUILD_TABLE,
    OP_INDEX_GET,
    OP_INDEX_SET,
    OP_PRINT,
    OP_RETURN,
} OpCode;

// A sequence of bytecode
typedef struct {
    uint32_t count;
    uint32_t capacity;
    uint8_t* code;
    AtomArray* constants; // An array of constants (numbers, strings) used by this chunk
} AtomChunk;

// Core chunk operations
AtomChunk* atom_chunk_create(void);
void atom_chunk_destroy(AtomChunk* chunk);

// Write a byte (opcode or operand) to the chunk
void atom_chunk_write(AtomChunk* chunk, uint8_t byte);

// Add a constant to the chunk's constant array, returning its index
uint32_t atom_chunk_add_constant(AtomChunk* chunk, AtomValue value);

#endif // ATOM_BYTECODE_H
