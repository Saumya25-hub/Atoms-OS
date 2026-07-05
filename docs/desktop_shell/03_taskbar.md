# Taskbar Component

The Taskbar is a horizontal surface anchored to the bottom of the screen. 

## Structure

- **Height**: Configurable (Default: 40px).
- **Surface Parent**: It is currently appended to the Desktop root surface.
- **Contents**: It owns and draws the Start Button. Future implementation will include running application tabs, a system tray, and a clock.

## Optimization

Because the taskbar relies on `taskbar_hit_test` which delegates to the `start_button_hit_test`, only the localized bounding box of the start button is submitted to the Dirty Region Engine on hover changes, preserving 60 FPS performance.
