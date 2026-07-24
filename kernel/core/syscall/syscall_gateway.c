#include "syscall_gateway.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/application/window_api/app_window_api.h"
#include "kernel/application/clipboard/app_clipboard.h"
#include "kernel/application/dialogs/app_dialogs.h"
#include "kernel/application/future_vfs_api/app_vfs_api.h"

extern void bwe_log(const char* level, const char* msg);

void ATOMS_SyscallGateway_Init(void) {
    bwe_log("INFO", "ATOMS Protected System Call Gateway Initialized");
}

uint64_t ATOMS_Syscall_Dispatch(uint32_t syscall_nr, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    switch (syscall_nr) {
        case SYS_WINDOW_CREATE:
            return ATOMS_CreateWindowForApp((uint32_t)arg1, (int32_t)arg2, (int32_t)arg3, 400, 300, (const char*)arg4, 0);

        case SYS_WINDOW_CLOSE:
            return ATOMS_CloseWindowForApp((uint32_t)arg1, (uint32_t)arg2);

        case SYS_FILESYSTEM_OPEN:
            return (uint64_t)ATOMS_VFS_OpenFile((uint32_t)arg1, (const char*)arg2, (const char*)arg3);

        case SYS_FILESYSTEM_READ:
            return (uint64_t)ATOMS_VFS_ReadFile((int32_t)arg1, (void*)arg2, (uint32_t)arg3);

        case SYS_FILESYSTEM_WRITE:
            return (uint64_t)ATOMS_VFS_WriteFile((int32_t)arg1, (const void*)arg2, (uint32_t)arg3);

        case SYS_CLIPBOARD_SET:
            return ATOMS_Clipboard_SetText((uint32_t)arg1, (const char*)arg2) ? 1 : 0;

        case SYS_CLIPBOARD_GET:
            return (uint64_t)ATOMS_Clipboard_GetText();

        case SYS_DIALOG_SHOW:
            return ATOMS_ShowMessageBox((uint32_t)arg1, (const char*)arg2, (const char*)arg3, ATOMS_DIALOG_ICON_INFO, 0);

        default:
            return (uint64_t)-1; // Invalid Syscall
    }
}
