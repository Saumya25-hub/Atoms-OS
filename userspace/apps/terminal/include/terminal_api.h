#ifndef BOS_TERMINAL_API_H
#define BOS_TERMINAL_API_H

#include "terminal_types.h"

// Lifecycle
int32_t  TerminalInitialize(void);
void     TerminalShutdown(void);

// I/O
bool     TerminalPrint(const char* text);
bool     TerminalReadLine(char* buffer, uint32_t length);
bool     TerminalClearScreen(void);

// Command execution
bool     TerminalExecute(const char* command);
bool     TerminalRegisterCommand(const char* name, TERMINAL_COMMAND_HANDLER handler);

// Aliases
bool     TerminalRegisterAlias(const char* alias, const char* target);

// Plugins
bool     TerminalLoadPlugin(const char* pluginPath);

// Scripts
bool     TerminalExecuteScript(const char* scriptPath);

// Diagnostics
void     TerminalDumpDiagnostics(void);

#endif // BOS_TERMINAL_API_H
