#ifndef BOS_COMCTL32_API_H
#define BOS_COMCTL32_API_H

#include "comctl32_types.h"

// Subsystem Lifecycle
int32_t COMDLG32_Init(void); // Standardized init
int32_t COMCTL32_Init(void);
int32_t COMCTL32_Shutdown(void);

void InitCommonControls(void);
BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX* picce);

// Control Creation Factory APIs
HCONTROL CreateButton(HANDLE hwndParent, const char* text, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateEdit(HANDLE hwndParent, const char* text, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateStatic(HANDLE hwndParent, const char* text, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateListBox(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateComboBox(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateListView(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateTreeView(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateTabControl(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateToolbar(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateStatusBar(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateProgressBar(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateTrackBar(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);
HCONTROL CreateHeader(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style, uint32_t id);

HIMAGELIST CreateImageList(int cx, int cy, uint32_t flags, int cInitial, int cGrow);
int        ImageList_Add(HIMAGELIST himl, HANDLE hbmImage, HANDLE hbmMask);
BOOL       ImageList_Draw(HIMAGELIST himl, int i, HANDLE hdcDst, int x, int y, uint32_t fStyle);

HCONTROL CreateToolTip(HANDLE hwndParent, uint32_t style);
HCONTROL CreateReBar(HANDLE hwndParent, uint32_t style);
HCONTROL CreateUpDownControl(HANDLE hwndParent, uint32_t style, HANDLE hwndBuddy);
HCONTROL CreatePager(HANDLE hwndParent, uint32_t style);
HCONTROL CreateAnimateControl(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style);
HCONTROL CreateMonthCalendar(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style);
HCONTROL CreateDateTimePicker(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style);
HCONTROL CreateHotKeyControl(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style);
HCONTROL CreateIPAddressControl(HANDLE hwndParent, int x, int y, int w, int h, uint32_t style);

BOOL SetControlTheme(HCONTROL hCtrl, uint32_t themeId);
BOOL RefreshControl(HCONTROL hCtrl);
BOOL InvalidateControl(HCONTROL hCtrl);
BOOL UpdateControl(HCONTROL hCtrl);
BOOL DestroyControl(HCONTROL hCtrl);

void comctl32_run_certification_suite(void);

#endif // BOS_COMCTL32_API_H
