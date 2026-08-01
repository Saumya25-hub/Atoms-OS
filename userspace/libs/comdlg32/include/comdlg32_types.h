#ifndef BOS_COMDLG32_TYPES_H
#define BOS_COMDLG32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

typedef int32_t  INT;
typedef uint32_t UINT;
typedef uintptr_t LPARAM;
typedef uint32_t COLORREF;

typedef struct {
    int32_t x;
    int32_t y;
} POINT;

typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} RECT;

#define OFN_OVERWRITEPROMPT 0x00000002
#define OFN_HIDEREADONLY    0x00000004
#define OFN_NOCHANGEDIR     0x00000008
#define OFN_ALLOWMULTISELECT 0x00000200
#define OFN_PATHMUSTEXIST   0x00000800
#define OFN_FILEMUSTEXIST   0x00001000
#define OFN_CREATEPROMPT    0x00002000

#define CC_RGBINIT          0x00000001
#define CC_FULLOPEN         0x00000002

#define CF_SCREENFONTS      0x00000001
#define CF_PRINTERFONTS     0x00000002
#define CF_EFFECTS          0x00000100

#define PD_ALLPAGES         0x00000000
#define PD_SELECTION        0x00000001
#define PD_PAGENUMS         0x00000002
#define PD_RETURNDC         0x00000100

typedef struct {
    DWORD lStructSize;
    HANDLE hwndOwner;
    HANDLE hInstance;
    LPCSTR lpstrFilter;
    LPSTR lpstrCustomFilter;
    DWORD nMaxCustFilter;
    DWORD nFilterIndex;
    LPSTR lpstrFile;
    DWORD nMaxFile;
    LPSTR lpstrFileTitle;
    DWORD nMaxFileTitle;
    LPCSTR lpstrInitialDir;
    LPCSTR lpstrTitle;
    DWORD Flags;
    WORD nFileOffset;
    WORD nFileExtension;
    LPCSTR lpstrDefExt;
    LPVOID lCustData;
    LPVOID lpfnHook;
    LPCSTR lpTemplateName;
} OPENFILENAME, *LPOPENFILENAME;

typedef struct {
    DWORD lStructSize;
    HANDLE hwndOwner;
    HANDLE hInstance;
    COLORREF rgbResult;
    COLORREF* lpCustColors;
    DWORD Flags;
    LPVOID lCustData;
    LPVOID lpfnHook;
    LPCSTR lpTemplateName;
} CHOOSECOLOR, *LPCHOOSECOLOR;

typedef struct {
    DWORD lStructSize;
    HANDLE hwndOwner;
    HANDLE hDC;
    LPVOID lpLogFont;
    INT iPointSize;
    DWORD Flags;
    COLORREF rgbColors;
    LPVOID lCustData;
    LPVOID lpfnHook;
    LPCSTR lpTemplateName;
    HANDLE hInstance;
    LPSTR lpszStyle;
    WORD nFontType;
    WORD ___MISSING_ALIGNMENT;
    INT nSizeMin;
    INT nSizeMax;
} CHOOSEFONT, *LPCHOOSEFONT;

typedef struct {
    DWORD lStructSize;
    HANDLE hwndOwner;
    HANDLE hDevMode;
    HANDLE hDevNames;
    HANDLE hDC;
    DWORD Flags;
    WORD nFromPage;
    WORD nToPage;
    WORD nMinPage;
    WORD nMaxPage;
    WORD nCopies;
    HANDLE hInstance;
    LPVOID lCustData;
    LPVOID lpfnHook;
    LPCSTR lpPrintTemplateName;
    LPCSTR lpSetupTemplateName;
    HANDLE hPrintTemplate;
    HANDLE hSetupTemplate;
} PRINTDLG, *LPPRINTDLG;

typedef struct {
    DWORD lStructSize;
    HANDLE hwndOwner;
    HANDLE hDevMode;
    HANDLE hDevNames;
    DWORD Flags;
    POINT ptPaperSize;
    RECT rtMinMargin;
    RECT rtMargin;
    HANDLE hInstance;
    LPVOID lCustData;
    LPVOID lpfnHook;
    LPCSTR lpPageSetupTemplateName;
    HANDLE hPageSetupTemplate;
} PAGESETUPDLG, *LPPAGESETUPDLG;

typedef struct {
    DWORD lStructSize;
    HANDLE hwndOwner;
    HANDLE hInstance;
    DWORD Flags;
    LPSTR lpstrFindWhat;
    LPSTR lpstrReplaceWith;
    WORD wFindWhatLen;
    WORD wReplaceWithLen;
    LPVOID lCustData;
    LPVOID lpfnHook;
    LPCSTR lpTemplateName;
} FINDREPLACE, *LPFINDREPLACE;

typedef struct {
    HANDLE hwndOwner;
    LPVOID pidlRoot;
    LPSTR pszDisplayName;
    LPCSTR lpszTitle;
    UINT ulFlags;
    LPVOID lpfn;
    LPARAM lParam;
    INT iImage;
} BROWSEINFO, *LPBROWSEINFO;

#endif // BOS_COMDLG32_TYPES_H
