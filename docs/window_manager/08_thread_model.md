# Thread Model

The Window Manager architecture is designed with future multi-threading in mind, though it operates synchronously initially.

## Architecture

1. **GUI Thread (Future)**: Handles input routing and application message passing.
2. **Compositor Thread**: Wakes up every frame (16.6ms), reads the dirty region queue, performs rendering passes, and pushes to VBE.
3. **Application Threads**: Applications issue commands (e.g., `window_move`) which push invalidation rectangles to the global queue without directly modifying the hardware.

## Synchronization

All access to the Surface Tree and the Dirty Region Queue must eventually be protected by locks to allow safe multi-threaded rendering and window management.
