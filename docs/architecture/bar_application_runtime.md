# 🏛️ BOS Application Runtime (BAR V1.0) Architectural Specification

> **Phase:** 11 — BOS Application Runtime (BAR V1.0)  
> **Authority:** Ring 3 Unified Application Subsystem Authority  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Kernel  

---

## 1. Executive Summary

The **BOS Application Runtime (BAR V1.0)** is the single, authoritative application runtime platform for Signatures OS / ATOMS OS. Inspired by the architectural isolation principles of Windows NT Executive (USER32/KERNEL32/SHELL32), CSRSS, ReactOS, Wayland, and Wine, BAR provides the complete application framework for process lifecycle, windowing, message routing, DLL loading, focus management, package manifests, and resource handling.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │                Applications (Explorer, Apps, Games)         │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Unified BAR Public API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                 BOS Application Runtime (BAR)               │
 │  ├── 1. Runtime Manager       ├── 11. Window Ownership      │
 │  ├── 2. Process Engine        ├── 12. Resource Manager       │
 │  ├── 3. Window Engine         ├── 13. Menu Runtime          │
 │  ├── 4. Session Engine        ├── 14. Cursor Manager        │
 │  ├── 5. App Registry          ├── 15. Dialog Runtime        │
 │  ├── 6. DLL Runtime           ├── 16. Package Manager       │
 │  ├── 7. Message Queue Engine  ├── 17. Manifest Manager      │
 │  ├── 8. Dispatcher Engine     ├── 18. Permission Manager    │
 │  ├── 9. Timer Engine          ├── 19. Security Engine       │
 │  └── 10. Focus Manager        └── 20. Diagnostics Engine    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Object Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │            BSOM (BOS Shell Object Model Engine)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ File Operations & Storage
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │               BFS (BOS File System Engine)                  │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Display & Shell Runtime
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │              BRT (BOS Runtime Environment)                  │
 └──────────────────────────────┬──────────────────────────────┘
                                │ GPU Graphics Platform
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │              AGP (ATOMS Graphics Platform V1.0)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ System Calls & Hardware
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      x86_64 Kernel                          │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Directory Layout (`kernel/bar/`)

```text
kernel/bar/
├── include/
│   ├── bar_types.h
│   ├── bar_api.h
│   └── bar_public.h
├── core/
│   └── bar_runtime.c
├── process/
│   └── bar_process.c
├── runtime/
│   └── bar_session.c
├── message/
│   └── bar_message.c
├── dispatcher/
│   └── bar_dispatcher.c
├── windows/
│   └── bar_windows.c
├── resources/
│   └── bar_resources.c
├── dialogs/
│   └── bar_dialogs.c
├── clipboard/
│   └── bar_clipboard.c
├── dragdrop/
│   └── bar_dragdrop.c
├── timers/
│   └── bar_timers.c
├── focus/
│   └── bar_focus.c
├── packages/
│   └── bar_packages.c
├── manifest/
│   └── bar_manifest.c
├── loader/
│   └── bar_dll.c
├── registry/
│   └── bar_registry.c
├── security/
│   └── bar_security.c
├── diagnostics/
│   └── bar_diagnostics.c
├── tests/
│   └── bar_certification_tests.c
└── docs/
    └── bar_application_runtime.md
```

---

## 3. The 20 Core BAR Engines

1. **Application Runtime Manager**: Initializes runtime pools, global state, handle tables.
2. **Process Runtime Engine**: Creates, tracks, and terminates application processes.
3. **Window Runtime Engine**: Manages window handles, styles, bounds, visibility state.
4. **Application Session Engine**: Handles user sessions, login/logout, state persistence.
5. **Application Registry**: Central database for registered applications and file associations.
6. **DLL Runtime**: Dynamic library loading (`BAR_LoadLibrary`, `BAR_GetProcAddress`).
7. **Message Queue Engine**: Per-thread and per-window message queues for input events.
8. **Dispatcher Engine**: Routes messages synchronously and asynchronously (`BAR_DispatchMessage`).
9. **Timer Engine**: High-resolution application timers (`BAR_SetTimer`, `BAR_KillTimer`).
10. **Focus Manager**: Active window focus, keyboard focus, and window activation tracking.
11. **Window Ownership Manager**: Parent-child window relationships, modal ownership.
12. **Resource Manager**: Loads icons, bitmaps, strings, and custom application resources.
13. **Menu Runtime**: Application menu bars, context menus, command dispatching.
14. **Cursor Manager**: System and custom application cursor state management.
15. **Dialog Runtime**: Modal and modeless dialog boxes (`BAR_CreateDialog`, `BAR_ShowDialog`).
16. **Package Manager**: Application bundle installation, package verification.
17. **Manifest Manager**: Parses application XML/JSON manifests, security declarations.
18. **Permission Manager**: Validates application capabilities (Network, Disk, Hardware).
19. **Security Engine**: Process isolation, sandboxing enforcement, security tokens.
20. **Diagnostics Engine**: Loggers, state dumps, deadlock detection, memory leak audit.
