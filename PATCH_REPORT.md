# 🛠️ PATCH REPORT: V1 CORE GUI SYSCALL ABI IMPLEMENTATION
**Subsystem:** ATOMS OS Kernel Syscall Gateway & Userspace Graphics Interface (`syscall.h`, `dispatcher.c`, `services.c`, `validation.c`, `syscalls_gui.h`)  
**Patch Engineer:** Antigravity / ARYA Core Patch Team  
**Date:** 2026-08-16  
**Status:** TASK 3 COMPLETE (Patch Phase)

---

## 1. Files & Functions Changed

### 1. `kernel/core/syscall/include/syscall.h`
* **Changes:**
  - Added Frozen V1 Core GUI Syscall Numbers (`SYS_GUI_CREATE_WINDOW` 16 to `SYS_GUI_GET_SCREEN_INFO` 23).
  - Defined ABI-versioned `BOS_GUIEvent` structure with 64-bit alignment.
  - Declared `sys_service_gui_*` kernel function prototypes.

### 2. `kernel/core/syscall/src/validation.c`
* **Functions:** `syscall_validate_user_string()`
* **Changes:**
  - Added null-terminated user-space string boundary validation within `[USER_WINDOW_MIN, USER_WINDOW_MAX)`.

### 3. `kernel/core/syscall/src/services.c`
* **Functions:** `sys_service_gui_create_window`, `sys_service_gui_destroy_window`, `sys_service_gui_show_window`, `sys_service_gui_set_bounds`, `sys_service_gui_map_surface`, `sys_service_gui_invalidate`, `sys_service_gui_poll_event`, `sys_service_gui_get_screen_info`
* **Changes:**
  - Integrated BWE window lifecycle management with strict process PID ownership checks.
  - Implemented window private canvas surface allocation and mapping.
  - Implemented per-window circular event queues (`MAX_GUI_EVENTS_PER_WIN = 32`).

### 4. `kernel/core/syscall/src/dispatcher.c`
* **Function:** `syscall_dispatch()`
* **Changes:**
  - Added switch cases for `SYS_GUI_*` syscalls (16 to 23).

### 5. `userspace/libbos_gui/include/syscalls_gui.h`
* **Changes:**
  - Synchronized userspace GUI header with the exact frozen V1 Core ABI.
