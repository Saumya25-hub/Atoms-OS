# Demo Applications

Five demo applications were created in `kernel/apps/`:
1. Terminal
2. Music Player
3. Settings
4. Files
5. Calculator

Each application simply calls `window_create()` with varying dimensions and PIDs. This is sufficient to prove that the Window Manager can track, render, and cleanly destroy overlapping windows without visual tearing.
