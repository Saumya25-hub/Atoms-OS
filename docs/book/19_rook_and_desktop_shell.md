# Chapter 19: Rook Login Supervisor & Desktop Shell

The desktop experience in ATOMS OS is governed by the **Rook** supervisor subsystem and the native Desktop Shell.

## 1. Rook Authentication Loop
- Initializes during late boot, presenting an interactive user authentication screen.
- Drains hardware USB keyboard and mouse input queues.
- Upon successful authentication, triggers smooth wallpaper transition and launches the main desktop environment.

## 2. Desktop Shell Architecture
- **Taskbar**: Renders system status, active applications, volume controls, and the ATOMS start menu.
- **Window Management**: Supports moveable, resizable, focusable client windows with title bars and close/minimize/maximize buttons.
- **Asset Integration**: High-resolution icons and wallpapers are dynamically rendered from kernel memory pools.
