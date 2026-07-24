#ifndef ATOMS_APP_CLIPBOARD_H
#define ATOMS_APP_CLIPBOARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS OS Global Clipboard Subsystem
// ============================================================

#define ATOMS_CLIPBOARD_MAX_TEXT 2048

typedef enum {
    ATOMS_CLIPBOARD_TYPE_NONE = 0,
    ATOMS_CLIPBOARD_TYPE_TEXT = 1,
    ATOMS_CLIPBOARD_TYPE_RAW_DATA = 2
} ATOMS_ClipboardType;

typedef struct {
    ATOMS_ClipboardType type;
    uint32_t            owner_pid;
    uint32_t            size;
    char                text[ATOMS_CLIPBOARD_MAX_TEXT];
} ATOMS_Clipboard;

void ATOMS_Clipboard_Init(void);
bool ATOMS_Clipboard_SetText(uint32_t owner_pid, const char* text);
const char* ATOMS_Clipboard_GetText(void);
void ATOMS_Clipboard_Clear(void);
bool ATOMS_Clipboard_HasText(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_CLIPBOARD_H
