#ifndef APP_CLIPBOARD_H
#define APP_CLIPBOARD_H

#include <stdint.h>
#include <stdbool.h>

void        App_ClipboardCut(const char* src_path);
void        App_ClipboardCopy(const char* src_path);
bool        App_ClipboardPaste(const char* dest_dir);
const char* App_ClipboardGetPath(void);
bool        App_ClipboardIsCut(void);

#endif // APP_CLIPBOARD_H
