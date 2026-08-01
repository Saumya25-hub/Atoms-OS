# 🏛️ USER32.sll V1.0 Architectural Specification

> **Subsystem:** USER32.sll V1.0 API Runtime  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Kernel  
> **Layer:** Ring 3 Win32 / ATOMS Standard User Interface API Runtime  

---

## 1. Executive Summary & Architecture

**USER32.sll** is the Ring 3 API Runtime for user interface and window management in Signatures OS / ATOMS OS. Deeply informed by Windows USER32.dll, CSRSS, ReactOS, Wine, Wayland, and X11, USER32.sll provides standard Win32-compatible types (`HWND`, `MSG`, `WNDCLASS`, `PAINTSTRUCT`) while delegating all underlying window state, ownership, and object management directly to **BAR V1.0** and **BSOM**.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │                Ring 3 Application Code                      │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard Win32 API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      USER32.sll                             │
 │  ├── 1. Window Class Engine   ├── 11. Cursor Manager        │
 │  ├── 2. Window Creation       ├── 12. Clipboard Runtime     │
 │  ├── 3. Window Proc Dispatch  ├── 13. Drag Drop Runtime     │
 │  ├── 4. Message Queue Runtime ├── 14. Menu Runtime          │
 │  ├── 5. Input Dispatcher      ├── 15. Dialog Runtime        │
 │  ├── 6. Keyboard Engine       ├── 16. Common Controls       │
 │  ├── 7. Mouse Engine          ├── 17. Timer Runtime         │
 │  ├── 8. Focus Manager         ├── 18. Window Hooks          │
 │  ├── 9. Capture Manager       ├── 19. Accelerator Engine    │
 │  └── 10. Caret Engine         └── 20. Diagnostics Engine    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ BAR Application Runtime Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                 BOS Application Runtime (BAR)               │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Shell Object Model Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │            BSOM (BOS Shell Object Model Engine)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ GPU Graphics Platform
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │              AGP (ATOMS Graphics Platform V1.0)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Kernel Syscalls & Drivers
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      x86_64 Kernel                          │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Directory Structure (`userspace/libs/user32/`)

```text
userspace/libs/user32/
├── include/
│   ├── user32_types.h
│   ├── user32_api.h
│   └── user32_public.h
├── core/
│   └── user32_runtime.c
├── window/
│   ├── user32_window.c
│   └── user32_class.c
├── message/
│   └── user32_message.c
├── controls/
│   └── user32_controls.c
├── dialogs/
│   └── user32_dialogs.c
├── menus/
│   └── user32_menus.c
├── clipboard/
│   └── user32_clipboard.c
├── dragdrop/
│   └── user32_dragdrop.c
├── cursor/
│   └── user32_cursor.c
├── caret/
│   └── user32_caret.c
├── focus/
│   └── user32_focus.c
├── keyboard/
│   └── user32_keyboard.c
├── mouse/
│   └── user32_mouse.c
├── accelerators/
│   └── user32_accelerators.c
├── classes/
│   └── user32_classes.c
├── hooks/
│   └── user32_hooks.c
├── input/
│   └── user32_input.c
├── timers/
│   └── user32_timers.c
├── diagnostics/
│   └── user32_diagnostics.c
├── tests/
│   └── user32_certification_tests.c
└── docs/
    └── user32_runtime.md
```

---

## 3. Core Engine Responsibilities

1. **Window Class Manager**: Manages `WNDCLASS` and `WNDCLASSEX` registration tables.
2. **Window Creation Engine**: Maps `CreateWindowEx()` to `BAR_CreateWindow()`.
3. **Window Procedure Dispatcher**: Dispatches incoming messages to `WNDPROC` callbacks.
4. **Message Queue Runtime**: Manages thread message queues (`GetMessage`, `PeekMessage`).
5. **Input Dispatcher**: Translates hardware events into Win32 messages (`WM_KEYDOWN`, `WM_MOUSEMOVE`).
6. **Keyboard Engine**: Handles keyboard focus, key state tables, and virtual keys.
7. **Mouse Engine**: Mouse motion, button clicks, and hit testing (`WM_NCHITTEST`).
8. **Focus Manager**: Active window focus, activation messages (`WM_SETFOCUS`, `WM_KILLFOCUS`).
9. **Capture Manager**: Mouse capture management (`SetCapture`, `ReleaseCapture`).
10. **Caret Engine**: Text input caret creation, positioning, and blink timing.
11. **Cursor Manager**: System cursor loading (`LoadCursor`, `SetCursor`).
12. **Clipboard Runtime**: Win32 clipboard API (`OpenClipboard`, `SetClipboardData`).
13. **Drag Drop Runtime**: OLE-compatible drag and drop session management.
14. **Menu Runtime**: Menu bar creation, submenus, popup menus (`TrackPopupMenu`).
15. **Dialog Runtime**: Modal and modeless dialog loops (`DialogBox`, `CreateDialog`).
16. **Common Controls Runtime**: Button, Edit, Static, ListBox, ComboBox controls.
17. **Timer Runtime**: Win32 application timers (`SetTimer`, `KillTimer`).
18. **Window Hooks**: System-wide and thread-specific message hooks (`SetWindowsHookEx`).
19. **Accelerator Engine**: Keyboard shortcut accelerator tables (`TranslateAccelerator`).
20. **Diagnostics**: USER32 handle table audit, memory leak detection, message profiler.
