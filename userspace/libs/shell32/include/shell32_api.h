#ifndef BOS_SHELL32_API_H
#define BOS_SHELL32_API_H

#include "shell32_types.h"

// Subsystem Lifecycle
int32_t ShellInitialize(void);
int32_t ShellShutdown(void);

// Execution & Launching APIs
HINSTANCE ShellExecute(HANDLE hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
BOOL      ShellExecuteEx(SHELLEXECUTEINFO* pExecInfo);

// Explorer & Navigation APIs
BOOL ShellOpenFolder(LPCSTR lpFolderPath);
BOOL ShellOpenFile(LPCSTR lpFilePath);

// Shortcuts APIs (.slink)
BOOL ShellCreateShortcut(LPCSTR lpShortcutPath, LPCSTR lpTargetPath, LPCSTR lpArguments, LPCSTR lpIconPath);
BOOL ShellResolveShortcut(LPCSTR lpShortcutPath, char* targetBuffer, uint32_t bufferSize);

// Recycle Bin APIs
BOOL ShellDelete(LPCSTR lpPath, BOOL bPermanent);
BOOL ShellRestore(LPCSTR lpPath);
BOOL ShellEmptyRecycleBin(HANDLE hwnd, LPCSTR pszRootPath, DWORD dwFlags);

// Special Folder APIs
BOOL ShellGetKnownFolder(int nFolder, char* pathBuffer, uint32_t bufferSize);

// Properties & Context Menu APIs
BOOL ShellShowProperties(LPCSTR lpPath);
BOOL ShellShowContextMenu(HANDLE hwndOwner, LPCSTR lpPath, int x, int y);

// Icons & ImageList APIs
HANDLE ShellGetIcon(LPCSTR lpPath, uint32_t iconFlags);
HANDLE ShellGetImageList(int iImageListType);

// File Associations APIs
BOOL ShellRegisterFileAssociation(LPCSTR lpExtension, LPCSTR lpAppPath);
BOOL ShellGetFileAssociation(LPCSTR lpExtension, char* appPathBuffer, uint32_t bufferSize);

// Notifications & Taskbar APIs
BOOL ShellShowNotification(LPCSTR title, LPCSTR message, uint32_t iconType);
BOOL ShellSearch(LPCSTR query, char* resultBuffer, uint32_t bufferSize);
BOOL ShellRefresh(void);
BOOL ShellRestartExplorer(void);
BOOL ShellPinTaskbar(LPCSTR lpAppPath);
BOOL ShellUnpinTaskbar(LPCSTR lpAppPath);

void shell32_run_certification_suite(void);

#endif // BOS_SHELL32_API_H
