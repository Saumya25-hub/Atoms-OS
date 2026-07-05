# Phase 8.5 — GUI Controls Framework Overview

## Introduction
The GUI Controls Framework (Phase 8.5) introduces a robust, modular, and reusable native widget library for ATOMS OS. It sits directly on top of the BOS Window Manager V2 and utilizes the existing `Painter`, `Surface Engine`, and `Dirty Region` infrastructure.

## Core Philosophy
1. **Inheritance-like Architecture**: All GUI controls inherit from a single base structure `BOSControl`. This guarantees polymorphism and uniform event handling.
2. **Native Rendering**: No direct framebuffer access. All rendering happens via the `Painter` to the window's client `Surface`, which is composited natively.
3. **Optimized Drawing**: Controls invoke `control_invalidate()`, pushing precise dirty rectangles to the region engine. The screen is never fully redrawn on hover or click.
4. **Theme Engine**: Complete separation of logic and styling. All colors are fetched from the `theme_engine`.

## Supported Controls
- **Button**: Standard push button with normal, hover, pressed, and disabled states.
- **Label**: Static text display with alignment.
- **TextBox**: Interactive single-line text input with caret rendering and keyboard support.
- **Panel**: Container control to group other controls.
- **CheckBox**: Boolean toggle control.
- **RadioButton**: Exclusive selection group control.
- **ProgressBar**: Visual completion indicator.
- **Slider**: Draggable value selector.
- **ListBox**: Selectable list of items.
- **ScrollBar**: Standard view scroller.
- **PictureBox**: Bitmap rendering container.

## API Usage Example
```c
BOSWindow* win = window_create("Application", 800, 600, owner_pid);

BOSLabel* lbl = label_create("Hello, ATOMS OS");
label_set_position(lbl, 10, 10);
window_add_control(win, (BOSControl*)lbl);

BOSButton* btn = button_create("Click Me");
button_set_position(btn, 10, 40);
window_add_control(win, (BOSControl*)btn);

window_show(win);
```
