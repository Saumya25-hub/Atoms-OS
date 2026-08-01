# 🏛️ COMCTL32.sll V1.0 Architectural Specification

> **Subsystem:** COMCTL32.sll V1.0 Ring 3 Common Controls Runtime & XP User Interface Framework  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Core Ring 3 Reusable User Interface Controls Library  

---

## 1. Executive Summary & Architectural Philosophy

**COMCTL32.sll V1.0** is the official Ring 3 Common Controls Runtime and XP User Interface Framework inside **ATOMS OS**. Designed following production control library principles from Windows `COMCTL32.dll`, ReactOS, Wine, and Win32 User Interface specifications, COMCTL32.sll provides the single authority for every reusable graphical control in the operating system.

### Core Architectural Mandates:
- **No Duplicate Controls**: Applications (Explorer, Settings, Control Panel, Installer, Browser, Terminal, Task Manager) MUST NOT implement their own custom widgets. All controls must come from COMCTL32.sll.
- **Layered Subsystem Flow**: COMCTL32 delegates window management to **USER32.sll**, graphics to **GDI32.sll**, system primitives to **KERNEL32.sll**, application contracts to **BAR**, shell objects to **BSOM**, files to **BFS**, display acceleration to **AGP**, and system calls to the **Kernel**.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │       Ring 3 Applications (Explorer, Apps, Settings)        │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard Win32 Common Controls API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      COMCTL32.sll                           │
 │  ├── 1. Button Engine        ├── 16. ToolTip Engine         │
 │  ├── 2. Edit Engine          ├── 17. ReBar Engine           │
 │  ├── 3. Static Controls      ├── 18. UpDown Control         │
 │  ├── 4. ListBox              ├── 19. Pager Engine           │
 │  ├── 5. ComboBox             ├── 20. Animation Engine       │
 │  ├── 6. ListView             ├── 21. Month Calendar         │
 │  ├── 7. TreeView             ├── 22. Date Time Picker       │
 │  ├── 8. Tab Control          ├── 23. HotKey Control         │
 │  ├── 9. Toolbar              ├── 24. IP Address Control     │
 │  ├── 10. StatusBar           ├── 25. Theme Runtime          │
 │  ├── 11. Progress Engine     ├── 26. Layout Engine          │
 │  ├── 12. TrackBar            ├── 27. Notifications Engine   │
 │  ├── 13. Header Engine       ├── 28. Accessibility Engine   │
 │  ├── 14. ImageList           └── 29. Diagnostics Engine     │
 │  └── 15. Runtime Manager                                    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Window & Control Message Dispatch
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
 └──────────────────────────────┬──────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/comctl32/`)

```text
userspace/libs/comctl32/
├── include/
│   ├── comctl32_types.h
│   ├── comctl32_api.h
│   └── comctl32_public.h
├── core/
│   └── comctl32_runtime.c
├── button/
│   └── comctl32_button.c
├── edit/
│   └── comctl32_edit.c
├── static/
│   └── comctl32_static.c
├── listbox/
│   └── comctl32_listbox.c
├── combobox/
│   └── comctl32_combobox.c
├── listview/
│   └── comctl32_listview.c
├── treeview/
│   └── comctl32_treeview.c
├── tab/
│   └── comctl32_tab.c
├── toolbar/
│   └── comctl32_toolbar.c
├── statusbar/
│   └── comctl32_statusbar.c
├── progress/
│   └── comctl32_progress.c
├── trackbar/
│   └── comctl32_trackbar.c
├── header/
│   └── comctl32_header.c
├── imagelist/
│   └── comctl32_imagelist.c
├── tooltip/
│   └── comctl32_tooltip.c
├── rebar/
│   └── comctl32_rebar.c
├── updown/
│   └── comctl32_updown.c
├── pager/
│   └── comctl32_pager.c
├── animation/
│   └── comctl32_animation.c
├── monthcal/
│   └── comctl32_monthcal.c
├── datetime/
│   └── comctl32_datetime.c
├── hotkey/
│   └── comctl32_hotkey.c
├── ipaddress/
│   └── comctl32_ipaddress.c
├── theme/
│   └── comctl32_theme.c
├── layout/
│   └── comctl32_layout.c
├── notifications/
│   └── comctl32_notifications.c
├── accessibility/
│   └── comctl32_accessibility.c
├── diagnostics/
│   └── comctl32_diagnostics.c
├── tests/
│   └── comctl32_certification_tests.c
└── docs/
    └── comctl32_runtime.md
```

---

## 3. Core Engine Responsibilities Matrix

1. **Runtime Manager**: Control factory, handle tables, notification dispatcher.
2. **Button Engine**: PushButton, CheckBox, RadioButton, GroupBox, SplitButton (`CreateButton`).
3. **Edit Engine**: Single/Multi-line, password, undo, caret, selection (`CreateEdit`).
4. **Static Controls**: Label, Frame, Icon, Hyperlink, Separator (`CreateStatic`).
5. **ListBox Engine**: Sorting, Owner-draw, Multi-select listbox (`CreateListBox`).
6. **ComboBox Engine**: DropDown, DropList, Auto-complete combobox (`CreateComboBox`).
7. **ListView Engine**: Large/Small icons, Details, Tiles, Virtual mode (`CreateListView`).
8. **TreeView Engine**: Hierarchy tree, Expand/Collapse, Checkboxes (`CreateTreeView`).
9. **Tab Control**: Multipage tab headers, icons, closable tabs (`CreateTabControl`).
10. **Toolbar Engine**: Buttons, Dropdown, Separators, Overflow (`CreateToolbar`).
11. **StatusBar Engine**: Multipanel statusbar, resize grip (`CreateStatusBar`).
12. **Progress Engine**: Standard, Marquee, Smooth progress bars (`CreateProgressBar`).
13. **TrackBar Engine**: Horizontal/Vertical sliders with tick marks (`CreateTrackBar`).
14. **Header Engine**: Column headers, resizable, sorting arrows (`CreateHeader`).
15. **ImageList Engine**: System icon repository, overlays, caching (`CreateImageList`).
16. **ToolTip Engine**: Balloon tips, delay tracking (`CreateToolTip`).
17. **ReBar Engine**: Movable, dockable bands (`CreateReBar`).
18. **UpDown Engine**: Spin control buddy box (`CreateUpDownControl`).
19. **Pager Engine**: Scrollable control container (`CreatePager`).
20. **Animation Engine**: AVI and frame animation player (`CreateAnimateControl`).
21. **Month Calendar**: Calendar grid selection (`CreateMonthCalendar`).
22. **Date Time Picker**: Calendar drop-down date time picker (`CreateDateTimePicker`).
23. **HotKey Control**: Keyboard shortcut recorder (`CreateHotKeyControl`).
24. **IP Address Control**: IPv4 address editor (`CreateIPAddressControl`).
25. **Theme Runtime**: XP Luna Blue, Silver, Olive, Classic, Dark, Light.
26. **Layout Engine**: Anchors, docking, DPI scaling layout engine.
27. **Notification Dispatcher**: `WM_NOTIFY` message dispatcher (`NMHDR`).
28. **Accessibility**: High contrast, keyboard focus, screen reader hooks.
29. **Diagnostics Engine**: Paint profiler, leak audit, metric meter.
