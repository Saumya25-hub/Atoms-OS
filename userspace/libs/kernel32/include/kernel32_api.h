#ifndef BOS_KERNEL32_API_H
#define BOS_KERNEL32_API_H

#include "kernel32_types.h"

// Subsystem Lifecycle
int32_t KERNEL32_Init(void);
int32_t KERNEL32_Shutdown(void);

// Process Operations
BOOL   CreateProcess(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
void   ExitProcess(DWORD uExitCode);
BOOL   TerminateProcess(HANDLE hProcess, DWORD uExitCode);
HANDLE GetCurrentProcess(void);
BOOL   DuplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, HANDLE* lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions);
HANDLE OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);

// Thread Operations
HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, size_t dwStackSize, void* lpStartAddress, void* lpParameter, DWORD dwCreationFlags, DWORD* lpThreadId);
void   ExitThread(DWORD dwExitCode);
DWORD  SuspendThread(HANDLE hThread);
DWORD  ResumeThread(HANDLE hThread);
void   Sleep(DWORD dwMilliseconds);
void   YieldProcessor(void);

// Virtual Memory & Heap
LPVOID VirtualAlloc(LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL   VirtualFree(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType);
BOOL   VirtualProtect(LPVOID lpAddress, size_t dwSize, DWORD flNewProtect, DWORD* lpflOldProtect);

HANDLE HeapCreate(DWORD flOptions, size_t dwInitialSize, size_t dwMaximumSize);
LPVOID HeapAlloc(HANDLE hHeap, DWORD dwFlags, size_t dwBytes);
LPVOID HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, size_t dwBytes);
BOOL   HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
BOOL   HeapDestroy(HANDLE hHeap);

// Synchronization
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
BOOL   ReleaseMutex(HANDLE hMutex);
HANDLE CreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, int32_t lInitialCount, int32_t lMaximumCount, LPCSTR lpName);
BOOL   ReleaseSemaphore(HANDLE hSemaphore, int32_t lReleaseCount, int32_t* lpPreviousCount);

void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

// Events
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
BOOL   SetEvent(HANDLE hEvent);
BOOL   ResetEvent(HANDLE hEvent);
BOOL   PulseEvent(HANDLE hEvent);
DWORD  WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD  WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);

// File I/O
HANDLE CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL   ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD* lpNumberOfBytesRead, void* lpOverlapped);
BOOL   WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, DWORD* lpNumberOfBytesWritten, void* lpOverlapped);
BOOL   FlushFileBuffers(HANDLE hFile);
BOOL   DeleteFile(LPCSTR lpFileName);
BOOL   MoveFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
BOOL   CopyFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
BOOL   LockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh);
BOOL   UnlockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh);

// Directories
BOOL   CreateDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL   RemoveDirectory(LPCSTR lpPathName);
HANDLE FindFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
BOOL   FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
DWORD  GetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer);
BOOL   SetCurrentDirectory(LPCSTR lpPathName);

// Pipes
BOOL CreatePipe(HANDLE* hReadPipe, HANDLE* hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize);

// Console
HANDLE CreateConsole(void);
BOOL   WriteConsole(HANDLE hConsoleOutput, LPCVOID lpBuffer, DWORD nNumberOfCharsToWrite, DWORD* lpNumberOfCharsWritten, LPVOID lpReserved);
BOOL   ReadConsole(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, DWORD* lpNumberOfCharsRead, LPVOID pInputControl);
BOOL   SetConsoleTitle(LPCSTR lpConsoleTitle);

// Time & Timers
DWORD GetTickCount(void);
BOOL  QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
BOOL  QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

// Environment
BOOL  SetEnvironmentVariable(LPCSTR lpName, LPCSTR lpValue);
DWORD GetEnvironmentVariable(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
LPSTR GetCommandLine(void);

// Dynamic Loader
HMODULE LoadLibrary(LPCSTR lpLibFileName);
BOOL    FreeLibrary(HMODULE hLibModule);
FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

// Thread Local Storage
DWORD  TlsAlloc(void);
LPVOID TlsGetValue(DWORD dwTlsIndex);
BOOL   TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);
BOOL   TlsFree(DWORD dwTlsIndex);

// Atoms
ATOM GlobalAddAtom(LPCSTR lpString);
ATOM GlobalFindAtom(LPCSTR lpString);
ATOM GlobalDeleteAtom(ATOM nAtom);

void kernel32_run_certification_suite(void);

#endif // BOS_KERNEL32_API_H
