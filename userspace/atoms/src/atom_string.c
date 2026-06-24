#include "../include/atom_string.h"
#include "../internal/atom_memory.h"
#include "../internal/atom_hash.h"
#include "../../libbos/include/bos.h"

// Simple String Intern Pool
// In V1, using a basic fixed-size bucket array with linked lists for collisions.
#define POOL_BUCKETS 64

typedef struct StringPoolEntry {
    AtomString* string;
    struct StringPoolEntry* next;
} StringPoolEntry;

static StringPoolEntry* string_pool[POOL_BUCKETS];
static bool pool_initialized = false;

extern uint32_t next_atom_id; // From atom_value.c

static size_t my_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static bool my_streq(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void atom_string_pool_init(void) {
    for (int i = 0; i < POOL_BUCKETS; i++) {
        string_pool[i] = NULL;
    }
    pool_initialized = true;
}

void atom_string_pool_destroy(void) {
    for (int i = 0; i < POOL_BUCKETS; i++) {
        StringPoolEntry* current = string_pool[i];
        while (current) {
            StringPoolEntry* next = current->next;
            // Force free the string memory regardless of ref count during pool destroy
            current->string->magic = ATOM_FREED_MAGIC;
            atom_free(current->string);
            atom_free(current);
            current = next;
        }
        string_pool[i] = NULL;
    }
    pool_initialized = false;
}

static AtomString* find_in_pool(const char* chars, uint32_t length, uint32_t hash) {
    uint32_t index = hash % POOL_BUCKETS;
    StringPoolEntry* current = string_pool[index];
    
    while (current) {
        if (current->string->length == length && 
            current->string->hash == hash &&
            my_streq(current->string->chars, chars, length)) {
            return current->string;
        }
        current = current->next;
    }
    return NULL;
}

static AtomString* allocate_string(const char* chars, uint32_t length, uint32_t hash) {
    AtomString* str = (AtomString*)atom_alloc(sizeof(AtomString) + length + 1);
    str->magic = ATOM_STRING_MAGIC;
    str->hash = hash;
    str->length = length;
    str->ref_count = 1;
    
    for (uint32_t i = 0; i < length; i++) {
        str->chars[i] = chars[i];
    }
    str->chars[length] = '\0';
    
    // Add to pool
    uint32_t index = hash % POOL_BUCKETS;
    StringPoolEntry* entry = (StringPoolEntry*)atom_alloc(sizeof(StringPoolEntry));
    entry->string = str;
    entry->next = string_pool[index];
    string_pool[index] = entry;
    
    return str;
}

AtomValue atom_string_create_len(const char* chars, uint32_t length) {
    if (!pool_initialized) {
        bos_print("[ATOMS] PANIC: String pool not initialized!\n");
        bos_exit();
        while(1) bos_yield();
    }

    uint32_t hash = atom_hash_bytes(chars, length);
    
    AtomString* interned = find_in_pool(chars, length, hash);
    if (interned) {
        interned->ref_count++;
    } else {
        interned = allocate_string(chars, length, hash);
    }
    
    AtomValue v;
    v.type = ATOM_TYPE_STRING;
    v.id = next_atom_id++; // Allocate a unique ID for the object (although pooled strings share the string ptr, the wrapper value gets a unique ID if needed, or we assign the ID to the struct. In V1 we assign it to the AtomValue wrapper).
    v.as.string = interned;
    return v;
}

AtomValue atom_string_create(const char* chars) {
    return atom_string_create_len(chars, (uint32_t)my_strlen(chars));
}

void atom_string_retain(AtomString* str) {
    if (!str) return;
    if (str->magic != ATOM_STRING_MAGIC) {
        bos_print("[ATOMS] PANIC: Bad magic in string_retain!\n");
        bos_exit();
        while(1) bos_yield();
    }
    str->ref_count++;
}

void atom_string_release(AtomString* str) {
    if (!str) return;
    if (str->magic != ATOM_STRING_MAGIC) {
        bos_print("[ATOMS] PANIC: Bad magic in string_release!\n");
        bos_exit();
        while(1) bos_yield();
    }
    
    if (str->ref_count == 0) {
        bos_print("[ATOMS] PANIC: Double free in string_release!\n");
        bos_exit();
        while(1) bos_yield();
    }
    
    str->ref_count--;
    if (str->ref_count == 0) {
        // Remove from pool
        uint32_t index = str->hash % POOL_BUCKETS;
        StringPoolEntry* prev = NULL;
        StringPoolEntry* current = string_pool[index];
        
        while (current) {
            if (current->string == str) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    string_pool[index] = current->next;
                }
                atom_free(current);
                break;
            }
            prev = current;
            current = current->next;
        }
        
        // Free string memory
        str->magic = ATOM_FREED_MAGIC;
        atom_free(str);
    }
}
