#include "../include/atom_bytecode.h"
#include "../internal/atom_memory.h"
#include "../../libbos/include/bos.h"

#define CHUNK_INIT_CAPACITY 8

AtomChunk* atom_chunk_create(void) {
    AtomChunk* chunk = (AtomChunk*)atom_alloc(sizeof(AtomChunk));
    if (!chunk) return NULL;
    
    chunk->count = 0;
    chunk->capacity = CHUNK_INIT_CAPACITY;
    chunk->code = (uint8_t*)atom_alloc(sizeof(uint8_t) * CHUNK_INIT_CAPACITY);
    if (!chunk->code) {
        atom_free(chunk);
        return NULL;
    }
    
    chunk->constants = atom_array_create();
    if (!chunk->constants) {
        atom_free(chunk->code);
        atom_free(chunk);
        return NULL;
    }
    
    return chunk;
}

void atom_chunk_destroy(AtomChunk* chunk) {
    if (!chunk) return;
    
    if (chunk->code) {
        atom_free(chunk->code);
    }
    
    if (chunk->constants) {
        atom_array_destroy(chunk->constants);
    }
    
    atom_free(chunk);
}

void atom_chunk_write(AtomChunk* chunk, uint8_t byte) {
    if (!chunk) return;
    
    if (chunk->count >= chunk->capacity) {
        uint32_t new_capacity = chunk->capacity * 2;
        uint8_t* new_code = (uint8_t*)atom_alloc(sizeof(uint8_t) * new_capacity);
        
        // Copy old code
        for (uint32_t i = 0; i < chunk->count; i++) {
            new_code[i] = chunk->code[i];
        }
        
        atom_free(chunk->code);
        chunk->code = new_code;
        chunk->capacity = new_capacity;
    }
    
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

uint32_t atom_chunk_add_constant(AtomChunk* chunk, AtomValue value) {
    if (!chunk) return 0;
    
    // We add to array, it handles its own resizing.
    // The index is count BEFORE pushing.
    uint32_t index = chunk->constants->count;
    atom_array_push(chunk->constants, value);
    return index;
}
