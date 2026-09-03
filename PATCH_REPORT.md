# ATOMS OS — PATCH REPORT: SYSCALL SECURITY & USER POINTER HARDENING

## 1. Metadata
- **Subsystem**: Ring 3 -> Ring 0 Syscall Boundary & Memory Validation
- **Architecture**: x86_64 Long Mode (Pure UEFI)
- **Classification**: 🔴 CONFIRMED BUG -> FIXED
- **Date**: 2026-09-03
- **Status**: PATCH APPLIED & VALIDATED

---

## 2. Files Modified & Functions Changed

### A. Syscall Validation Core
- **File**: [`kernel/core/syscall/src/validation.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/validation.c)
  - **Functions Changed / Added**:
    - `syscall_validate_user_ptr(const void *ptr, size_t size)`: Replaced naive static pointer range check with canonical address verification, boundary overflow protection, and dynamic VMM PML4 page table walk (`vmm_validate_user_range` with `VMM_ACCESS_READ`).
    - `syscall_validate_user_ptr_writable(void *ptr, size_t size)`: Added new function verifying user ranges for read/write access (`VMM_ACCESS_READ | VMM_ACCESS_WRITE`), preventing kernel writes into read-only user pages.
    - `syscall_validate_user_string(const char *str, size_t max_len)`: Added safe byte-by-byte string validation that walks page table presence and user permissions before crossing each 4KB page boundary. Prevents Ring 0 `#PF` crashes from unterminated strings.
  - **Lines Changed**: 1-105

### B. Syscall Header Declarations
- **File**: [`kernel/core/syscall/include/syscall.h`](file:///d:/Signatures_OS/kernel/core/syscall/include/syscall.h)
  - **Declarations Added**:
    - `bool syscall_validate_user_ptr_writable(void *ptr, size_t size);`
    - `bool syscall_validate_user_string(const char *str, size_t max_len);`
  - **Lines Changed**: 142-146

### C. VMM Subsystem Interop
- **File**: [`kernel/core/memory/vmm/include/vmm.h`](file:///d:/Signatures_OS/kernel/core/memory/vmm/include/vmm.h)
  - **Declarations Added**: `bool vmm_address_canonical(uint64_t address);`
  - **Lines Changed**: 9
- **File**: [`kernel/core/memory/vmm/src/vmm.c`](file:///d:/Signatures_OS/kernel/core/memory/vmm/src/vmm.c)
  - **Changes**: Made `vmm_address_canonical()` non-static for architectural reuse across kernel validation layers.
  - **Lines Changed**: 273

### D. Syscall Service Handlers
- **File**: [`kernel/core/syscall/src/services.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c)
  - **Functions Changed**:
    - `sys_service_debug_print`: Replaced unsafe 1-byte pointer check with `syscall_validate_user_string(msg, 512)`.
    - `sys_service_open`: Replaced unsafe 1-byte pointer check with `syscall_validate_user_string(path, 256)`.
    - `sys_service_clock_gettime`: Replaced read check with `syscall_validate_user_ptr_writable(tp, sizeof(struct timespec))`.
    - `sys_service_gui_map_surface`: Replaced read check with `syscall_validate_user_ptr_writable(fb_ptr_out, sizeof(void*))`.
    - `sys_service_gui_poll_event`: Replaced read check with `syscall_validate_user_ptr_writable(event, sizeof(gui_event_t))`.
    - `sys_service_gui_get_screen_info`: Replaced read check with `syscall_validate_user_ptr_writable(info, sizeof(gui_screen_info_t))`.
    - `sys_service_read`: Replaced read check with `syscall_validate_user_ptr_writable(buf, count)`.
  - **Lines Changed**: 97, 260-270, 376-415, 623-665

### E. Dedicated Forensic Debug Dashboard & Harness
- **File [NEW]**: [`kernel/debug/syscall_security_debug.h`](file:///d:/Signatures_OS/kernel/debug/syscall_security_debug.h)
  - Declared diagnostic telemetry structs, fault containment counters, and test cases A through J.
- **File [NEW]**: [`kernel/debug/syscall_security_debug.c`](file:///d:/Signatures_OS/kernel/debug/syscall_security_debug.c)
  - Implemented full-screen ABDE diagnostic dashboard (`ATOMS SYSCALL SECURITY FORENSIC`).
  - Implemented real-time rotating heartbeat spinner (`| / - \`), non-blocking rate-limited screenshot sender (`atoms_screenshot_step`), functional test harness (Tests A–J), and 600-cycle stress harness.
- **File**: [`kernel/kernel.c`](file:///d:/Signatures_OS/kernel/kernel.c)
  - Integrated `ATOMS_DEBUG_MODE_SYSCALL_SECURITY` and executed test harness prior to desktop entry.
- **File**: [`build.ps1`](file:///d:/Signatures_OS/build.ps1)
  - Integrated `syscall_security_debug.c` into kernel compilation and link script response file.

---

## 3. Subsystem Freeze & Rule 0 Compliance
- **VMM Lifecycle**: 100% Frozen (only exposed existing canonical address checker).
- **Scheduler**: 100% Frozen.
- **USB / xHCI Stack**: 100% Frozen (All keyboard LED and transfer improvements remain intact).
- **Compositor & Desktop Shell**: 100% Frozen.
- **PMM**: 100% Frozen.
