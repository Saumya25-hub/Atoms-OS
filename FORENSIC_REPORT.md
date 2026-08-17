# FORENSIC REPORT — MILESTONE 3: RING 3 DESKTOP SHELL MIGRATION

## 1. Objective & Scope
Migrate the ATOMS OS Desktop Shell from kernel supervisor mode (Ring 0) into an autonomous, secure userspace process (Ring 3).

## 2. Forensic Audit of Existing Architecture
- **Ring 0 Hardware Authority**:
  - GOP Framebuffer, xHCI/PS2 USB Input Core, VMM Paging, Syscall Gateway, PMM Physical Allocator, and BWE Compositor are strictly situated in Ring 0.
  - GUI Syscalls V1 (`SYS_GUI_CREATE_WINDOW=16`, `SYS_GUI_DESTROY_WINDOW=17`, `SYS_GUI_SHOW_WINDOW=18`, `SYS_GUI_MAP_SURFACE=20`, `SYS_GUI_INVALIDATE=21`, `SYS_GUI_POLL_EVENT=22`, `SYS_GUI_GET_SCREEN_INFO=23`, `SYS_YIELD=3`, `SYS_WRITE=0`) are already fully operational and verified on physical H81 hardware.
- **Ring 3 Desktop Role**:
  - The desktop shell must create its desktop surface via `SYS_GUI_CREATE_WINDOW` + `SYS_GUI_MAP_SURFACE`.
  - It renders the desktop background, icons (`[My Computer]`, `[Files]`, `[Terminal]`, `[Settings]`), the bottom taskbar (`0xFF1E293B`), the Start button (`START`), and system clock.
  - It handles icon clicks, mouse movements, keyboard navigation, and application launching requests.
  - It receives events via `SYS_GUI_POLL_EVENT` and yields CPU time via `SYS_YIELD`.

## 3. Risk Analysis
- **Risk 1: Screen Resolution Mismatch**: Screen dimensions must be queried dynamically via `sys_gui_get_screen_info` or clamped to physical width/height so full taskbar and icons fit seamlessly.
- **Risk 2: Fallback in PXE Boot**: Pure network boot does not mount local disks; the embedded usermode fallback in `kernel.c` must spawn the complete Desktop Shell logic so real H81 PXE boot renders the full desktop immediately.

## 4. Root Cause & Suspected Fix
- Replace the minimal single-card `gui_demo` bytecode and application entry with the complete `desktop_shell` process.
- Desktop shell will initialize the desktop layout, render icons, taskbar, start button, and clock into its mapped user-private surface, invalidate via BWE compositor, and run the persistent interactive event loop.
