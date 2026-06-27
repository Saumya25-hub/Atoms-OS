#ifndef BOS_FILE_ASSOC_H
#define BOS_FILE_ASSOC_H

#include <stdint.h>

#define BOS_MAX_FILE_ASSOCS 16
#define BOS_EXT_LEN 8

// Callback: receives the full path of the file to open
typedef void (*BOS_FileOpenFunc)(const char* filepath);

typedef struct {
    char extension[BOS_EXT_LEN]; // e.g. "TXT", "ELF", "BMP"
    char app_name[32];           // e.g. "Text Viewer"
    BOS_FileOpenFunc open_func;
} BOS_FileAssocEntry;

void BOS_FileAssoc_Init(void);
int  BOS_RegisterFileAssociation(const char* extension, const char* app_name, BOS_FileOpenFunc open_func);
BOS_FileOpenFunc BOS_GetAssociatedApp(const char* filename);
int  BOS_OpenFile(const char* filepath);

#endif // BOS_FILE_ASSOC_H
