# 🏛️ Explorer.exe V1.0 Desktop Shell, Explorer Runtime & Session Manager Architecture Spec

## Subsystem Overview

**Explorer.exe V1.0** is the official **Desktop Session Host and Primary Shell Process** of **ATOMS OS**.
It is responsible for hosting the desktop session, desktop icons, wallpaper, taskbar, start menu, notification area, file browser windows, context menus, search engine, run dialog, control panel launcher, settings launcher, quick access, and recent documents.

```text
Applications (Browser, Paint, Settings, Terminal, Custom Apps)
        │
        ▼
   Explorer.exe (Desktop Shell & Session Host)
─────────────────────────────────────────────────────────────────
  Desktop Manager           Taskbar Manager       Start Menu
  Wallpaper Service         Notification Area     Icon Engine
  File Browser Windows      Context Menu Engine   Search Runtime
  Drag & Drop Handler       Clipboard Runtime     Recycle Bin UI
  Control Panel Launcher    Settings Launcher     Session Host
─────────────────────────────────────────────────────────────────
        │
        ▼
SHELL32.sll / USER32.sll / COMCTL32.sll / COMDLG32.sll / GDI32.sll / OLE32.sll / KERNEL32.sll / BOSLL.sll
        │
        ▼
ATOMS Kernel
```

---

## Technical Constraints & Design Principles

1. **Not a Mere File Manager:** Explorer.exe is the entire Desktop Environment session host.
2. **Strict Component Ownership Boundaries:**
   - Explorer DOES NOT own windows (USER32 owns window management).
   - Explorer DOES NOT own graphics/drawing (GDI32 / OpenGL32 / AGP own rendering).
   - Explorer DOES NOT own process creation or system scheduling (KERNEL32 / BOSLL / Kernel own process lifetime).
3. **Single Session Process Ownership:** PID 101 acts as the authoritative Desktop Session Owner.
4. **Zero Kernel Bypass:** All Explorer operations delegate strictly to Ring 3 runtime libraries (`SHELL32`, `USER32`, `COMCTL32`, `COMDLG32`, `OLE32`, `WS2_32`, `ADVAPI32`, `KERNEL32`, `BOSLL`).
