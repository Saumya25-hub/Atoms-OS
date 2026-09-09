/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Process Lifecycle Implementation
 */

#include "apal_process.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"

apal_pid_t apal_process_getpid(void) {
    return (apal_pid_t)atoms_sys_getpid();
}

void apal_process_exit(int code) {
    atoms_sys_exit(code);
}

apal_status_t apal_process_launch(const char *path, const char **argv, const char **envp, apal_process_t *out_proc) {
    if (!path || !out_proc) return APAL_ERR_INVALID_PARAM;

    int64_t pid = __syscall3(SYS_EXEC, (int64_t)path, (int64_t)argv, (int64_t)envp);
    if (pid <= 0) {
        out_proc->pid = 0;
        out_proc->is_valid = false;
        out_proc->exit_code = -1;
        return APAL_ERR_NOT_FOUND;
    }

    out_proc->pid = (apal_pid_t)pid;
    out_proc->is_valid = true;
    out_proc->exit_code = 0;
    return APAL_OK;
}

apal_status_t apal_process_wait(apal_process_t *proc, int *out_exit_code, uint32_t timeout_ms) {
    if (!proc || !proc->is_valid) return APAL_ERR_INVALID_PARAM;
    (void)timeout_ms;

    int32_t status = 0;
    int64_t reaped_pid = atoms_sys_waitpid(proc->pid, &status, 0);
    if (reaped_pid > 0) {
        proc->exit_code = status;
        proc->is_valid = false;
        if (out_exit_code) {
            *out_exit_code = status;
        }
        return APAL_OK;
    }
    return APAL_ERR_NOT_FOUND;
}

apal_status_t apal_process_kill(apal_process_t *proc, int signal) {
    if (!proc || !proc->is_valid) return APAL_ERR_INVALID_PARAM;
    int64_t res = atoms_sys_kill(proc->pid, signal);
    if (res == 0) {
        return APAL_OK;
    }
    return APAL_ERR_NOT_FOUND;
}

apal_status_t apal_process_get_status(apal_pid_t pid, atoms_process_status_t *out_status) {
    if (!out_status) return APAL_ERR_INVALID_PARAM;
    int64_t res = atoms_sys_process_status(pid, out_status);
    if (res == 0) {
        return APAL_OK;
    }
    return APAL_ERR_NOT_FOUND;
}
