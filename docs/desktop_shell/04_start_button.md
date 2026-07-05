# Start Button

The Start Button is the core navigational entry point for ATOMS OS. 

## Interactions

It maintains three states:
1. `Normal`: Standard blue button.
2. `Hover`: Slightly darker blue when the mouse enters the bounding box.
3. `Pressed`: Darkest blue when left-clicked.

## Event Generation

When a left click is released *inside* the Start Button bounds (transitioning from Pressed -> Hover), it fires the `GUI_EVENT_STARTMENU_OPEN` event, which the global shell router will eventually use to spawn the Start Menu surface.
