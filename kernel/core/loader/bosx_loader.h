#ifndef BOSX_LOADER_H
#define BOSX_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "bosx_format.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOSX_MAX_PROCESSES 32

typedef int32_t bosx_error_t;

#define BOSX_SUCCESS                    0
#define BOSX_ERR_FILE_NOT_FOUND        -1
#define BOSX_ERR_BAD_MAGIC             -2
#define BOSX_ERR_BAD_HEADER            -3
#define BOSX_ERR_UNSUPPORTED_ARCH      -4
#define BOSX_ERR_MEMORY_ALLOC          -5
#define BOSX_ERR_RELOCATION_FAILED     -6
#define BOSX_ERR_UNRESOLVED_IMPORT     -7
#define BOSX_ERR_SECURITY_VIOLATION    -8
#define BOSX_ERR_PROCESS_LIMIT         -9

typedef enum {
    BOSX_PROC_CLOSED = 0,
    BOSX_PROC_INIT,
    BOSX_PROC_RUNNING,
    BOSX_PROC_SUSPENDED,
    BOSX_PROC_TERMINATED
} BOSX_ProcessState;

typedef struct {
    uint32_t            pid;
    char                name[64];
    char                filepath[128];
    BOSX_ProcessState   state;
    uint32_t            memory_used;
    uint32_t            window_count;
    uint64_t            entry_point;
    uint64_t            image_base;
    uint32_t            capabilities;
} BOSX_Process;

// Core Loader 9-Stage Pipeline APIs
void         BOSX_Init(void);
bosx_error_t BOSX_ValidateHeader(const BOSX_Header* header);
bosx_error_t BOSX_Load(const char* filepath);
bosx_error_t BOSX_LoadExecutableBuffer(const uint8_t* buffer, uint32_t size, uint32_t* out_pid);
void         bosx_loader_open(const char* filepath);
void         bosx_cleanup_process(uint32_t pid);
BOSX_Process* BOSX_GetProcessByPID(uint32_t pid);
uint32_t     BOSX_GetProcessCount(void);
void         BOSX_ProcessMonitor_Display(void);

#ifdef __cplusplus
}
#endif

#endif // BOSX_LOADER_H
