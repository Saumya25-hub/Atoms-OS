/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Process Lifecycle Adapter (Chromium base::Process / base::LaunchProcess)
 */

#ifndef ATOMS_APAL_PROCESS_H
#define ATOMS_APAL_PROCESS_H

#include "../include/apal_types.h"

#include "atoms_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t apal_pid_t;

typedef struct {
    apal_pid_t pid;
    int exit_code;
    bool is_valid;
} apal_process_t;

apal_pid_t apal_process_getpid(void);
apal_status_t apal_process_launch(const char *path, const char **argv, const char **envp, apal_process_t *out_proc);
apal_status_t apal_process_wait(apal_process_t *proc, int *out_exit_code, uint32_t timeout_ms);
apal_status_t apal_process_kill(apal_process_t *proc, int signal);
apal_status_t apal_process_get_status(apal_pid_t pid, atoms_process_status_t *out_status);
void apal_process_exit(int code);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_PROCESS_H */
