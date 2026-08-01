#ifndef BOS_SHELL32_TYPES_H
#define BOS_SHELL32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

typedef int32_t INT;
typedef HANDLE  HINSTANCE;

typedef struct {
    DWORD cbSize;
    HANDLE hwnd;
    LPCSTR lpVerb;
    LPCSTR lpFile;
    LPCSTR lpParameters;
    LPCSTR lpDirectory;
    int nShow;
    HINSTANCE hInstApp;
    LPVOID lpIDList;
    LPCSTR lpClass;
    HANDLE hkeyClass;
    DWORD dwHotKey;
    HANDLE hIcon;
    HANDLE hProcess;
} SHELLEXECUTEINFO, *LPSHELLEXECUTEINFO;

typedef struct {
    DWORD cbSize;
    HANDLE hwndOwner;
    DWORD dwFlags;
    LPCSTR lpszTitle;
    LPCSTR lpszMessage;
} NOTIFYICONDATA, *PNOTIFYICONDATA;

#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_SHOW             5

#define CSIDL_DESKTOP                 0x0000
#define CSIDL_PROGRAMS                0x0002
#define CSIDL_PERSONAL                0x0005
#define CSIDL_FAVORITES               0x0006
#define CSIDL_STARTUP                 0x0007
#define CSIDL_RECENT                  0x0008
#define CSIDL_BITBUCKET               0x000a
#define CSIDL_DESKTOPDIRECTORY        0x0010
#define CSIDL_DRIVES                  0x0011
#define CSIDL_NETWORK                 0x0012

#endif // BOS_SHELL32_TYPES_H
