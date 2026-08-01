#ifndef BOS_USER32_TYPES_H
#define BOS_USER32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t HWND;
typedef uint32_t HMENU;
typedef uint32_t HCURSOR;
typedef uint32_t HICON;
typedef uint32_t HINSTANCE;
typedef uint32_t HBRUSH;
typedef uint32_t HDC;
typedef uint32_t HACCEL;
typedef uint32_t WPARAM;
typedef uint64_t LPARAM;
typedef uint64_t LRESULT;

typedef LRESULT (*WNDPROC)(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} RECT, *PRECT, *LPRECT;

typedef struct {
    int32_t x;
    int32_t y;
} POINT, *PPOINT, *LPPOINT;

typedef struct {
    HWND     hwnd;
    uint32_t message;
    WPARAM   wParam;
    LPARAM   lParam;
    uint32_t time;
    POINT    pt;
} MSG, *PMSG, *LPMSG;

typedef struct {
    uint32_t style;
    WNDPROC  lpfnWndProc;
    int32_t  cbClsExtra;
    int32_t  cbWndExtra;
    HINSTANCE hInstance;
    HICON    hIcon;
    HCURSOR  hCursor;
    HBRUSH   hbrBackground;
    const char* lpszMenuName;
    const char* lpszClassName;
} WNDCLASS, *PWNDCLASS, *LPWNDCLASS;

typedef struct {
    uint32_t cbSize;
    uint32_t style;
    WNDPROC  lpfnWndProc;
    int32_t  cbClsExtra;
    int32_t  cbWndExtra;
    HINSTANCE hInstance;
    HICON    hIcon;
    HCURSOR  hCursor;
    HBRUSH   hbrBackground;
    const char* lpszMenuName;
    const char* lpszClassName;
    HICON    hIconSm;
} WNDCLASSEX, *PWNDCLASSEX, *LPWNDCLASSEX;

typedef struct {
    HDC      hdc;
    bool     fErase;
    RECT     rcPaint;
    bool     fRestore;
    bool     fIncUpdate;
    uint8_t  rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *LPAINTSTRUCT;

typedef struct {
    void*    lpCreateParams;
    HINSTANCE hInstance;
    HMENU    hMenu;
    HWND     hwndParent;
    int32_t  cy;
    int32_t  cx;
    int32_t  y;
    int32_t  x;
    uint32_t style;
    const char* lpszName;
    const char* lpszClass;
    uint32_t dwExStyle;
} CREATESTRUCT, *LPCREATESTRUCT;

typedef struct {
    uint8_t  fVirt;
    uint16_t key;
    uint16_t cmd;
} ACCEL, *LPACCEL;

#define WM_NULL            0x0000
#define WM_CREATE          0x0001
#define WM_DESTROY         0x0002
#define WM_MOVE            0x0003
#define WM_SIZE            0x0005
#define WM_ACTIVATE        0x0006
#define WM_SETFOCUS        0x0007
#define WM_KILLFOCUS       0x0008
#define WM_PAINT           0x000F
#define WM_CLOSE           0x0010
#define WM_QUIT            0x0012
#define WM_ERASEBKGND      0x0014
#define WM_SHOWWINDOW      0x0018
#define WM_KEYDOWN         0x0100
#define WM_KEYUP           0x0101
#define WM_CHAR            0x0102
#define WM_COMMAND         0x0111
#define WM_TIMER           0x0113
#define WM_MOUSEMOVE       0x0200
#define WM_LBUTTONDOWN     0x0201
#define WM_LBUTTONUP       0x0202
#define WM_RBUTTONDOWN     0x0204
#define WM_RBUTTONUP       0x0205

#define MB_OK              0x00000000L
#define MB_OKCANCEL        0x00000001L
#define MB_YESNO           0x00000004L
#define MB_ICONINFORMATION 0x00000040L

#endif // BOS_USER32_TYPES_H
