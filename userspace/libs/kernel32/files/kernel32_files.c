#include "../include/kernel32_api.h"

extern int vfs_open(const char* path, int flags);
extern int vfs_read(int fd, void* buf, size_t count);
extern int vfs_write(int fd, const void* buf, size_t count);
extern int vfs_delete(const char* path);

HANDLE CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    (void)dwDesiredAccess; (void)dwShareMode; (void)lpSecurityAttributes; (void)dwCreationDisposition;
    (void)dwFlagsAndAttributes; (void)hTemplateFile;
    if (!lpFileName) return INVALID_HANDLE_VALUE;
    
    int fd = vfs_open(lpFileName, 0);
    if (fd < 0) return INVALID_HANDLE_VALUE;
    return (HANDLE)(fd + 10);
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD* lpNumberOfBytesRead, void* lpOverlapped) {
    (void)lpOverlapped;
    if (hFile == INVALID_HANDLE_VALUE || !lpBuffer) return false;
    int fd = (int)hFile - 10;
    int read_bytes = vfs_read(fd, lpBuffer, nNumberOfBytesToRead);
    if (read_bytes < 0) return false;
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = (DWORD)read_bytes;
    return true;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, DWORD* lpNumberOfBytesWritten, void* lpOverlapped) {
    (void)lpOverlapped;
    if (hFile == INVALID_HANDLE_VALUE || !lpBuffer) return false;
    int fd = (int)hFile - 10;
    int wrote_bytes = vfs_write(fd, lpBuffer, nNumberOfBytesToWrite);
    if (wrote_bytes < 0) return false;
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = (DWORD)wrote_bytes;
    return true;
}

BOOL FlushFileBuffers(HANDLE hFile) {
    return (hFile != INVALID_HANDLE_VALUE);
}

BOOL DeleteFile(LPCSTR lpFileName) {
    if (!lpFileName) return false;
    return (vfs_delete(lpFileName) == 0);
}

BOOL MoveFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName) {
    (void)lpExistingFileName; (void)lpNewFileName;
    return true;
}

BOOL CopyFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists) {
    (void)lpExistingFileName; (void)lpNewFileName; (void)bFailIfExists;
    return true;
}

BOOL LockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh) {
    (void)hFile; (void)dwFileOffsetLow; (void)dwFileOffsetHigh; (void)nNumberOfBytesToLockLow; (void)nNumberOfBytesToLockHigh;
    return true;
}

BOOL UnlockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh) {
    (void)hFile; (void)dwFileOffsetLow; (void)dwFileOffsetHigh; (void)nNumberOfBytesToUnlockLow; (void)nNumberOfBytesToUnlockHigh;
    return true;
}
