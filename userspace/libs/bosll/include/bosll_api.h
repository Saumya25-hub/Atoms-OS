#ifndef BOS_BOSLL_API_H
#define BOS_BOSLL_API_H

#include "bosll_types.h"

// Subsystem Bootstrap
BOS_STATUS BosInitialize(void);
BOS_STATUS BosShutdown(void);

// Process & Thread Runtime
BOS_STATUS BosCreateProcess(const char* imagePath, const char* cmdLine, BOS_PROCESS_INFO* pInfo, BOS_HANDLE* phProcess);
BOS_STATUS BosTerminateProcess(BOS_HANDLE hProcess, uint32_t exitCode);
BOS_HANDLE BosGetCurrentProcess(void);

BOS_STATUS BosCreateThread(BOS_HANDLE hProcess, void* entryPoint, void* param, BOS_THREAD_INFO* tInfo, BOS_HANDLE* phThread);
BOS_STATUS BosExitThread(uint32_t exitCode);
BOS_HANDLE BosGetCurrentThread(void);

// Memory & Virtual Memory Runtime
BOS_STATUS BosAllocateVirtualMemory(BOS_HANDLE hProcess, void** ppBase, size_t size, uint32_t allocType, uint32_t protect);
BOS_STATUS BosFreeVirtualMemory(BOS_HANDLE hProcess, void* pBase, size_t size, uint32_t freeType);
BOS_STATUS BosProtectVirtualMemory(BOS_HANDLE hProcess, void* pBase, size_t size, uint32_t newProtect, uint32_t* pOldProtect);

// Heap Runtime
void*      BosAllocateHeap(size_t size);
bool       BosFreeHeap(void* ptr);

// Handles & Object Manager
BOS_HANDLE BosCreateHandle(void* pObject, uint32_t accessMask);
BOS_STATUS BosCloseHandle(BOS_HANDLE hHandle);
BOS_STATUS BosDuplicateHandle(BOS_HANDLE hSourceProcess, BOS_HANDLE hSourceHandle, BOS_HANDLE hTargetProcess, BOS_HANDLE* phTargetHandle);

// Native Loader
BOS_HANDLE BosLoadLibrary(const char* libraryPath);
void*      BosGetProcedure(BOS_HANDLE hModule, const char* procName);

// Native Synchronization
BOS_HANDLE BosCreateEvent(bool manualReset, bool initialState);
BOS_HANDLE BosCreateMutex(bool initialOwner);
BOS_STATUS BosWaitObject(BOS_HANDLE hObject, uint32_t timeoutMs);

// Exceptions, Syscall, IPC, Timer, Performance & Diagnostics
BOS_STATUS BosRaiseException(uint32_t code, uint32_t flags);
int64_t    BosDispatchSyscall(uint64_t syscallNumber, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
uint64_t   BosQueryPerformance(void);
uint64_t   BosQuerySystemTime(void);
const char* BosGetEnvironment(const char* varName);
BOS_HANDLE BosCreateIPC(const char* channelName, uint32_t bufferSize);

BOS_STATUS BosGetThreadContext(BOS_HANDLE hThread, BOS_CPU_CONTEXT* pCtx);
BOS_STATUS BosSetThreadContext(BOS_HANDLE hThread, const BOS_CPU_CONTEXT* pCtx);

void bosll_run_certification_suite(void);

#endif // BOS_BOSLL_API_H
