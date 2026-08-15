# 📐 ARCHITECTURE PATCH PLAN: V1 CORE GUI SYSCALL ABI IMPLEMENTATION
**Subsystem:** ATOMS OS Kernel Syscall Gateway & Userspace Graphics Interface (`syscall.h`, `dispatcher.c`, `services.c`, `validation.c`, `syscalls_gui.h`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Implement the 8 Core V1 GUI System Calls (16-23) in the Kernel Syscall Gateway.
- Implement strict user-pointer and bounds validation.
- Provide process-owned window surface mapping and non-blocking event polling.
- Ensure 100% compilation and pure UEFI QEMU pre-flight validation.

---

## 2. Target Files for Modification
1. `kernel/core/syscall/include/syscall.h`
2. `kernel/core/syscall/src/validation.c`
3. `kernel/core/syscall/src/services.c`
4. `kernel/core/syscall/src/dispatcher.c`
5. `userspace/libbos_gui/include/syscalls_gui.h`

---

## 3. Detailed Syscall Signatures & Memory Safety Rules

### Syscall 16: `SYS_GUI_CREATE_WINDOW`
* **Input:** `int32_t x`, `int32_t y`, `int32_t w`, `int32_t h`, `uint32_t flags`, `const char* title_user_ptr`
* **Validation:** Clamp $w \le 1920, h \le 1080$, validate title string pointer in user memory.
* **Return:** `uint32_t win_id` (or `0` on failure).

### Syscall 17: `SYS_GUI_DESTROY_WINDOW`
* **Input:** `uint32_t win_id`
* **Validation:** Verify `win_id` exists and is owned by calling process PID.
* **Return:** `SYSCALL_OK` (0) or error code.

### Syscall 18: `SYS_GUI_SHOW_WINDOW`
* **Input:** `uint32_t win_id`, `uint32_t visible` (1 = show, 0 = hide)
* **Validation:** Ownership check.

### Syscall 19: `SYS_GUI_SET_BOUNDS`
* **Input:** `uint32_t win_id`, `int32_t x`, `int32_t y`, `int32_t w`, `int32_t h`
* **Validation:** Ownership check, geometry bounds check.

### Syscall 20: `SYS_GUI_MAP_SURFACE`
* **Input:** `uint32_t win_id`, `uint64_t* out_user_surface_ptr`, `uint32_t* out_stride_bytes`
* **Validation:** Writable user pointers, returns virtual mapping to window's private canvas.

### Syscall 21: `SYS_GUI_INVALIDATE`
* **Input:** `uint32_t win_id`, `int32_t x`, `int32_t y`, `int32_t w`, `int32_t h`
* **Validation:** Marks damaged rect for BWE Compositor re-rendering.

### Syscall 22: `SYS_GUI_POLL_EVENT`
* **Input:** `uint32_t win_id`, `BOS_GUIEvent* out_user_event`, `uint32_t event_struct_size`
* **Validation:** Verify `event_struct_size == sizeof(BOS_GUIEvent)`, writable user pointer.
* **Return:** `1` if event returned, `0` if queue empty.

### Syscall 23: `SYS_GUI_GET_SCREEN_INFO`
* **Input:** `uint32_t* out_w`, `uint32_t* out_h`, `uint32_t* out_bpp`
* **Validation:** Writable user pointers.

---

## 4. Rollback Plan
Revert changes to Git commit `8c1ee59`.
