# Task Panel Architecture

The Task Panel replaces the legacy Taskbar.

## Design Rules
- Zero polling. Fully event-driven via Window Manager hooks.
- Static dimensions (floating bottom center).
- Square tabs. No transparency.
- Live clock powered by RTC.
