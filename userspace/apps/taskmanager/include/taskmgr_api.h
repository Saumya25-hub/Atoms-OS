#ifndef BOS_TASKMGR_API_H
#define BOS_TASKMGR_API_H

#include "taskmgr_types.h"

// Lifecycle
int32_t  TaskManagerInitialize(void);
void     TaskManagerShutdown(void);

// Refresh APIs
bool     RefreshProcesses(void);
bool     RefreshThreads(void);
bool     RefreshMemory(void);
bool     RefreshCPU(void);
bool     RefreshGPU(void);
bool     RefreshStorage(void);
bool     RefreshNetwork(void);
bool     RefreshDrivers(void);
bool     RefreshServices(void);
extern bool     taskmgr_modules_refresh(void);
bool     RefreshHandles(void);
bool     RefreshHardware(void);
bool     RefreshSensors(void);
bool     RefreshPerformance(void);
bool     RefreshKernelDiagnostics(void);
bool     RefreshPower(void);

// Process Actions
bool     TerminateSelectedProcess(uint32_t pid);
bool     SuspendSelectedProcess(uint32_t pid);
bool     ResumeSelectedProcess(uint32_t pid);
bool     CreateProcessDump(uint32_t pid);

// Export
bool     ExportDiagnostics(const char* path);

// Diagnostics
void     TaskManagerDumpDiagnostics(void);

#endif // BOS_TASKMGR_API_H
