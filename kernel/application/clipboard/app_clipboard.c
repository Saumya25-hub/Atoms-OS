#include "app_clipboard.h"
#include "kernel/core/lib/include/string.h"

static ATOMS_Clipboard g_clipboard;

void ATOMS_Clipboard_Init(void) {
    g_clipboard.type = ATOMS_CLIPBOARD_TYPE_NONE;
    g_clipboard.owner_pid = 0;
    g_clipboard.size = 0;
    g_clipboard.text[0] = '\0';
}

bool ATOMS_Clipboard_SetText(uint32_t owner_pid, const char* text) {
    if (!text) return false;

    g_clipboard.type = ATOMS_CLIPBOARD_TYPE_TEXT;
    g_clipboard.owner_pid = owner_pid;

    uint32_t len = 0;
    while (text[len] != '\0' && len < ATOMS_CLIPBOARD_MAX_TEXT - 1) {
        g_clipboard.text[len] = text[len];
        len++;
    }
    g_clipboard.text[len] = '\0';
    g_clipboard.size = len;
    return true;
}

const char* ATOMS_Clipboard_GetText(void) {
    if (g_clipboard.type != ATOMS_CLIPBOARD_TYPE_TEXT) return "";
    return g_clipboard.text;
}

void ATOMS_Clipboard_Clear(void) {
    g_clipboard.type = ATOMS_CLIPBOARD_TYPE_NONE;
    g_clipboard.owner_pid = 0;
    g_clipboard.size = 0;
    g_clipboard.text[0] = '\0';
}

bool ATOMS_Clipboard_HasText(void) {
    return (g_clipboard.type == ATOMS_CLIPBOARD_TYPE_TEXT && g_clipboard.size > 0);
}
