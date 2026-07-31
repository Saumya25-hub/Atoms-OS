#ifndef STUDIO_EDITOR_H
#define STUDIO_EDITOR_H

#include "platform/include/bos_types.h"

void BOS_Studio_CodeEditorInit(void);
void BOS_Studio_LoadFile(const char* filepath);
void BOS_Studio_InsertEventHandler(const char* control_name, const char* event_name);

#endif /* STUDIO_EDITOR_H */
