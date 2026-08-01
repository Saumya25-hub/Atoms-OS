#ifndef BOS_EXPLORER_API_H
#define BOS_EXPLORER_API_H

#include "explorer_types.h"

// Core Session & Lifecycle APIs
int32_t ExplorerInitialize(void);
bool    ExplorerStartSession(void);
void    ExplorerShutdown(void);
void    ExplorerRestart(void);
void    ExplorerRestartShell(void);
void    ExplorerLogout(void);
void    ExplorerLockDesktop(void);

// Desktop & Window Launchers
void    ExplorerOpenFolder(const char* path);
void    ExplorerOpenDesktop(void);
void    ExplorerOpenRecycleBin(void);
void    ExplorerOpenControlPanel(void);
void    ExplorerOpenSettings(void);
void    ExplorerRunDialog(void);
void    ExplorerSearch(const char* query);

// UI Refresh & Customization
void    ExplorerRefreshDesktop(void);
void    ExplorerRefreshIcons(void);
void    ExplorerPinTaskbar(HANDLE hwnd);
void    ExplorerUnpinTaskbar(HANDLE hwnd);
void    ExplorerShowNotification(const char* title, const char* message);

// Diagnostics
void    ExplorerDumpDiagnostics(void);

#endif // BOS_EXPLORER_API_H
