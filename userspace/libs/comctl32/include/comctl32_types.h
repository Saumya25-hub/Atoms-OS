#ifndef BOS_COMCTL32_TYPES_H
#define BOS_COMCTL32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

typedef uintptr_t LPARAM;
typedef uint32_t  HIMAGELIST;
typedef uint32_t  HCONTROL;
typedef uint32_t  COLORREF;

#define ICC_LISTVIEW_CLASSES   0x00000001
#define ICC_TREEVIEW_CLASSES   0x00000002
#define ICC_BAR_CLASSES        0x00000004
#define ICC_TAB_CLASSES        0x00000008
#define ICC_UPDOWN_CLASS       0x00000010
#define ICC_PROGRESS_CLASS     0x00000020

typedef struct {
    DWORD dwSize;
    DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;

typedef struct {
    HANDLE hwndFrom;
    uintptr_t idFrom;
    uint32_t code;
} NMHDR, *LPNMHDR;

typedef struct {
    uint32_t mask;
    int iImage;
    int iParam;
    char* pszText;
    int cchTextMax;
    int iSubItem;
} LVITEM, *LPLVITEM;

typedef struct {
    uint32_t mask;
    int fmt;
    int cx;
    char* pszText;
    int cchTextMax;
    int iSubItem;
} LVCOLUMN, *LPLVCOLUMN;

typedef struct {
    uint32_t mask;
    HANDLE hItem;
    uint32_t state;
    uint32_t stateMask;
    char* pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
} TVITEM, *LPTVITEM;

typedef struct {
    HANDLE hParent;
    HANDLE hInsertAfter;
    TVITEM item;
} TVINSERTSTRUCT, *LPTVINSERTSTRUCT;

#endif // BOS_COMCTL32_TYPES_H
