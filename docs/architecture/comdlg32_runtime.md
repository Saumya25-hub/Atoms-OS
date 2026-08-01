# 🏛️ COMDLG32.sll V1.0 Architectural Specification

> **Subsystem:** COMDLG32.sll V1.0 Ring 3 Common Dialog Runtime & Application Dialog Framework  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Core Ring 3 Common Dialog Runtime (Used by all Apps & Shell Components)  

---

## 1. Executive Summary & Architectural Philosophy

**COMDLG32.sll V1.0** is the official Ring 3 Common Dialog Runtime and Application Dialog Framework inside **ATOMS OS**. Modeled after Windows `COMDLG32.dll`, ReactOS, Wine, Qt Dialog Framework, GTK File Chooser, and KDE Dialog Runtime, COMDLG32.sll provides standardized, reusable Windows XP style common dialogs (`GetOpenFileName`, `GetSaveFileName`, `ChooseColor`, `ChooseFont`, `PrintDlg`, `PageSetupDlg`, `FindText`, `ReplaceText`, `SHBrowseForFolder`).

### Core Architectural Mandates:
- **No Manual Application Dialogs**: No Ring 3 application creates custom open/save/color/font dialogs. Everything delegates to COMDLG32.sll.
- **Layered Subsystem Flow**: COMDLG32 delegates window management to **USER32.sll**, graphics to **GDI32.sll**, system primitives to **KERNEL32.sll**, application contracts to **BAR**, shell objects to **BSOM**, files to **BFS**, display acceleration to **AGP**, and system calls to the **Kernel**.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │    Ring 3 Applications (Explorer, Paint, Browser, Apps)     │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard Win32 Common Dialog API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      COMDLG32.sll                           │
 │  ├── 1. Open/Save Engine      ├── 11. Quick Access          │
 │  ├── 2. Folder Picker         ├── 12. Sidebar Runtime       │
 │  ├── 3. Color Dialog          ├── 13. Filters Engine        │
 │  ├── 4. Font Dialog           ├── 14. Navigation Engine     │
 │  ├── 5. Print Dialog          ├── 15. Validation Engine     │
 │  ├── 6. Page Setup            ├── 16. Layout Engine         │
 │  ├── 7. Find / Replace        ├── 17. Theme Engine          │
 │  ├── 8. Preview Engine        ├── 18. Icon Engine           │
 │  ├── 9. History Manager       ├── 19. Bookmarks Engine      │
 │  └── 10. Favorites Engine     └── 20. Diagnostics Engine    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Window & Input Routing
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                         USER32.sll                          │
 └──────────────────────────────┬──────────────────────────────┘
                                │ 2D Graphics Acceleration
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                         GDI32.sll                           │
 └──────────────────────────────┬──────────────────────────────┘
                                │ System Runtime Primitives
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                       KERNEL32.sll                          │
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
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/comdlg32/`)

```text
userspace/libs/comdlg32/
├── include/
│   ├── comdlg32_types.h
│   ├── comdlg32_api.h
│   └── comdlg32_public.h
├── core/
│   └── comdlg32_runtime.c
├── filedialog/
│   └── comdlg32_open_save.c
├── folderdialog/
│   └── comdlg32_folder.c
├── colordialog/
│   └── comdlg32_color.c
├── fontdialog/
│   └── comdlg32_font.c
├── printdialog/
│   └── comdlg32_print.c
├── pagedialog/
│   └── comdlg32_page_setup.c
├── findreplace/
│   └── comdlg32_find_replace.c
├── preview/
│   └── comdlg32_preview.c
├── history/
│   └── comdlg32_history.c
├── favorites/
│   └── comdlg32_favorites.c
├── quickaccess/
│   └── comdlg32_quickaccess.c
├── sidebar/
│   └── comdlg32_sidebar.c
├── filters/
│   └── comdlg32_filters.c
├── navigation/
│   └── comdlg32_navigation.c
├── validation/
│   └── comdlg32_validation.c
├── layout/
│   └── comdlg32_layout.c
├── theme/
│   └── comdlg32_theme.c
├── icons/
│   └── comdlg32_icons.c
├── bookmarks/
│   └── comdlg32_bookmarks.c
├── diagnostics/
│   └── comdlg32_diagnostics.c
├── tests/
│   └── comdlg32_certification_tests.c
└── docs/
    └── comdlg32_runtime.md
```

---

## 3. Core Engine Responsibilities Matrix

1. **Runtime Manager**: Subsystem lifecycle, dialog registration, handle tracking.
2. **Open/Save File Dialog**: Windows XP style file open and save dialogs (`GetOpenFileName`, `GetSaveFileName`).
3. **Folder Picker**: Tree-based directory browser (`SHBrowseForFolder`).
4. **Color Dialog**: RGB, HSV, palette, custom and recent colors (`ChooseColor`).
5. **Font Dialog**: System font enumerator, size, style, and preview (`ChooseFont`).
6. **Print Dialog**: Printer selection, copies, orientation, margins (`PrintDlg`).
7. **Page Setup**: Page dimensions, paper orientation, margins (`PageSetupDlg`).
8. **Find / Replace**: Search history, case sensitivity, whole word matching (`FindText`, `ReplaceText`).
9. **Preview Engine**: Thumbnail and text file content preview buffer.
10. **History Manager**: Navigation history and recently accessed directories.
11. **Favorites Engine**: User-pinned directories and favorites manager.
12. **Quick Access Engine**: System quick access locations (Desktop, Documents, Downloads, USB).
13. **Sidebar Runtime**: Places bar (Desktop, This PC, Network, USB, Downloads).
14. **Filters Engine**: File extension pattern matching (`*.txt`, `*.png`, `*.bmp`, `*.exe`, `*.*`).
15. **Navigation Engine**: Back, Forward, Up, Breadcrumbs, Address bar parsing.
16. **Validation Engine**: Illegal character check, overwrite confirmation, path existence check.
17. **Layout Engine**: Windows XP standard dialog layout engine.
18. **Theme Engine**: XP Classic, Silver, Blue, Dark, and Light themes.
19. **Icon Engine**: File, Folder, Drive, USB, and Network icon renderer.
20. **Bookmarks Engine**: Persistent folder bookmarks.
21. **Diagnostics Engine**: Latency, open handles, memory leak audit, test profiler.
