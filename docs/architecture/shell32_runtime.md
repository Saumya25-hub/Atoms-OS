# 🏛️ SHELL32.sll V1.0 Architectural Specification

> **Subsystem:** SHELL32.sll V1.0 Ring 3 Shell Runtime, Desktop Services & Explorer Framework  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Core Ring 3 Desktop Shell & Application Launch Subsystem  

---

## 1. Executive Summary & Architectural Philosophy

**SHELL32.sll V1.0** is the official Ring 3 Shell Runtime, Desktop Services & Explorer Framework inside **ATOMS OS**. Designed following production shell library principles from Windows `SHELL32.dll`, ReactOS, Wine, and Win32 Shell API specifications, SHELL32.sll provides the single authority for Desktop management, Explorer runtime, Shell objects, Recycle Bin, Shortcuts (`.slink`), Context Menus, File Associations, Drag & Drop, Application Launching (`ShellExecute`), Notifications, Taskbar, System Tray, Search, Special Folders, and Shell Services.

### Core Architectural Mandates:
- **Desktop Experience Ownership**: SHELL32 owns shell interaction, shortcut resolution, launcher routing, context menu dispatching, taskbar pinning, and system tray status.
- **Zero Duplication**: Applications (Explorer, Desktop, Taskbar, Control Panel, Settings, Installer, Terminal, Browser) delegate all shell operations to SHELL32.sll.
- **Layered Subsystem Flow**: SHELL32 delegates window management to **USER32.sll**, graphics to **GDI32.sll**, controls to **COMCTL32.sll**, common dialogs to **COMDLG32.sll**, system primitives to **KERNEL32.sll**, application contracts to **BAR**, shell objects to **BSOM**, files to **BFS**, display acceleration to **AGP**, and system calls to the **Kernel**.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │       Ring 3 Applications (Explorer, Desktop, Taskbar)      │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard Win32 Shell API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      SHELL32.sll                            │
 │  ├── 1. Runtime Manager       ├── 11. File Associations      │
 │  ├── 2. Desktop Engine        ├── 12. Execute Engine         │
 │  ├── 3. Explorer Runtime      ├── 13. Properties Engine      │
 │  ├── 4. Namespace Engine      ├── 14. Clipboard Integration  │
 │  ├── 5. Special Folders       ├── 15. Drag & Drop Engine     │
 │  ├── 6. Recycle Bin Engine    ├── 16. Notification Engine    │
 │  ├── 7. Shortcut Engine       ├── 17. Taskbar Integration    │
 │  ├── 8. Icon Manager          ├── 18. System Tray Engine     │
 │  ├── 9. ImageList Engine      ├── 19. Search Engine          │
 │  └── 10. Context Menu Engine  └── 20. Diagnostics Engine     │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Subsystem Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │  USER32.sll | GDI32.sll | COMCTL32.sll | COMDLG32.sll | ...  │
 └──────────────────────────────┬──────────────────────────────┘
                                │ BAR Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                     BAR Application Runtime                 │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Shell Object Model Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │               BSOM (BOS Shell Object Model Engine)          │
 └──────────────────────────────┬──────────────────────────────┘
                                │ File System Engine
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                BFS (BOS File System Engine)                 │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Display & GPU Acceleration
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │               AGP (ATOMS Graphics Platform V1.0)            │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Kernel Syscalls
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                       x86_64 Kernel                         │
 └──────────────────────────────┬──────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/shell32/`)

```text
userspace/libs/shell32/
├── include/
│   ├── shell32_types.h
│   ├── shell32_api.h
│   └── shell32_public.h
├── core/
│   └── shell_runtime.c
├── desktop/
│   └── shell_desktop.c
├── explorer/
│   └── shell_explorer.c
├── namespace/
│   └── shell_namespace.c
├── folders/
│   └── shell_specialfolders.c
├── recyclebin/
│   └── shell_recyclebin.c
├── shortcuts/
│   └── shell_shortcuts.c
├── icons/
│   └── shell_icons.c
├── imagelist/
│   └── shell_imagelist.c
├── contextmenu/
│   └── shell_contextmenu.c
├── fileassoc/
│   └── shell_fileassoc.c
├── launch/
│   └── shell_execute.c
├── properties/
│   └── shell_properties.c
├── clipboard/
│   └── shell_clipboard.c
├── dragdrop/
│   └── shell_dragdrop.c
├── notifications/
│   └── shell_notifications.c
├── taskbar/
│   └── shell_taskbar.c
├── tray/
│   └── shell_tray.c
├── search/
│   └── shell_search.c
├── history/
│   └── shell_history.c
├── diagnostics/
│   └── shell_diagnostics.c
├── tests/
│   └── shell32_certification_tests.c
└── docs/
    └── shell32_runtime.md
```

---

## 3. Core Engine Responsibilities Matrix

1. **Runtime Manager**: Shell subsystem lifecycle, registry initialization.
2. **Desktop Engine**: Desktop surface, wallpaper, grid icon layout.
3. **Explorer Runtime**: Explorer window lifecycle, navigation history, refresh triggers.
4. **Namespace Engine**: Virtual desktop, My Computer, Documents, Downloads, Network, USB, Recycle Bin.
5. **Special Folder Engine**: Known folders, environment paths (`%USERPROFILE%`, `%APPDATA%`).
6. **Recycle Bin Engine**: Soft delete, restore, empty recycle bin operations.
7. **Shortcut Engine**: `.slink` parsing, target resolution, icon override, argument parsing.
8. **Icon Manager**: System icon cache (Folder, Drive, File, USB, Network, App).
9. **ImageList Engine**: System icon repository for Explorer treeview, listview, and taskbar.
10. **Context Menu Engine**: Right-click shell menu dispatcher (Open, Edit, Print, Copy, Cut, Paste, Delete, Properties).
11. **File Association Engine**: File extension handlers mapping (`.txt`, `.png`, `.exe`, `.pdf`, `.doc`).
12. **Execute Engine**: Process launching (`ShellExecute`, `ShellExecuteEx`).
13. **Properties Engine**: Detailed property dialogs for files, folders, drives, and shortcuts.
14. **Clipboard Integration**: Shell object clipboard operations (Copy, Cut, Paste, CF_HDROP).
15. **Drag & Drop Engine**: Shell drag & drop target validation, copy, move, link operations.
16. **Notification Engine**: System toast notifications, balloon tips, taskbar alerts.
17. **Taskbar Integration**: Pinned applications, jump lists, active application buttons.
18. **System Tray Engine**: Status icons (Clock, Volume, Battery, Network, USB ejection).
19. **Search Engine**: Shell file search, metadata query, path indexing.
20. **History Engine**: Shell navigation history, recent documents store.
21. **Diagnostics Engine**: Shell handle audit, memory leak detector, crash log reporter.
