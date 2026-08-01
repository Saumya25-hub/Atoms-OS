#include "../include/kernel32_api.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    char name[64];
    uint32_t ref_count;
    bool active;
} KERNEL32AtomEntry;

static KERNEL32AtomEntry g_atom_table[128];

ATOM GlobalAddAtom(LPCSTR lpString) {
    if (!lpString) return 0;
    
    // Check existing
    for (int i = 0; i < 128; i++) {
        if (g_atom_table[i].active && strcmp(g_atom_table[i].name, lpString) == 0) {
            g_atom_table[i].ref_count++;
            return (ATOM)(i + 0xC000);
        }
    }
    // Add new
    for (int i = 0; i < 128; i++) {
        if (!g_atom_table[i].active) {
            strcpy(g_atom_table[i].name, lpString);
            g_atom_table[i].ref_count = 1;
            g_atom_table[i].active = true;
            return (ATOM)(i + 0xC000);
        }
    }
    return 0;
}

ATOM GlobalFindAtom(LPCSTR lpString) {
    if (!lpString) return 0;
    for (int i = 0; i < 128; i++) {
        if (g_atom_table[i].active && strcmp(g_atom_table[i].name, lpString) == 0) {
            return (ATOM)(i + 0xC000);
        }
    }
    return 0;
}

ATOM GlobalDeleteAtom(ATOM nAtom) {
    if (nAtom < 0xC000) return 0;
    int idx = nAtom - 0xC000;
    if (idx >= 0 && idx < 128 && g_atom_table[idx].active) {
        g_atom_table[idx].ref_count--;
        if (g_atom_table[idx].ref_count == 0) {
            g_atom_table[idx].active = false;
        }
    }
    return 0;
}
