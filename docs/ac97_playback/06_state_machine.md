# AC'97 Playback - State Machine

The playback driver maintains a higher-level state machine on top of the DMA foundation.

## States
- `UNINITIALIZED`: The base addresses have been acquired, but no memory is allocated.
- `PREPARED`: The DMA manager has allocated the physical ring buffer and programmed the BDBAR.
- `STARTING`: The software mixer is currently pre-filling the 32 descriptors.
- `RUNNING`: The RPBM bit is high; the hardware is streaming.
- `STOPPING`: The RPBM bit is low; the software is polling for the `DCH` halt signal.
- `STOPPED`: The hardware is confirmed idle, but the physical memory remains allocated.
- `ERROR`: Catastrophic initialization failure.

## Transitions
To ensure safety, the software enforces strict transition logic. For example, `ac97_playback_start()` will immediately return `false` if the state is not `PREPARED` or `STOPPED`. It is impossible to invoke `shutdown()` while the engine is `RUNNING` because `shutdown()` forcefully invokes `stop()` and waits for completion first.
