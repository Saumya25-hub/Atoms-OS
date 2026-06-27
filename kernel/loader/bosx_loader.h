#ifndef BOSX_LOADER_H
#define BOSX_LOADER_H

#include <stdint.h>
#include <stdbool.h>

#define BOSX_MAX_PROCESSES 16

typedef enum {
    BOSX_PROC_CLOSED = 0,
    BOSX_PROC_RUNNING,
    BOSX_PROC_TERMINATED
} BOSX_ProcessState;

typedef struct {
    uint32_t pid;
    char name[64];
    char filepath[128];
    BOSX_ProcessState state;
    uint32_t memory_used;
    uint32_t window_count;
} BOSX_Process;

void BOSX_Init(void);
int BOSX_Load(const char* filepath);
void bosx_loader_open(const char* filepath);
void bosx_cleanup_process(uint32_t pid);
BOSX_Process* BOSX_GetProcessByPID(uint32_t pid);
void BOSX_ProcessMonitor_Display(void);

#endif // BOSX_LOADER_H
