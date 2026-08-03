#ifndef SHELL_CLIPBOARD_H
#define SHELL_CLIPBOARD_H

#include <stdint.h>
#include <stdbool.h>

void        Shell_ClipboardCut(const char* src_path);
void        Shell_ClipboardCopy(const char* src_path);
bool        Shell_ClipboardPaste(const char* dest_dir);
const char* Shell_ClipboardGetPath(void);
bool        Shell_ClipboardIsCut(void);

#endif // SHELL_CLIPBOARD_H
