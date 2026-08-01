#ifndef BOS_KERNEL32_TYPES_H
#define BOS_KERNEL32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t HANDLE;
typedef uint32_t DWORD;
typedef int32_t  BOOL;
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint64_t DWORD64;
typedef void*    LPVOID;
typedef const void* LPCVOID;
typedef char*    LPSTR;
typedef const char* LPCSTR;
typedef uint16_t ATOM;
typedef uint32_t HMODULE;
typedef void (*FARPROC)(void);

#define INVALID_HANDLE_VALUE ((HANDLE)(uint32_t)-1)

#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000

#define PAGE_NOACCESS   0x01
#define PAGE_READONLY   0x02
#define PAGE_READWRITE  0x04
#define PAGE_EXECUTE    0x10

#define GENERIC_READ    0x80000000
#define GENERIC_WRITE   0x40000000
#define GENERIC_EXECUTE 0x20000000

#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5

#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010

#define INFINITE        0xFFFFFFFF

typedef struct {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    BYTE* lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFO, *LPSTARTUPINFO;

typedef struct {
    DWORD dwFileAttributes;
    uint64_t ftCreationTime;
    uint64_t ftLastAccessTime;
    uint64_t ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    char cFileName[260];
    char cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct {
    DWORD LockCount;
    DWORD RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD SpinCount;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef union {
    struct {
        DWORD LowPart;
        int32_t HighPart;
    } DUMMYSTRUCTNAME;
    int64_t QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#endif // BOS_KERNEL32_TYPES_H
