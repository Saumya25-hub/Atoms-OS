#ifndef BOS_USER32_API_H
#define BOS_USER32_API_H

#include "user32_types.h"

int32_t USER32_Init(void);
int32_t USER32_Shutdown(void);

uint16_t RegisterClass(const WNDCLASS* lpWndClass);
uint16_t RegisterClassEx(const WNDCLASSEX* lpwcx);

HWND CreateWindow(const char* lpClassName, const char* lpWindowName, uint32_t dwStyle, int32_t x, int32_t y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, void* lpParam);
HWND CreateWindowEx(uint32_t dwExStyle, const char* lpClassName, const char* lpWindowName, uint32_t dwStyle, int32_t x, int32_t y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, void* lpParam);

bool DestroyWindow(HWND hWnd);
bool ShowWindow(HWND hWnd, int32_t nCmdShow);
bool HideWindow(HWND hWnd);
bool MoveWindow(HWND hWnd, int32_t x, int32_t y, int32_t nWidth, int32_t nHeight, bool bRepaint);
bool SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int32_t x, int32_t y, int32_t cx, int32_t cy, uint32_t uFlags);

bool UpdateWindow(HWND hWnd);
bool InvalidateRect(HWND hWnd, const RECT* lpRect, bool bErase);
bool ValidateRect(HWND hWnd, const RECT* lpRect);
HDC  BeginPaint(HWND hWnd, PAINTSTRUCT* lpPaint);
bool EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);

bool GetMessage(MSG* lpMsg, HWND hWnd, uint32_t wMsgFilterMin, uint32_t wMsgFilterMax);
bool PeekMessage(MSG* lpMsg, HWND hWnd, uint32_t wMsgFilterMin, uint32_t wMsgFilterMax, uint32_t wRemoveMsg);
bool TranslateMessage(const MSG* lpMsg);
LRESULT DispatchMessage(const MSG* lpMsg);
LRESULT DefWindowProc(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam);

LRESULT SendMessage(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam);
bool    PostMessage(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam);
void    PostQuitMessage(int32_t nExitCode);

HWND SetFocus(HWND hWnd);
HWND GetFocus(void);
HWND SetCapture(HWND hWnd);
bool ReleaseCapture(void);

HCURSOR SetCursor(HCURSOR hCursor);
HCURSOR LoadCursor(HINSTANCE hInstance, const char* lpCursorName);
HICON   LoadIcon(HINSTANCE hInstance, const char* lpIconName);

HMENU CreateMenu(void);
bool  AppendMenu(HMENU hMenu, uint32_t uFlags, uint32_t uIDNewItem, const char* lpNewItem);
bool  TrackPopupMenu(HMENU hMenu, uint32_t uFlags, int32_t x, int32_t y, int32_t nReserved, HWND hWnd, const RECT* prcRect);

HWND    CreateDialog(HINSTANCE hInstance, const char* lpTemplateName, HWND hWndParent, WNDPROC lpDialogFunc);
int32_t DialogBox(HINSTANCE hInstance, const char* lpTemplateName, HWND hWndParent, WNDPROC lpDialogFunc);
bool    EndDialog(HWND hDlg, int32_t nResult);

bool  OpenClipboard(HWND hWndNewOwner);
bool  CloseClipboard(void);
bool  EmptyClipboard(void);
void* SetClipboardData(uint32_t uFormat, void* hMem);
void* GetClipboardData(uint32_t uFormat);

uint32_t SetTimer(HWND hWnd, uint32_t nIDEvent, uint32_t uElapse, void* lpTimerFunc);
bool     KillTimer(HWND hWnd, uint32_t uIDEvent);

bool RegisterHotKey(HWND hWnd, int32_t id, uint32_t fsModifiers, uint32_t vk);
bool UnregisterHotKey(HWND hWnd, int32_t id);

int32_t MessageBox(HWND hWnd, const char* lpText, const char* lpCaption, uint32_t uType);

void user32_run_certification_suite(void);

#endif // BOS_USER32_API_H
