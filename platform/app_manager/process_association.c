#include "platform/include/bos_app_manager.h"
#include "platform/include/bos_window.h"

extern bool BOS_WindowRegistry_Lookup(BOS_WindowHandle handle, uint32_t* out_surface_id, uint32_t* out_owner_pid);
extern bool BOS_WindowRegistry_FreeSlot(BOS_WindowHandle handle);

void BOS_AppManager_CleanupOrphanWindows(uint32_t pid) {
    if (pid == 0) return;

    /* Scan window registry for any windows registered to terminating process ID */
    for (uint32_t slot = 0; slot < 512U; slot++) {
        BOS_WindowHandle handle = slot | (1U << 16); /* Handle search mask */
        uint32_t surface_id, owner_pid;
        if (BOS_WindowRegistry_Lookup(handle, &surface_id, &owner_pid)) {
            if (owner_pid == pid) {
                BOS_DestroyWindow(handle);
            }
        }
    }
}
