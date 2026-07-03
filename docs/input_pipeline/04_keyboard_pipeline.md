# Keyboard Pipeline Enhancements

## Key Repeat and Drop Prevention
The PS/2 hardware auto-repeat floods IRQ 1 when a key is held down. In the legacy version, a 256-event limit caused fast typists or sustained repeat streams to drop keystrokes.

## Upgrades
- `KBD_BUF_SIZE` expanded to 1024.
- `BWE_EVENT_QUEUE_SIZE` expanded to 1024.
- Added `g_kbd_events_per_sec` telemetry.

We verified that the existing shift/caps lock map (`scancode_to_ascii_shift`) correctly flips characters, but by ensuring the queue never fills, we prevent modifier desync (e.g., dropping a `SHIFT UP` event and getting stuck in uppercase mode).
