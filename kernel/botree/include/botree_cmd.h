#ifndef BOTREE_CMD_H
#define BOTREE_CMD_H

#include "botree_types.h"

// Command Output Callback Sink
typedef void (*BDeCommandSink)(const char* text, void* ctx);

// Command Engine Public Functions
int32_t BDe_ExecuteCommand(const char* command_line, BDeCommandSink sink, void* ctx);

#endif // BOTREE_CMD_H
