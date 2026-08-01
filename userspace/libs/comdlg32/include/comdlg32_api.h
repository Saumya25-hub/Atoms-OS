#ifndef BOS_COMDLG32_API_H
#define BOS_COMDLG32_API_H

#include "comdlg32_types.h"

// Subsystem Lifecycle
int32_t COMDLG32_Init(void);
int32_t COMDLG32_Shutdown(void);

// Standard Win32 Common Dialog APIs
BOOL GetOpenFileName(OPENFILENAME* lpofn);
BOOL GetSaveFileName(OPENFILENAME* lpofn);
BOOL ChooseColor(CHOOSECOLOR* lpcc);
BOOL ChooseFont(CHOOSEFONT* lpcf);
BOOL PrintDlg(PRINTDLG* lppd);
BOOL PageSetupDlg(PAGESETUPDLG* lppsd);
HANDLE FindText(FINDREPLACE* lpfr);
HANDLE ReplaceText(FINDREPLACE* lpfr);
LPVOID SHBrowseForFolder(BROWSEINFO* lpbi);
DWORD CommDlgExtendedError(void);

// Helper & Utility APIs
BOOL AddDialogFavorite(const char* path);
BOOL RemoveDialogFavorite(const char* path);
BOOL SetDialogTheme(uint32_t themeId);
DWORD GetDialogHistory(char* buffer, uint32_t max_count);
BOOL ClearDialogHistory(void);
BOOL SetDialogFilter(const char* filter_spec);
BOOL RefreshDialog(uint32_t dialogHandle);
BOOL NavigateDialog(uint32_t dialogHandle, const char* targetPath);
BOOL PreviewFile(const char* filePath, void* previewBuffer);

void comdlg32_run_certification_suite(void);

#endif // BOS_COMDLG32_API_H
