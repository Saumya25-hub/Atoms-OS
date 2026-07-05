# Start Menu Manager

The Start Menu (`kernel/gui/startmenu/start_menu.c`) is a floating panel.

## Mechanics

- **Initialization**: Created as a child of the Desktop Root, but positioned precisely above the Taskbar.
- **Z-Ordering**: Whenever `start_menu_show()` is called, it detaches and reattaches itself to the head of the root's children, ensuring it renders on top of all application windows.
- **Hit Testing**: Captures clicks first. Clicking outside of its bounds automatically closes it (`start_menu_hide()`).
