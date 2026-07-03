# First Sound - Testing

The First Sound test represents a 10-minute blocking operation that proves hardware stability.

## Procedure
1. Create Audio Stream
2. Start AC'97 Playback Engine
3. Generate 28.8 million frames of 440Hz sine wave data dynamically.
4. Stop engine and shutdown streams.

## Stress Conditions
This ensures that long-running audio playback does not incur memory leaks, ring buffer fragmentation, or hardware locking. In QEMU environments, this serves as a rapid integration test. On physical hardware, this serves as an exhaustive acoustic confirmation of the DAC integrity.
